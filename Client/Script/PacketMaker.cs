using JetBrains.Annotations;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading.Tasks;
using Unity.Collections.LowLevel.Unsafe;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UIElements;



/// <summary>
/// �����κ��� ������ ������ ResHandler���� ������ ������ ������ �����͸� �������� Ŭ���̾�Ʈ�� �ݿ��ϴ� Ŭ�����̴�.
/// ������ Ư�� ��û�� �ϱ� ���� ReqMessage�� �ۼ��ϴ� Ŭ�����̱⵵ �ϴ�.
/// 
/// �۽ſ� ���Ǵ� �޽����� ������
/// [size]ReqJsonStr �̸�
/// 
/// ReqJsonStr�� ReqType, ReqNo, Msg�� �����Ǿ� �ִ�.
/// ReqType�� ��û�ϴ� �����̳� �������� ��ȣ�̸�
/// ReqNo�� Ŭ���̾�Ʈ���� ������ ó���� ���� ��û��ȣ�̴�. ������ ó���� ���� ��û�� ��û��ȣ�� �״�� �������Ѵ�.
/// Msg�� ReqType�� �´� �Ķ���ͷ� ������ JsonStr�̴�.
/// 
/// ���ſ� ���Ǵ� �޽����� ������
/// [size]ResJsonStr �̸�
/// 
/// ResJsonStr�� ReqNo, ResCode, Msg�� �����Ǿ� �ִ�.
/// ReqNo�� ReqMessage�� �ԷµǾ��� ��û��ȣ�� �״�� �����Ѵ�. ���� ��û�� ������ �ƴϰ� ���ĵ� ������� ReqNo�� 0���� ���ŵȴ�.
/// ResCode�� ó������� ������ ��� Enum���̴�.
/// Msg�� Optional Message�̸� �����͸� ��û�� ��쿡 �ش� �����͸� Msg�� ��� ������ �������ش�.
/// 
/// 
/// ������ �������κ��� ResHandler�� [size]����� �����ϰ� ResJsonStr�� HandleServerResponse�� �Ű������� ȣ���ϰ� �ȴ�.
/// ResJsonStr�� ResMessage�� ������ȭ�Ͽ� ReqNo�� 0���� Ȯ���ϰ� 0�̶�� ResCode�� ���� � ������������ �Ǵ��Ͽ� hashtable�� ����� �Լ��� ȣ���Ѵ�.
/// ReqNo�� 0�� �ƴ϶�� ��û�ߴ� �޽��� ����Ʈ���� ReqNo�� ã�� � ��û�̾����� Ȯ���ϰ� hashtable�� ����� �Լ��� ȣ���� ����� Ŭ���̾�Ʈ�� �ݿ��Ѵ�.
/// 
/// ReqNo�� ���� ����ϴ� ������ ������ ��û�� ���� <-> Ŭ���̾�Ʈ�� �������� TCP�� �������忡 ���� ������ �ٲ��� ������
/// �������� ó���ϴ� ���������� ó���ð��� ���� ����� ������ ����� �� �����Ƿ� � ��û�� ���� �������� Ȯ���� ����� �ʿ��ϴ�.
/// 
/// �۽ſ� ����� �޽����� ToReqMessage�� ���� �ۼ��ϰ�, NetworkManaer�� SendMsg�� ���� �����Ѵ�.
/// </summary>
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
        GET_INVEN,                  // �κ��丮 ���� ��û
        GET_SALESLIST,              // Ư�� npc�� �ǸŸ�� ��û
        DEBUG_SET_GOLD,             // ������. ��� ����
        SWAP_INVENTORY,             // �κ��丮 ���� 2���� ������ ��ȯ.

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
        SEND_INFO_MONSTER_CREATED,      // �ش� ���� ���� ���� ����
        SEND_INFO_CHAT_EVERYONE,        // �ش� ���� ��ο��� ä��
        SEND_INFO_OBJECT_CREATED,       // �ش� ���� ������ ���� ����
        SEND_INFO_OBJECT_DISCARDED,     // �ش� ���� ������ �Ҹ� ����
        SEND_INFO_OBJECT_OBTAINED		// �ش� ���� ������ Ÿ�ο� ���� �������� (�ڽ��� ������ Req�� ���� SUCCESS�� ó��)
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

    public class SelectCharParam
    {
        public int Charcode;
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


    /// <summary>
    /// �ν��Ͻ��� �ʱ�ȭ�մϴ�. <br/>
    /// ������ ��û�� ���� ����޽��� ó���Լ��� hashtable�� ��� ������ �����մϴ�. <br/>
    /// ��û�� �ƴ� �������� �����ϴ� �޽��� ó���Լ��� hashtable�� ���� ����ϴ�.
    /// </summary>
    private void Init()
    {
        DontDestroyOnLoad(this.gameObject);
        _msgs = new SortedDictionary<uint, ReqMessage>();
        _resHandleFuncs = new Hashtable();
        _infoHandleFuncs = new Hashtable();

        // ----- Response to Req -----

        ResHandleFunc SignIn = HandleSignInResponse;
        _resHandleFuncs.Add(ReqType.SIGNIN, SignIn);

        ResHandleFunc SignUp = HandleSignUpResponse;
        _resHandleFuncs.Add(ReqType.SIGNUP, SignUp);

        ResHandleFunc ReserveNick = HandleReserveNicknameResponse;
        _resHandleFuncs.Add(ReqType.RESERVE_CHAR_NAME, ReserveNick);

        ResHandleFunc CreateChar = HandleCreateCharResponse;
        _resHandleFuncs.Add(ReqType.CREATE_CHAR, CreateChar);

        ResHandleFunc CancelReserve = HandleCancelReserveNicknameResponse;
        _resHandleFuncs.Add(ReqType.CANCEL_CHAR_NAME_RESERVE, CancelReserve);

        ResHandleFunc GetCharInfo = HandleGetCharInfoResponse;
        _resHandleFuncs.Add(ReqType.GET_CHAR_INFO, GetCharInfo);

        ResHandleFunc GetInven = HandleGetInvenResponse;
        _resHandleFuncs.Add(ReqType.GET_INVEN, GetInven);

        ResHandleFunc SelectChar = HandleSelectCharResponse;
        _resHandleFuncs.Add(ReqType.SELECT_CHAR, SelectChar);

        ResHandleFunc MoveMap = HandleMoveMapResponse;
        _resHandleFuncs.Add(ReqType.MOVE_MAP, MoveMap);

        ResHandleFunc GetSalesList = HandleGetSalesListResponse;
        _resHandleFuncs.Add(ReqType.GET_SALESLIST, GetSalesList);

        ResHandleFunc BuyItem = HandleBuyItemResponse;
        _resHandleFuncs.Add(ReqType.BUY_ITEM, BuyItem);

        ResHandleFunc SwapInven = HandleSwapInventory;
        _resHandleFuncs.Add(ReqType.SWAP_INVENTORY, SwapInven);

        ResHandleFunc DropItem = HandleDropObjectResponse;
        _resHandleFuncs.Add(ReqType.DROP_ITEM, DropItem);

        ResHandleFunc GetItem = HandleGetObjectResponse;
        _resHandleFuncs.Add(ReqType.GET_OBJECT, GetItem);

        // ----- SendInfo -----

        InfoHandleFunc Pos = HandlePosInfo;
        _infoHandleFuncs.Add(ResCode.SEND_INFO_POS, Pos);

        InfoHandleFunc Chat = HandleChat;
        _infoHandleFuncs.Add(ResCode.SEND_INFO_CHAT_EVERYONE, Chat);

        InfoHandleFunc CreateObject = HandleCreationObject;
        _infoHandleFuncs.Add(ResCode.SEND_INFO_OBJECT_CREATED, CreateObject);

        InfoHandleFunc DiscardObject = HandleDiscardObject;
        _infoHandleFuncs.Add(ResCode.SEND_INFO_OBJECT_DISCARDED, DiscardObject);

        InfoHandleFunc ObtainObject = HandleObtainedObject;
        _infoHandleFuncs.Add(ResCode.SEND_INFO_OBJECT_OBTAINED, ObtainObject);

        // ----- For Debug -----
        ResHandleFunc SetGold = HandleSetGoldResponse;
        _resHandleFuncs.Add(ReqType.DEBUG_SET_GOLD, SetGold);


    }

    /// <summary>
    /// ��û�� ���ۿ� ���� ReqMsg�� ����ϴ�. <br/>
    /// ���ϰ��� NetworkManager.instance.SendMsg()�� ������ �˴ϴ�.
    /// </summary>
    /// <param name="type_">��û�� ������ enum�� �Է��մϴ�.</param>
    /// <param name="msg_">��û�� ������ �Ķ���͸� �Է��մϴ�.</param>
    /// <returns></returns>
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

        //Debug.Log($"PacketMaker::ToMessage : {_ReqNo}, {_msgs[_ReqNo].ToString()}");

        return JsonUtility.ToJson(_msgs[_ReqNo]);
    }


    /// <summary>
    /// ���Ÿ޽����� payload�� �޾� ó���մϴ�.
    /// 
    /// ReqNo�� Ȯ���Ͽ� 0�̶�� Ŭ���̾�Ʈ���� ��û�� ���� �ƴ� �������� �Ϲ������� �����ϴ� �޽����Դϴ�.
    /// 0�̶�� ResCode�� ���� � ���ĸ޽������� Ȯ���Ͽ� ó���ϰ�
    /// 0�� �ƴ϶�� ReqNo�� ��ȸ�Ͽ� � ��û�� �߾����� Ȯ���ϰ� ó���մϴ�.
    /// </summary>
    /// <param name="msg_">ResMessage�� JSONStr</param>
    /// <returns></returns>
    public async Task HandleServerResponse(string msg_)
    {
        ResMessage res;

        try
        {
            res = JsonUtility.FromJson<ResMessage>(msg_);
        }
        catch (Exception e)
        {
            Debug.Log($"PacketMaker::HandleServerResponse : Parsing Error On Msg : {msg_}   . {e.Message}");
            return;
        }

        //Debug.Log($"PacketMaker::HandleServerResponse : Recvmsg : {msg_}");
        //Debug.Log($"PacketMaker::HandleServerResponse : ReqNo : {res.ReqNo}");

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
                if (_resHandleFuncs.ContainsKey(reqmsg.type) && _resHandleFuncs[reqmsg.type] != null)
                {
                    await ((ResHandleFunc)_resHandleFuncs[reqmsg.type]).Invoke(reqmsg, res);
                }
                else
                {
                    Debug.Log($"PacketMaker::HandleServerResponse : {reqmsg.type}");
                }
            }
            catch (InvalidCastException e)
            {
                Debug.Log($"PacketMaker::HandleServerResponse : Not A Delegate In _resHandleFuncs, {e.Message}");
            }
            catch (Exception e)
            {
                Debug.Log($"PacketMaker::HandleServerResponse : {e.Message}");
                Debug.Log($"PacketMaker::HandleServerResponse : stack trace : {e.StackTrace}");
            }

            _msgs.Remove(res.ReqNo);
        }
        // info message
        else
        {
            Debug.Log($"PacketMaker::HandleServerResponse : info-> res.Reqno : {res.ReqNo}");

            try
            {
                Debug.Log($"PacketMaker::HandleServerResponse : rescode : {(int)res.ResCode}");
                if (_infoHandleFuncs[res.ResCode] != null)
                {
                    ((InfoHandleFunc)_infoHandleFuncs[res.ResCode]).Invoke(res);
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


    /// <summary>
    /// ȸ������ ��û�� ���� ������ ó���մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
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

    /// <summary>
    /// �α��� ��û�� ���� ������ ó���մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleSignInResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                await UserData.instance.InitCharList(resmsg_.Msg);
                //Debug.Log("PacketMaker::HandleSignInResponse : SUCCESS!!");

                if(UserData.instance.IsCompletelyLoadedCharInfo())
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
    /// [�̻��]
    /// 
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

    /// <summary>
    /// [�̻��]
    /// </summary>
    /// <param name="reqmsg_"></param>
    /// <param name="resmsg_"></param>
    /// <returns></returns>
    private async Task HandleGetCharListResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;
        Charlist charlist = JsonUtility.FromJson<Charlist>(resmsg_.Msg);

        // ... ��Ƶ� ��ũ��Ʈ�� �����ؼ� ������Ʈ �ϴ� ������ ����.

        return;
    }


    /// <summary>
    /// CharInfo�� ��û�ϴ� �޽����� ����޽����� ó���ϴ� �Լ�. <br/>
    /// �Ϲ������� �α��� ���Ŀ� CharInfo�� ��û�ϰ� �ǹǷ� <br/>
    /// UserData�� �����ϰ� ���� ���� LoginScene�̶�� UserData�� �����ؾ��ϴ� �����Ͱ� ���� ����Ǿ����� Ȯ���ϰ� ���� �����մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleGetCharInfoResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        UserData.instance.InitCharInfo(resmsg_.Msg);

        Scene currentScene = SceneManager.GetActiveScene();

        if(currentScene.name == "LoginScene" && UserData.instance.IsCompletelyLoadedCharInfo())
        {
            SceneManager.LoadScene("SelectCharacterScene");
        }

        return;
    }


    /// <summary>
    /// ĳ���� ������ ���� �г����� �����ϴ� ��û�� ���� ����޽����� ó���ϴ� �Լ��Դϴ�. <br/>
    /// 
    /// ���������ϴٰ� ������ ��� �ش� �г������� �����ϴ� ���� �´��� Ȯ���ϴ� �г��� �����մϴ�. <br/>
    /// ���࿡ ������ ��� ������ ��Ȳ�� �´� �ؽ�Ʈ�г��� ����Ͽ� ����ڰ� Ȯ���� �� �ְ� �մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleReserveNicknameResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

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


    /// <summary>
    /// ������ �г��ӿ� ���� ������ ��û�� �Ϳ� ���� ����޽����� ó���ϴ� �Լ��Դϴ�. <br/>
    /// 
    /// ������ �����ߴٸ� UserData�� CharList�� �߰��ϸ� ĳ���� ����ȭ������ ���ư��ϴ�. <br/>
    /// ������ �����ߴٸ� � ������ �����ߴ��� �ؽ�Ʈ�޽����� �����մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleCreateCharResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                UserData.instance.AddCharList(resmsg_.Msg);

                ReturnToCharlistPanel();

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


    /// <summary>
    /// ĳ���� ����â���� ���ư��ϴ�.
    /// </summary>
    private void ReturnToCharlistPanel()
    {
        Canvas canvas = FindAnyObjectByType<Canvas>();

        if (canvas == null)
        {
            Debug.Log($"PacketMaker::ReturnToCharlistPanel : nullref.");
            return;
        }

        Transform charlistPanel = canvas.transform.GetChild(0); // Charlist panel

        if (charlistPanel.name != "CharlistPanel")
        {
            Debug.Log($"PacketMaker::ReturnToCharlistPanel : Cant Find CharlistPanel");
            return;
        }
        charlistPanel.gameObject.SetActive(true);

        Transform createCharPanel = canvas.transform.GetChild(1); // CreateChar Panel

        if (createCharPanel.name != "CreateCharPanel")
        {
            Debug.Log($"PacketMaker::ReturnToCharlistPanel : Cant Find CreateCharPanel");
            return;
        }

        createCharPanel.gameObject.SetActive(false);

    }


    /// <summary>
    /// ĳ���� �г��� ������ ����ϴ� ��û�� ����޽����� ó���մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
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


    /// <summary>
    /// �κ��丮 ������ ��û�� �Ϳ� ���� ����޽����� ó���մϴ�.
    /// UserData�� �κ��丮 ���� JSONStr�� �����մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleGetInvenResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                UserData.GetCharInvenParam param = JsonUtility.FromJson<UserData.GetCharInvenParam>(reqmsg_.msg);

                int charcode = param.Charcode;

                UserData.instance.AddInvenJstr(charcode, resmsg_.Msg);
                break;

            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleGetInvenResponse : WRONG_PARAM!");
                break;

            case ResCode.SYSTEM_FAIL:
                Debug.Log($"PacketMaker::HandleGetInvenResponse : SYSTEM_FAIL!");
                break;

            default:

                break;

        }


        

        return;
    }


    /// <summary>
    /// ĳ���� ���� ��û�� ���� ����޽����� ó���մϴ�. <br/>
    /// 
    /// ĳ���� ���� ���� �ƴ϶�� �׳� �����մϴ�. <br/>
    /// �ش� ĳ������ �κ��丮 ������ ���ŵ��� �ʾҴٸ� �κ��丮 ������ ��û�ϰ� ����ϴ� �Լ��� ȣ���մϴ�. <br/>
    /// �κ��丮 ������ ���������� ���ŵǾ��ٸ� �κ��丮�гο� ������ ��� ���� ��ȯ�մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleSelectCharResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        if(SceneManager.GetActiveScene().name != "SelectCharacterScene")
        {
            Debug.Log($"PacketMaker::HandleSelectCharResponse : not in select char scene.");
            return;
        }

        switch(resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                //Debug.Log($"PacketMaker::HandleSelectCharResponse : SUCCESS!");
                SelectCharParam param = JsonUtility.FromJson<SelectCharParam>(reqmsg_.msg);

                if(await UserData.instance.CheckInvenLoaded(param.Charcode) == false)
                {
                    Debug.Log($"PacketMaker::HandleSelectCharResponse : Not Have Invenjstr");
                    return;
                }

                string invenjstr = UserData.instance.GetInvenJstr(param.Charcode);

                UserData.instance.SetSelectedCharNo(param.Charcode);
                InventoryPanel.InitInvenInfo(invenjstr);

                int mapCode = UserData.instance.GetMapCode();
                SceneManager.LoadScene(mapCode.ToString());

                break;
            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleSelectCharResponse : WRONG_PARAM");
                break;

            case ResCode.SYSTEM_FAIL:
                Debug.Log($"PacketMaker::HandleSelectCharResponse : SYSTEM_FAIL");
                break;
            case ResCode.WRONG_ORDER:
                Debug.Log($"PacketMaker::HandleSelectCharResponse : WRONG_ORDER");
                break;
            default:
                Debug.Log($"PacketMaker::HandleSelectCharResponse : Undefined Code");
                break;
        }
    }


    /// <summary>
    /// ���̵��� ��û�� ���� ����޽����� ó���մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleMoveMapResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                Debug.Log($"PacketMaker::HandleMoveMapResponse : SUCCESS!");

                try
                {
                    PlayerCharacter.MoveMapParameter param = JsonUtility.FromJson<PlayerCharacter.MoveMapParameter>(reqmsg_.msg);

                    UserData.instance.SetMapCode(param.MapCode);

                    ItemObject.InitDictionary();

                    SceneManager.LoadScene(param.MapCode.ToString());
                }
                catch(Exception e)
                {
                    Debug.Log($"PacketMaker::HandleMoveMapResponse : {e.Message}, {reqmsg_.msg}");
                    return;
                }

                break;
            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleMoveMapResponse : WRONG_PARAM");
                break;

            case ResCode.WRONG_ORDER:
                Debug.Log($"PacketMaker::HandleMoveMapResponse : WRONG_ORDER");
                break;
            case ResCode.UNDEFINED:
                Debug.Log($"PacketMaker::HandleMoveMapResponse : UNDEFINED");
                break;
            default:
                Debug.Log($"PacketMaker::HandleMoveMapResponse : Undefined Code");
                break;
        }

        return;
    }


    /// <summary>
    /// �������� ��û�� ���� ����޽����� ó���մϴ�. <br/>
    /// ���������� ��� ������ �� �� ���� ��쿡�� ��û�ϹǷ� <br/>
    /// �ش� ����޽����� �Դٸ� ShopPanel�� ������ ��� �ݹ��Լ��� ȣ���մϴ�.
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleGetSalesListResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;
        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                NonePlayerCharactor.GetSalesListParam stParam = JsonUtility.FromJson<NonePlayerCharactor.GetSalesListParam>(reqmsg_.msg);

                ShopPanel.AddData(stParam.NPCCode, resmsg_.Msg);

                NonePlayerCharactor.CallBackAfterNetworkResponse(stParam.NPCCode);

                break;
            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleGetSalesList : WRONG_PARAM");
                break;
            case ResCode.SYSTEM_FAIL:
                Debug.Log($"PacketMaker::HandleGetSalesList : SYSTEM_FAIL");
                break;
            default:
                Debug.Log($"PacketMaker::HandleGetSalesList : Undefined Code.");
                break;
        }
    }

    /// <summary>
    /// ����뵿�� ��û�̰� 10000 �����̴ϱ� ���� �ۼ���
    /// </summary>
    /// <param name="reqmsg_"></param>
    /// <param name="resmsg_"></param>
    /// <returns></returns>
    private async Task HandleSetGoldResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                InventoryPanel.Instance.SetGold(10000);
                break;

            case ResCode.REQ_FAIL:
                Debug.Log($"PacketMaker::HandleSetGoldResponse : REQ_FAIL");
                break;

            default:
                Debug.Log($"PacketMaker::HandleSetGoldResponse : Undefined rescode");
                break;
        }
    }

    /// <summary>
    /// ������ ���� ��û�� ���� ����޽����� ó��. <br/>
    /// �κ��丮 ������ �����Ѵ�. <br/>
    /// UserData�� InvenJstr�� ����, InventoryPanel�� ������ ����. <br/>
    /// ������ �޽����� Gold�� ó�� (CharInfo).
    /// </summary>
    /// <param name="reqmsg_">��û�޽���</param>
    /// <param name="resmsg_">����޽���</param>
    /// <returns></returns>
    private async Task HandleBuyItemResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;
        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                InventoryPanel.InitInvenInfo(resmsg_.Msg);

                int charcode = UserData.instance.GetSelectedChar();

                UserData.instance.RenewInvenJstr(charcode, resmsg_.Msg);

                Debug.Log($"PacketMaker::HandleBuyItemResponse : SUCCESS!!");

                break;
            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleBuyItemResponse : WRONG_PARAM");
                break;
            case ResCode.WRONG_ORDER:
                Debug.Log($"PacketMaker::HandleBuyItemResponse : WRONG_ORDER");
                break;
            case ResCode.REQ_FAIL:
                TextMessage.CreateTextPanel("���ſ� �����߽��ϴ�. ������â�� ���³� �������� Ȯ���ϼ���.");
                break;
            default:

                break;
        }
    }

    private async Task HandleGetObjectResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                ItemObject.GetObjectParam param = JsonUtility.FromJson<ItemObject.GetObjectParam>(reqmsg_.msg);

                ItemObject.GetItem(param.ItemIdx);

                Debug.Log($"PacketMaker::HandleGetObjectRes : SUCCESS!");

                break;
            case ResCode.REQ_FAIL:
                Debug.Log($"PacketMaker::HandleGetObjectRes : REQ_FAIL");
                break;
            case ResCode.MODIFIED_MAPCODE:
                Debug.Log($"PacketMaker::HandleGetObjectRes : MODIFIED_MAPCODE");
                break;
            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleGetObjectRes : WRONG_PARAM");
                break;
            default:
                break;
        }
    }

    private async Task HandleDropObjectResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask; 
        switch (resmsg_.ResCode)
        {
            case ResCode.SUCCESS:
                DropItemPanel.DropItemParam stParam = JsonUtility.FromJson<DropItemPanel.DropItemParam>(reqmsg_.msg);
                InventoryPanel.Remove(stParam.SlotIdx, stParam.Count);
                
                // ItemObject.CreateItem()�� InfoMsg�� ���� ó��.

                // �ٽ� �����ϵ��� ����
                InventoryPanel.SetInteractable(true);

                break;
            case ResCode.REQ_FAIL:
                Debug.Log($"PacketMaker::HandleDropObjectResponse : REQ_FAIL");
                break;
            case ResCode.MODIFIED_MAPCODE:
                Debug.Log($"PacketMaker::HandleDropObjectResponse : MODIFIED_MAPCODE");
                break;
            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleDropObjectResponse : WRONG_PARAM");
                break;
            case ResCode.UNDEFINED:
                Debug.Log($"PacketMaker::HandleDropObjectResponse : UNDEFINED");
                break;
            case ResCode.SYSTEM_FAIL:
                Debug.Log($"PacketMaker::HandleDropObjectResponse : SYSTEM_FAIL");
                break;
            default:
                break;
        }
    }

    private async Task HandleSwapInventory(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;

        switch (resmsg_.ResCode)
        { 
            case ResCode.SUCCESS:
                InventoryPanel.SwapInvenParam stParam = JsonUtility.FromJson<InventoryPanel.SwapInvenParam>(reqmsg_.msg);

                InventoryPanel.HandleSwapInventory(stParam.Idx1, stParam.Idx2);
                break;
            case ResCode.SYSTEM_FAIL:
                Debug.Log($"PacketMaker::HandleSwapInventory : SYSTEM_FAIL");
                break;
            case ResCode.WRONG_PARAM:
                Debug.Log($"PacketMaker::HandleSwapInventory : WRONG_PARAM");
                break;
            case ResCode.WRONG_ORDER:
                Debug.Log($"PacketMaker::HandleSwapInventory : WRONG_ORDER");
                break;
            default:
                break;
        }
    }

    /// <summary>
    /// �������� ������ ĳ���͵��� ��ġ���� ó��.
    /// </summary>
    /// <param name="resmsg_"></param>
    private void HandlePosInfo(ResMessage resmsg_)
    {
        // ���� �ʿ�
        return;
    }

    /// <summary>
    /// �������� ������ ä�� ���� ó��. <br/>
    /// ChatPanel�� ���ڿ� �߰�.
    /// </summary>
    /// <param name="resmsg_">�����޽���</param>
    private void HandleChat(ResMessage resmsg_)
    {
        ChatPanel.Instance.AddString(resmsg_.Msg);
        return;
    }

    public class CreateObjectInfo
    {
        public uint ObjectIdx;
        public int ItemCode;
        public long ExTime;
        public int Count;
        public float PosX;
        public float PosY;
    }

    private void HandleCreationObject(ResMessage resmsg_)
    {
        CreateObjectInfo res = JsonUtility.FromJson<CreateObjectInfo>(resmsg_.Msg);

        Debug.Log($"PacketMaker::HandleCreationObject : {res.PosX}, {res.PosY}");

        ItemObject.CreateItem(res.ObjectIdx, res.ItemCode, res.ExTime, res.Count, new Vector3(res.PosX, res.PosY, 0f));

        return;
    }

    public class ObtainedObjectInfo
    {
        public uint ObjectIdx;
        public int CharCode;
    }

    private void HandleObtainedObject(ResMessage resmsg_)
    {
        Debug.Log($"PacketMaker::HandleObtainedObject");
        ObtainedObjectInfo res = JsonUtility.FromJson<ObtainedObjectInfo>(resmsg_.Msg);

        // �ڽ��� �Ծ���. (HandleGetObject���� ó��)
        if(res.CharCode == UserData.instance.GetSelectedChar())
        {
            return;
        }

        ItemObject.SetObtained(res.ObjectIdx, res.CharCode);

        return;
    }

    public class DiscardObjectInfo
    {
        public uint ObjectIdx;
    }

    private void HandleDiscardObject(ResMessage resmsg_)
    {
        Debug.Log($"PacketMaker::HandleDiscardObject");
        DiscardObjectInfo res = JsonUtility.FromJson<DiscardObjectInfo>(resmsg_.Msg);

        ItemObject.Discard(res.ObjectIdx);

        return;
    }
}
