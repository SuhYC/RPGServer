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
        SEND_INFO_MONSTER_CREATED,		// 해당 맵의 몬스터 생성 정보
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
            Debug.Log("PacketMaker::ToMessage : ReqNo 충돌");
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

    private async Task HandleGetCharListResponse(ReqMessage reqmsg_, ResMessage resmsg_)
    {
        await Task.CompletedTask;
        Charlist charlist = JsonUtility.FromJson<Charlist>(resmsg_.Msg);

        // ... 담아둘 스크립트를 구성해서 업데이트 하는 정도만 하자.

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
        await Task.CompletedTask; // async Task를 넣을 필요가 있을까?
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
                TextMessage.CreateTextPanel("생성할 수 없는 닉네임입니다.");
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
                // 생성 성공 처리
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
        // 구현 필요
        return;
    }
}
