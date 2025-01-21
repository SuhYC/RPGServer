using JetBrains.Annotations;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading.Tasks;
using Unity.Collections.LowLevel.Unsafe;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;

public class PacketMaker : MonoBehaviour
{
    // singleton
    private static PacketMaker _instance;

    private uint _ReqNo = 1; // 0�� Ŭ���̾�Ʈ�� ��û�� ������� �޽����̴�. (���ĸ޽���)
    private SortedDictionary<uint, ReqMessage> _msgs;

    // �����κ��� ���� ����� �� ������ �°� �б��ϱ� ����
    public delegate Task ResHandleFunc(ReqMessage reqmsg_, ResMessage resmsg_);
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
        ResHandleFunc ReserveNick = HandleReserveNicknameResponse;
        ResHandleFunc CreateChar = HandleCreateCharResponse;
        ResHandleFunc CancelReserve = HandleCancelReserveNicknameResponse;
        ResHandleFunc GetCharInfo = HandleGetCharInfoResponse;

        _resHandleFuncs.Add(ReqType.SIGNIN, SignIn);
        _resHandleFuncs.Add(ReqType.SIGNUP, SignUp);
        _resHandleFuncs.Add(ReqType.RESERVE_CHAR_NAME, ReserveNick);
        _resHandleFuncs.Add(ReqType.CANCEL_CHAR_NAME_RESERVE, CancelReserve);
        _resHandleFuncs.Add(ReqType.CREATE_CHAR, CreateChar);
        _resHandleFuncs.Add(ReqType.GET_CHAR_INFO, GetCharInfo);

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

    public async Task HandleServerResponse(string msg_)
    {
        ResMessage res = JsonUtility.FromJson<ResMessage>(msg_);

        Debug.Log($"PacketMaker::HandleServerResponse : Recvmsg : {msg_}");
        Debug.Log($"PacketMaker::HandleServerResponse : ReqNo : {res.ReqNo}");

        // res message
        if (res.ReqNo != 0)
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
                    await (((ResHandleFunc)_resHandleFuncs[reqmsg.type]).Invoke(reqmsg, res));
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

    private async Task HandleSignUpResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                TextMessage.CreateTextPanel("Sign Up SUCCESSLY");
                //Debug.Log("PacketMaker::HandleSignUpResponse : SUCCESS!!");
                break;

            case ResCode.WRONG_PARAM:
                TextMessage.CreateTextPanel("WRONG PARAMETER!");
                //Debug.Log("PacketMaker::HandleSignUpResponse : WRONG_PARAM..");
                break;

            case ResCode.SIGNUP_ALREADY_IN_USE:
                TextMessage.CreateTextPanel("YOUR ID IS ALREADY IN USE!");
                //Debug.Log("PacketMaker::HandleSignUpResponse : SIGNUP_ALREADY_IN_USE..");
                break;

            case ResCode.SIGNUP_FAIL:
                TextMessage.CreateTextPanel("FAILED TO SIGN UP.");
                //Debug.Log("PacketMaker::HandleSignUpResponse : SIGNUP_FAIL..");
                break;

            case ResCode.SYSTEM_FAIL:
                TextMessage.CreateTextPanel("ERRORCODE 2");
                //Debug.Log("PacketMaker::HandleSignUpResponse : SYSTEM_FAIL..");
                break;

            default:
                Debug.Log("PacketMaker::HandleSignUpResponse : Undefined ResCode");
                break;
        }

        return;
    }

    private async Task HandleSignInResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                await UserData.instance.InitCharList(resmsg_.Msg);
                //Debug.Log("PacketMaker::HandleSignInResponse : SUCCESS!!");

                if(UserData.instance.IsCompletelyLoaded())
                {
                    SceneManager.LoadScene("SelectCharacterScene");
                }

                break;

            case ResCode.WRONG_PARAM:
                TextMessage.CreateTextPanel("Wrong Input.");
                //Debug.Log("PacketMaker::HandleSignInResponse : WRONG_PARAM.");
                break;

            case ResCode.SIGNIN_FAIL:
                TextMessage.CreateTextPanel("Wrong ID/PW.");
                //Debug.Log("PacketMaker::HandleSignInResponse : SIGNIN_FAIL.");
                break;

            case ResCode.SIGNIN_ALREADY_HAVE_SESSION:
                TextMessage.CreateTextPanel("This ID Already Have Session.");
                //Debug.Log("PacketMaker::HandleSignInResponse : ALREADY HAVE SESSION.");
                break;

            default:
                Debug.Log("PacketMaker::HandleSignInResponse : Undefined ResCode");
                break;
        }

        return;
    }

    /// <summary>
    /// SignIn�� ���������� ����Ϸ��� ������
    /// SignIn �Ŀ� �ݵ�� ��û�Ͽ��� �ϹǷ� HandleSignIn ������ �ذ�.
    /// ReqCharList�� ���������� ��������� �𸣰���.
    /// </summary>
    /// <returns></returns>
    private async Task ReqCharList()
    {
        string str = ToReqMessage(ReqType.GET_CHAR_LIST, "");
        await NetworkManager.Instance.SendMsg(str);
        return;
    }

    private async Task HandleGetCharListResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;
        Charlist charlist = JsonUtility.FromJson<Charlist>(resmsg_.Msg);

        // ... ��Ƶ� ��ũ��Ʈ�� �����ؼ� ������Ʈ �ϴ� ������ ����.

        return;
    }

    private async Task HandleGetCharInfoResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        UserData.instance.InitCharInfo(resmsg_.Msg);

        if(UserData.instance.IsCompletelyLoaded())
        {
            SceneManager.LoadScene("SelectCharacterScene");
        }

        return;
    }

    private async Task HandleReserveNicknameResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask; // async Task�� ���� �ʿ䰡 ������?
        Debug.Log($"PacketMaker::HandleReserveNicknameResponse : rescode : {resmsg_.ResCode}");

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                CheckNicknamePanel.CreateTextPanel(reqmsg_.msg);
                Debug.Log($"PacketMaker::HandleReserveNicknameResponse : SUCCESS!");
                break;

            case ResCode.WRONG_PARAM:
                TextMessage.CreateTextPanel("Wrong input.");
                Debug.Log($"PacketMaker::HandleReserveNicknameResponse : WRONG_PARAM");
                break;

            case ResCode.REQ_FAIL:
                TextMessage.CreateTextPanel("������ �� ���� �г����Դϴ�.");
                Debug.Log($"PacketMaker::HandleReserveNicknameResponse : REQ_FAIL");
                break;

            default:
                Debug.Log($"PacketMaker::HandleReserveNicknameResponse : Undefined ResCode.");
                break;
        }
    }

    private async Task HandleCreateCharResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        Debug.Log($"PacketMaker::HandleCreateCharResponse : rescode : {resmsg_.ResCode}");

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                // ���� ���� ó��
                Debug.Log($"PacketMaker::HandleCreateCharResponse : SUCCESS!");
                break;

            case ResCode.WRONG_PARAM:
                TextMessage.CreateTextPanel("Wrong input.");
                Debug.Log($"PacketMaker::HandleCreateCharResponse : WRONG_PARAM");
                break;

            case ResCode.REQ_FAIL:
                TextMessage.CreateTextPanel("������ �����߽��ϴ�.");
                Debug.Log($"PacketMaker::HandleCreateCharResponse : REQ_FAIL");
                break;

            case ResCode.SYSTEM_FAIL:
                TextMessage.CreateTextPanel("ERRORCODE 2");
                Debug.Log($"PacketMaker::HandleCreateCharResponse : SYSTEM_FAIL");
                break;

            default:
                Debug.Log($"PacketMaker::HandleCreateCharResponse : Undefined ResCode.");
                break;
        }
    }

    private async Task HandleCancelReserveNicknameResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        Debug.Log($"PacketMaker::HandleCancelReserveNicknameResponse : rescode : {resmsg_.ResCode}");

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                Debug.Log($"PacketMaker::HandleCancelReserveNicknameResponse : SUCCESS!");
                break;

            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleCancelReserveNicknameResponse : WRONG_PARAM");
                break;

            case ResCode.REQ_FAIL:
                Debug.Log($"PacketMaker::HandleCancelReserveNicknameResponse : REQ_FAIL");
                break;

            default:
                Debug.Log($"PacketMaker::HandleCancelReserveNicknameResponse : Undefined ResCode.");
                break;
        }
    }

    private void HandlePosInfo(ResMessage resmsg_)
    {
        // ���� �ʿ�
        return;
    }
}
