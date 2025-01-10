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

    private uint _ReqNo = 1; // 0은 클라이언트의 요청과 관계없는 메시지이다. (전파메시지)
    private SortedDictionary<uint, ReqMessage> _msgs;

    // 서버로부터 받은 결과를 각 유형에 맞게 분기하기 위함
    public delegate void ResHandleFunc(ReqMessage reqmsg_, ResMessage resmsg_);
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
            Debug.Log("PacketMaker::ToMessage : ReqNo 충돌");
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
