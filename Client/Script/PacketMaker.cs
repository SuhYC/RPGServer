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
/// 서버로부터 수신한 응답을 ResHandler에서 적절히 가공해 도착한 데이터를 바탕으로 클라이언트에 반영하는 클래스이다.
/// 서버에 특정 요청을 하기 위한 ReqMessage를 작성하는 클래스이기도 하다.
/// 
/// 송신에 사용되는 메시지의 포맷은
/// [size]ReqJsonStr 이며
/// 
/// ReqJsonStr은 ReqType, ReqNo, Msg로 구성되어 있다.
/// ReqType은 요청하는 동작이나 데이터의 번호이며
/// ReqNo는 클라이언트에서 적절한 처리를 위한 요청번호이다. 서버는 처리가 끝난 요청의 요청번호를 그대로 재전송한다.
/// Msg는 ReqType에 맞는 파라미터로 구성된 JsonStr이다.
/// 
/// 수신에 사용되는 메시지의 포맷은
/// [size]ResJsonStr 이며
/// 
/// ResJsonStr은 ReqNo, ResCode, Msg로 구성되어 있다.
/// ReqNo는 ReqMessage에 입력되었던 요청번호를 그대로 전송한다. 만약 요청한 정보는 아니고 전파된 정보라면 ReqNo가 0으로 수신된다.
/// ResCode는 처리결과의 정보를 담는 Enum값이다.
/// Msg는 Optional Message이며 데이터를 요청한 경우에 해당 데이터를 Msg에 담아 서버가 전송해준다.
/// 
/// 
/// 수신한 응답으로부터 ResHandler가 [size]헤더를 제거하고 ResJsonStr만 HandleServerResponse의 매개변수로 호출하게 된다.
/// ResJsonStr을 ResMessage로 역직렬화하여 ReqNo가 0인지 확인하고 0이라면 ResCode를 통해 어떤 전파정보인지 판단하여 hashtable에 저장된 함수를 호출한다.
/// ReqNo가 0이 아니라면 요청했던 메시지 리스트에서 ReqNo를 찾아 어떤 요청이었는지 확인하고 hashtable에 저장된 함수를 호출해 결과를 클라이언트에 반영한다.
/// 
/// ReqNo를 굳이 사용하는 이유는 각각의 요청이 서버 <-> 클라이언트를 오갈때는 TCP의 순서보장에 의해 순서가 바뀌지 않지만
/// 서버에서 처리하는 과정에서는 처리시간에 의해 충분히 순서가 변경될 수 있으므로 어떤 요청에 대한 응답인지 확인할 방법이 필요하다.
/// 
/// 송신에 사용할 메시지는 ToReqMessage를 통해 작성하고, NetworkManaer에 SendMsg를 통해 전송한다.
/// </summary>
public class PacketMaker : MonoBehaviour
{
    // singleton
    private static PacketMaker _instance;

    private uint _ReqNo = 1; // 0은 클라이언트의 요청과 관계없는 메시지이다. (전파메시지)
    private SortedDictionary<uint, ReqMessage> _msgs;

    // 서버로부터 받은 결과를 각 유형에 맞게 분기하기 위함
    public delegate Task ResHandleFunc(ReqMessage reqmsg_, ResMessage resmsg_);
    Hashtable _resHandleFuncs;
    public delegate void InfoHandleFunc(ResMessage resmsg_);
    Hashtable _infoHandleFuncs;

    public enum ReqType
    {
        SIGNIN,                     // 로그인
        SIGNUP,                     // 회원가입
        MODIFY_PW,                  // 비밀번호 변경
        GET_CHAR_LIST,              // 해당 유저의 캐릭터 코드 리스트 요청 (로그인 이후 사용해야함)
        GET_CHAR_INFO,              // 특정 캐릭터 코드에 맞는 캐릭터 정보 요청
        RESERVE_CHAR_NAME,          // 캐릭터 생성 시 닉네임을 입력할 경우 미리 예약을 건 후, 가능한 닉네임인 경우 사용할 지 여부를 선택한다.
        CANCEL_CHAR_NAME_RESERVE,   // RESERVE_CHAR_NAME으로 예약한 닉네임을 취소한다.
        CREATE_CHAR,                // 캐릭터 생성 결정. DB에 캐릭터 정보를 기록한다.
        SELECT_CHAR,                // 해당 캐릭터 접속
        PERFORM_SKILL,              // 스킬 사용
        GET_OBJECT,                 // 맵에 존재하는 아이템 습득
        BUY_ITEM,                   // 상점 이용
        DROP_ITEM,                  // 인벤토리의 아이템을 버리고 맵에 생성
        USE_ITEM,                   // 인벤토리의 아이템 사용
        MOVE_MAP,                   // 맵 이동 요청
        CHAT_EVERYONE,              // 해당 맵의 모든 인원에게 채팅 (다른 종류의 채팅은 더 생각해보자.)
        GET_INVEN,                  // 인벤토리 정보 요청
        GET_SALESLIST,              // 특정 npc의 판매목록 요청
        DEBUG_SET_GOLD,             // 디버깅용. 골드 설정
        SWAP_INVENTORY,             // 인벤토리 슬롯 2개의 정보를 교환.

        POS_INFO,                   // 캐릭터의 위치, 속도 등에 대한 정보를 업데이트하는 파라미터
        LAST = POS_INFO				// enum class가 수정되면 마지막 원소로 지정할 것
    }

    public enum ResCode
    {
        // -----유저의 요청에 대한 응답-----

        SUCCESS,
        WRONG_PARAM,                    // 요청번호와 맞지 않는 파라미터
        SYSTEM_FAIL,                    // 시스템의 문제
        SIGNIN_FAIL,                    // ID와 PW가 맞지 않음
        SIGNIN_ALREADY_HAVE_SESSION,    // 이미 로그인된 계정
        SIGNUP_FAIL,                    // ID규칙이나 PW규칙이 맞지 않음
        SIGNUP_ALREADY_IN_USE,          // 이미 사용중인 ID
        WRONG_ORDER,                    // 요청이 상황에 맞지 않음
        MODIFIED_MAPCODE,               // 해당 요청이 들어올 때의 맵코드와 현재 서버가 가지고 있는 해당 유저의 맵코드가 상이함.
        REQ_FAIL,                       // 현재 조건에 맞지 않아 실행이 실패함 (이미 사라진 아이템 등..)
        UNDEFINED,                      // 알수없는 오류

        // -----이하 해당 유저의 요청에 의해 전달된 정보가 아닌 정보전달 -----
        // (채팅, 맵변화, 다른플레이어 상호작용...)

        SEND_INFO_POS,                  // 타 플레이어의 위치정보
        SEND_INFO_PERFORM_SKILL,        // 타 플레이어의 스킬 발동 정보
        SEND_INFO_GET_DAMAGE,           // 타 플레이어의 피격 정보
        SEND_INFO_MONSTER_DESPAWN,      // 해당 맵의 몬스터 사망 정보
        SEND_INFO_MONSTER_GET_DAMAGE,   // 해당 맵의 몬스터 피격 정보
        SEND_INFO_MONSTER_CREATED,      // 해당 맵의 몬스터 생성 정보
        SEND_INFO_CHAT_EVERYONE,        // 해당 맵의 모두에게 채팅
        SEND_INFO_OBJECT_CREATED,       // 해당 맵의 아이템 생성 정보
        SEND_INFO_OBJECT_DISCARDED,     // 해당 맵의 아이템 소멸 정보
        SEND_INFO_OBJECT_OBTAINED		// 해당 맵의 아이템 타인에 의한 습득정보 (자신의 습득은 Req에 의한 SUCCESS로 처리)
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
    /// 인스턴스를 초기화합니다. <br/>
    /// 각각의 요청에 대한 응답메시지 처리함수를 hashtable에 담는 과정을 포함합니다. <br/>
    /// 요청이 아닌 서버에서 전파하는 메시지 처리함수도 hashtable에 따로 담습니다.
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
    /// 요청할 동작에 대해 ReqMsg를 만듭니다. <br/>
    /// 리턴값을 NetworkManager.instance.SendMsg()에 넣으면 됩니다.
    /// </summary>
    /// <param name="type_">요청할 동작의 enum을 입력합니다.</param>
    /// <param name="msg_">요청할 동작의 파라미터를 입력합니다.</param>
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
            Debug.Log("PacketMaker::ToMessage : ReqNo 충돌");
        }
        catch (Exception e)
        {
            Debug.Log($"PacketMaker::ToMessage : {e.Message}");
        }

        //Debug.Log($"PacketMaker::ToMessage : {_ReqNo}, {_msgs[_ReqNo].ToString()}");

        return JsonUtility.ToJson(_msgs[_ReqNo]);
    }


    /// <summary>
    /// 수신메시지의 payload를 받아 처리합니다.
    /// 
    /// ReqNo를 확인하여 0이라면 클라이언트에서 요청한 것이 아닌 서버에서 일방적으로 전파하는 메시지입니다.
    /// 0이라면 ResCode를 통해 어떤 전파메시지인지 확인하여 처리하고
    /// 0이 아니라면 ReqNo를 조회하여 어떤 요청을 했었는지 확인하고 처리합니다.
    /// </summary>
    /// <param name="msg_">ResMessage의 JSONStr</param>
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
    /// 회원가입 요청에 대한 응답을 처리합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
    /// 로그인 요청에 대한 응답을 처리합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
    /// [미사용]
    /// 
    /// SignIn과 독립적으로 사용하려고 했으나
    /// SignIn 후에 반드시 요청하여야 하므로 HandleSignIn 내에서 해결.
    /// ReqCharList만 독립적으로 사용할지는 모르겠음.
    /// </summary>
    /// <returns></returns>
    private async Task ReqCharList()
    {
        string str = ToReqMessage(ReqType.GET_CHAR_LIST, "");
        await NetworkManager.Instance.SendMsg(str);
        return;
    }

    /// <summary>
    /// [미사용]
    /// </summary>
    /// <param name="reqmsg_"></param>
    /// <param name="resmsg_"></param>
    /// <returns></returns>
    private async Task HandleGetCharListResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;
        Charlist charlist = JsonUtility.FromJson<Charlist>(resmsg_.Msg);

        // ... 담아둘 스크립트를 구성해서 업데이트 하는 정도만 하자.

        return;
    }


    /// <summary>
    /// CharInfo를 요청하는 메시지의 응답메시지를 처리하는 함수. <br/>
    /// 일반적으로 로그인 이후에 CharInfo를 요청하게 되므로 <br/>
    /// UserData에 저장하고 현재 씬이 LoginScene이라면 UserData에 저장해야하는 데이터가 전부 저장되었는지 확인하고 씬을 변경합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
    /// 캐릭터 생성을 위해 닉네임을 예약하는 요청에 대한 응답메시지를 처리하는 함수입니다. <br/>
    /// 
    /// 생성가능하다고 응답한 경우 해당 닉네임으로 생성하는 것이 맞는지 확인하는 패널을 생성합니다. <br/>
    /// 예약에 실패한 경우 각각의 상황에 맞는 텍스트패널을 출력하여 사용자가 확인할 수 있게 합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
                TextMessage.CreateTextPanel("생성할 수 없는 닉네임입니다.");
                Debug.Log($"PacketMaker::HandleReserveNicknameResponse : REQ_FAIL");
                break;

            default:
                Debug.Log($"PacketMaker::HandleReserveNicknameResponse : Undefined ResCode.");
                break;
        }
    }


    /// <summary>
    /// 예약한 닉네임에 대해 생성을 요청한 것에 대한 응답메시지를 처리하는 함수입니다. <br/>
    /// 
    /// 생성에 성공했다면 UserData의 CharList에 추가하며 캐릭터 선택화면으로 돌아갑니다. <br/>
    /// 생성에 실패했다면 어떤 이유로 실패했는지 텍스트메시지를 생성합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
                TextMessage.CreateTextPanel("생성에 실패했습니다.");
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
    /// 캐릭터 선택창으로 돌아갑니다.
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
    /// 캐릭터 닉네임 예약을 취소하는 요청의 응답메시지를 처리합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
    /// 인벤토리 정보를 요청한 것에 대한 응답메시지를 처리합니다.
    /// UserData에 인벤토리 정보 JSONStr을 저장합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
    /// 캐릭터 선택 요청에 대한 응답메시지를 처리합니다. <br/>
    /// 
    /// 캐릭터 선택 씬이 아니라면 그냥 종료합니다. <br/>
    /// 해당 캐릭터의 인벤토리 정보가 수신되지 않았다면 인벤토리 정보를 요청하고 대기하는 함수를 호출합니다. <br/>
    /// 인벤토리 정보도 정상적으로 수신되었다면 인벤토리패널에 정보를 담고 씬을 전환합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
    /// 맵이동의 요청에 대한 응답메시지를 처리합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
    /// 상점정보 요청에 대한 응답메시지를 처리합니다. <br/>
    /// 상점정보가 없어서 상점을 열 수 없는 경우에만 요청하므로 <br/>
    /// 해당 응답메시지가 왔다면 ShopPanel에 정보를 담고 콜백함수를 호출합니다.
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
    /// 디버깅동작 요청이고 10000 고정이니까 대충 작성함
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
    /// 아이템 구매 요청에 대한 응답메시지를 처리. <br/>
    /// 인벤토리 정보를 갱신한다. <br/>
    /// UserData의 InvenJstr도 갱신, InventoryPanel의 정보도 갱신. <br/>
    /// 별도의 메시지로 Gold도 처리 (CharInfo).
    /// </summary>
    /// <param name="reqmsg_">요청메시지</param>
    /// <param name="resmsg_">응답메시지</param>
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
                TextMessage.CreateTextPanel("구매에 실패했습니다. 아이템창의 상태나 소지금을 확인하세요.");
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
                
                // ItemObject.CreateItem()은 InfoMsg로 따로 처리.

                // 다시 동작하도록 설정
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
    /// 서버에서 전파한 캐릭터들의 위치정보 처리.
    /// </summary>
    /// <param name="resmsg_"></param>
    private void HandlePosInfo(ResMessage resmsg_)
    {
        // 구현 필요
        return;
    }

    /// <summary>
    /// 서버에서 전파한 채팅 정보 처리. <br/>
    /// ChatPanel에 문자열 추가.
    /// </summary>
    /// <param name="resmsg_">정보메시지</param>
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

        // 자신이 먹었다. (HandleGetObject에서 처리)
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
