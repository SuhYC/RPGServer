using System;
using System.Collections;
using System.Collections.Generic;
using Unity.Collections.LowLevel.Unsafe;
using UnityEditor;
using UnityEngine;

public class PacketMaker : MonoBehaviour
{
    // singleton
    private static PacketMaker _instance;

    private uint _ReqNo = 1; // 0�� Ŭ���̾�Ʈ�� ��û�� ������� �޽����̴�. (���ĸ޽���)
    private SortedDictionary<uint, ReqMessage> _msgs;

    // �����κ��� ���� ����� �� ������ �°� �б��ϱ� ����
    public delegate void ResHandleFunc(ReqMessage reqmsg_, ResMessage resmsg_);
    Hashtable _resHandleFuncs;
    public delegate void InfoHandleFunc(ResMessage resmsg_);
    Hashtable _infoHandleFuncs;

    public enum ReqType
    {
        SIGNIN,                     // �α���
        SIGNUP,                     // ȸ������
        MODIFY_PW,                  // ��й�ȣ ����
        GET_CHAR_LIST,              // �ش� ������ ĳ���� �ڵ� ����Ʈ ��û (�α��� ���� ����ؾ���)
        GET_CHAR_INFO,              // Ư�� ĳ���� �ڵ忡 �´� ĳ���� ���� ��û
        RESERVE_CHAR_NAME,          // ĳ���� ���� �� �г����� �Է��� ��� �̸� ������ �� ��, ������ �г����� ��� ����� �� ���θ� �����Ѵ�.
        CANCEL_CHAR_NAME_RESERVE,   // RESERVE_CHAR_NAME���� ������ �г����� ����Ѵ�.
        CREATE_CHAR,                // ĳ���� ���� ����. DB�� ĳ���� ������ ����Ѵ�.
        SELECT_CHAR,                // �ش� ĳ���� ����
        PERFORM_SKILL,              // ��ų ���
        GET_OBJECT,                 // �ʿ� �����ϴ� ������ ����
        BUY_ITEM,                   // ���� �̿�
        DROP_ITEM,                  // �κ��丮�� �������� ������ �ʿ� ����
        USE_ITEM,                   // �κ��丮�� ������ ���
        MOVE_MAP,                   // �� �̵� ��û
        CHAT_EVERYONE,              // �ش� ���� ��� �ο����� ä�� (�ٸ� ������ ä���� �� �����غ���.)

        POS_INFO,                   // ĳ������ ��ġ, �ӵ� � ���� ������ ������Ʈ�ϴ� �Ķ����
        LAST = POS_INFO				// enum class�� �����Ǹ� ������ ���ҷ� ������ ��
    }

    public enum ResCode
    {
        // -----������ ��û�� ���� ����-----

        SUCCESS,
        WRONG_PARAM,                    // ��û��ȣ�� ���� �ʴ� �Ķ����
        SYSTEM_FAIL,                    // �ý����� ����
        SIGNIN_FAIL,                    // ID�� PW�� ���� ����
        SIGNIN_ALREADY_HAVE_SESSION,    // �̹� �α��ε� ����
        SIGNUP_FAIL,                    // ID��Ģ�̳� PW��Ģ�� ���� ����
        SIGNUP_ALREADY_IN_USE,          // �̹� ������� ID
        WRONG_ORDER,                    // ��û�� ��Ȳ�� ���� ����
        MODIFIED_MAPCODE,               // �ش� ��û�� ���� ���� ���ڵ�� ���� ������ ������ �ִ� �ش� ������ ���ڵ尡 ������.
        REQ_FAIL,                       // ���� ���ǿ� ���� �ʾ� ������ ������ (�̹� ����� ������ ��..)
        UNDEFINED,                      // �˼����� ����

        // -----���� �ش� ������ ��û�� ���� ���޵� ������ �ƴ� �������� -----
        // (ä��, �ʺ�ȭ, �ٸ��÷��̾� ��ȣ�ۿ�...)

        SEND_INFO_POS,                  // Ÿ �÷��̾��� ��ġ����
        SEND_INFO_PERFORM_SKILL,        // Ÿ �÷��̾��� ��ų �ߵ� ����
        SEND_INFO_GET_DAMAGE,           // Ÿ �÷��̾��� �ǰ� ����
        SEND_INFO_MONSTER_DESPAWN,      // �ش� ���� ���� ��� ����
        SEND_INFO_MONSTER_GET_DAMAGE,   // �ش� ���� ���� �ǰ� ����
        SEND_INFO_MONSTER_CREATED,		// �ش� ���� ���� ���� ����
    }

    [Serializable]
    public class ReqMessage
    {
        public ReqMessage(ReqType type_,  uint reqNo_, string msg_)
        {
            type = type_;
            reqNo = reqNo_;
            msg = msg_;
        }

        public ReqType type;
        public uint reqNo;
        public string msg;
    }

    [Serializable]
    public class ResMessage
    {
        public ResMessage() { }

        public uint ReqNo;
        public ResCode ResCode;
        public string Msg;
    }

    public static PacketMaker instance
    {
        get
        {
            if (_instance == null)
            {
                _instance = FindFirstObjectByType<PacketMaker>();
                if (_instance == null)
                {
                    GameObject packetManagerObj = new GameObject("PacketManager");
                    DontDestroyOnLoad(packetManagerObj);
                    _instance = packetManagerObj.AddComponent<PacketMaker>();

                    _instance.Init();
                }
            }
            return _instance;
        }
    }

    void Awake()
    {
        if (_instance == null)
        {
            _instance = this;
            this.Init();
        }
        else if(_instance != this)
        {
            Destroy(this.gameObject);
        }
    }

    private void Init()
    {
        DontDestroyOnLoad(this.gameObject);
        _msgs = new SortedDictionary<uint, ReqMessage>();
        _resHandleFuncs = new Hashtable();
        _infoHandleFuncs = new Hashtable();

        ResHandleFunc SignIn = HandleSignInResponse;
        ResHandleFunc SignUp = HandleSignUpResponse;

        _resHandleFuncs.Add(ReqType.SIGNIN, SignIn);
        _resHandleFuncs.Add(ReqType.SIGNUP, SignUp);

        InfoHandleFunc Pos = HandlePosInfo;

        _infoHandleFuncs.Add(ResCode.SEND_INFO_POS, Pos);
    }

    public string ToReqMessage(ReqType type_, string msg_)
    {
        _ReqNo++;

        try
        {
            _msgs.Add(_ReqNo, new ReqMessage(type_, _ReqNo, msg_));
        }
        catch (ArgumentException)
        {
            Debug.Log("PacketMaker::ToMessage : ReqNo �浹");
        }
        catch (Exception e)
        {
            Debug.Log($"PacketMaker::ToMessage : {e.Message}");
        }

        Debug.Log($"PacketMaker::ToMessage : {_ReqNo}, {_msgs[_ReqNo].ToString()}");

        return JsonUtility.ToJson(_msgs[_ReqNo]);
    }

    public void HandleServerResponse(string msg_)
    {
        ResMessage res = JsonUtility.FromJson<ResMessage>(msg_);

        Debug.Log(msg_);
        Debug.Log(res.ReqNo);

        // res message
        if(res.ReqNo != 0)
        {
            ReqMessage reqmsg;
            if(!_msgs.TryGetValue(res.ReqNo, out reqmsg))
            {
                Debug.Log("PacketMaker::HandleServerResponse : Invalid ReqNo.");
                return;
            }

            try
            {
                if (_resHandleFuncs[reqmsg.type] != null)
                {
                    ((ResHandleFunc)_resHandleFuncs[reqmsg.type]).Invoke(reqmsg, res);
                }
            }
            catch (InvalidCastException e)
            {
                Debug.Log($"PacketMaker::HandleServerResponse : Not A Delegate In _resHandleFuncs, {e.Message}");
            }
            catch (Exception e)
            {
                Debug.Log($"PacketMaker::HandleServerResponse : {e.Message}");
            }

            _msgs.Remove(res.ReqNo);
        }
        // info message
        else
        {
            Debug.Log(res.ReqNo);

            try
            {
                if (_infoHandleFuncs[res.ResCode] != null)
                {
                    ((InfoHandleFunc)_infoHandleFuncs[(int)res.ResCode]).Invoke(res);
                }
            }
            catch (InvalidCastException e)
            {
                Debug.Log($"PacketMaker::HandleServerResponse : Not A Delegate In _infoHandleFuncs, {e.Message}");
            }
            catch (Exception e)
            {
                Debug.Log($"PacketMaker::HandleServerResponse : {e.Message}");
            }
        }

        return;
    }


    private void HandleSignUpResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        Debug.Log(resmsg_.ResCode);

        if(resmsg_.ResCode == ResCode.SUCCESS)
        {
            Debug.Log("PacketMaker::HandleSignUpResponse : SUCCESS!!");
        }
        else if(resmsg_.ResCode == ResCode.WRONG_PARAM)
        {
            Debug.Log("PacketMaker::HandleSignUpResponse : WRONG_PARAM..");
        }
        else if(resmsg_.ResCode == ResCode.SIGNUP_ALREADY_IN_USE)
        {
            Debug.Log("PacketMaker::HandleSignUpResponse : SIGNUP_ALREADY_IN_USE..");
        }
        else if(resmsg_.ResCode == ResCode.SIGNUP_FAIL)
        {
            Debug.Log("PacketMaker::HandleSignUpResponse : SIGNUP_FAIL..");
        }
        else if(resmsg_.ResCode == ResCode.SYSTEM_FAIL)
        {
            Debug.Log("PacketMaker::HandleSignUpResponse : SYSTEM_FAIL..");
        }
        else
        {
            Debug.Log("PacketMaker::HandleSignUpResponse : Undefined ResCode");
        }
    }

    private void HandleSignInResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        Debug.Log(resmsg_.ResCode);

        if(resmsg_.ResCode == ResCode.SUCCESS)
        {

        }
        else if(resmsg_.ResCode == ResCode.WRONG_PARAM)
        {

        }
        else if(resmsg_.ResCode == ResCode.SIGNIN_FAIL)
        {

        }
        else if(resmsg_.ResCode == ResCode.SIGNIN_ALREADY_HAVE_SESSION)
        {

        }
        else
        {
            Debug.Log("PacketMaker::HandleSignInResponse : Undefined ResCode");
        }

        return;
    }

    private void HandlePosInfo(ResMessage resmsg_)
    {

        return;
    }
}
