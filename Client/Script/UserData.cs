using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Threading.Tasks;
using static PacketMaker;
using UnityEngine.SceneManagement;


/// <summary>
/// 사용자의 정보를 저장하는 클래스.
/// CharList, CharInfo, CharInvenJSONstr을 저장한다.
/// 선택된 캐릭터에 맞게 정보를 불러올 수 있다.
/// </summary>
public class UserData : MonoBehaviour
{
    private static UserData _instance = null;
    public static UserData instance
    {
        get
        {
            if (_instance == null)
            {
                Debug.Log("UserData::instance : null Ref.");
                _instance = FindAnyObjectByType<UserData>();
                if (_instance == null)
                {
                    GameObject netManagerObj = new GameObject("UserData");
                    _instance = netManagerObj.AddComponent<UserData>();

                    _instance.Init();
                }
            }
            return _instance;
        }
    }

    private Charlist _charlist;
    public class CharInfo
    {
        public int CharNo;
        public string CharName;
        public int ClassCode;
        public int Level;
        public int Experience;
        public int StatPoint;
        public int HealthPoint;
        public int ManaPoint;
        public int CurrentHealth;
        public int CurrentMana;
        public int Strength;
        public int Dexterity;
        public int Intelligence;
        public int Mentality;
        public int Gold;
        public int LastMapCode;
    }

    private Dictionary<int, CharInfo> _CharInfos;
    private Dictionary<int, string> _CharInvenJstr;
    private class CharListMsg
    {
        public string Type;
        public int[] CharList;
    }

    private int _SelectedCharNo;

    void Awake()
    {
        if (_instance != null && _instance != this)
        {
            Destroy(this.gameObject);
            return;
        }
        else
        {
            _instance = this;
            Init();
        }
    }

    private void Init()
    {
        DontDestroyOnLoad(this.gameObject);
        _charlist = new Charlist();
        _CharInfos = new Dictionary<int, CharInfo>();
        _CharInvenJstr = new Dictionary<int, string>();
    }


    /// <summary>
    /// CharList정보를 초기화하면서 CharInfo를 서버에 요청한다. <br/>
    /// 해당 메소드때문에 PacketMaker의 _resHandleFuncs가 async메소드가 된다. (조금 아쉽..) 
    /// </summary>
    /// <param name="jsonstr">CharList JSON문자열</param>
    /// <returns></returns>
    public async Task InitCharList(string jsonstr)
    {
        CharListMsg msg = JsonUtility.FromJson<CharListMsg>(jsonstr);
        _charlist.array = msg.CharList;

        //_charlist.PrintList();

        for(int i = 0; i < _charlist.array.Length; i++)
        {
            await ReqCharInfo(_charlist[i]);
            await ReqCharInven(_charlist[i]);
        }

        return;
    }

    /// <summary>
    /// CreateChar 의 반환값으로 온 Charinfo jstr을 그대로 넣는다.
    /// CharInfo로 파싱하여 데이터를 처리한다.
    /// </summary>
    /// <param name="jsonstr">CharInfo JSON 문자열</param>
    public void AddCharList(string jsonstr)
    {
        CharInfo charInfo = JsonUtility.FromJson<CharInfo>(jsonstr);

        _charlist.Add(charInfo.CharNo);

        Debug.Log($"UserData::InitCharInfo : charName : {charInfo.CharName}, charNo : {charInfo.CharNo}, classcode : {charInfo.ClassCode}" +
            $", level : {charInfo.Level}, Experience : {charInfo.Experience}, statpoint : {charInfo.StatPoint}, health : {charInfo.HealthPoint}" +
            $", mana : {charInfo.ManaPoint}, curHealth : {charInfo.CurrentHealth}, curMana : {charInfo.CurrentMana}, str : {charInfo.Strength}" +
            $", dex : {charInfo.Dexterity}, int : {charInfo.Intelligence}, men : {charInfo.Mentality}, gold : {charInfo.Gold}, map : {charInfo.LastMapCode}");

        _CharInfos[charInfo.CharNo] = charInfo;

        return;
    }

    /// <summary>
    /// 파라미터는 CharInfo의 JSONSTR.
    /// 
    /// 인덱서를 사용하여 요소 추가 및 수정.
    /// 기존 값을 지워버릴 수 있음. 주의.
    /// </summary>
    /// <param name="jsonstr">CharInfo JSONStr</param>
    public void InitCharInfo(string jsonstr)
    {
        CharInfo charInfo = JsonUtility.FromJson<CharInfo>(jsonstr);

        //Debug.Log($"UserData::InitCharInfo : charName : {charInfo.CharName}, charNo : {charInfo.CharNo}, classcode : {charInfo.ClassCode}" +
        //    $", level : {charInfo.Level}, Experience : {charInfo.Experience}, statpoint : {charInfo.StatPoint}, health : {charInfo.HealthPoint}" +
        //    $", mana : {charInfo.ManaPoint}, curHealth : {charInfo.CurrentHealth}, curMana : {charInfo.CurrentMana}, str : {charInfo.Strength}" +
        //    $", dex : {charInfo.Dexterity}, int : {charInfo.Intelligence}, men : {charInfo.Mentality}, gold : {charInfo.Gold}, map : {charInfo.LastMapCode}");

        _CharInfos[charInfo.CharNo] = charInfo;

        if(InventoryPanel.Instance != null)
        {
            InventoryPanel.Instance.SetGold(charInfo.Gold);
        }

        return;
    }


    /// <summary>
    /// 인벤토리 정보에 대한 JSON 문자열을 받아 저장한다.
    /// </summary>
    /// <param name="charcode"></param>
    /// <param name="jsonstr"></param>
    public void AddInvenJstr(int charcode, string jsonstr)
    {
        if(jsonstr == string.Empty)
        {
            return;
        }

        _CharInvenJstr.TryAdd(charcode, jsonstr);

        return;
    }

    /// <summary>
    /// 인벤토리 정보에 대한 JSON문자열을 받아 갱신한다.
    /// </summary>
    /// <param name="charcode"></param>
    /// <param name="jsonstr"></param>
    public void RenewInvenJstr(int charcode, string jsonstr)
    {
        if(jsonstr == string.Empty)
        {
            return;
        }

        if (!_CharInvenJstr.ContainsKey(charcode))
        {
            return;
        }

        _CharInvenJstr[charcode] = jsonstr;
    }

    public class GetCharInfoParam
    {
        public int Charcode;
        public GetCharInfoParam(int charcode)
        {
            Charcode = charcode;
        }
    }


    /// <summary>
    /// 서버에 CharInfo를 요청한다.
    /// </summary>
    /// <param name="charNo_">캐릭터코드</param>
    /// <returns></returns>
    public async Task ReqCharInfo(int charNo_)
    {
        GetCharInfoParam param = new GetCharInfoParam(charNo_);
        string str = JsonUtility.ToJson(param);
        str = PacketMaker.instance.ToReqMessage(ReqType.GET_CHAR_INFO, str);
        await NetworkManager.Instance.SendMsg(str);

        return;
    }


    public class GetCharInvenParam
    {
        public int Charcode;

        public GetCharInvenParam(int charcode)
        {
            Charcode = charcode;
        }
    }


    /// <summary>
    /// 인벤토리 정보를 서버에 요청한다.
    /// </summary>
    /// <param name="charNo_">캐릭터코드.</param>
    /// <returns></returns>
    private async Task ReqCharInven(int charNo_)
    {
        GetCharInvenParam param = new GetCharInvenParam(charNo_);
        string str = JsonUtility.ToJson(param);
        str = PacketMaker.instance.ToReqMessage(ReqType.GET_INVEN, str);
        await NetworkManager.Instance.SendMsg(str);

        return;
    }


    /// <summary>
    /// 모든캐릭터가 CharInfo를 수신했는지 확인하는 함수. <br/>
    /// LoginScene에서 SelectCharScene으로 넘어가기 전에 확인한다.
    /// </summary>
    /// <returns>모든 CharInfo를 수신했는지 여부</returns>
    public bool IsCompletelyLoadedCharInfo()
    {
        //Debug.Log($"UserData::IsCompletelyLoaded : Infos.Count : {_CharInfos.Count}, list.length : {_charlist.array.Length}");

        if(_CharInfos.Count == _charlist.array.Length)
        {
            return true;
        }

        return false;
    }


    /// <summary>
    /// SelectChar를 수행할 때 해당 캐릭터의 인벤토리정보를 받았는지 확인하는 함수. <br/>
    /// 아직 수신하지 못했다면 재요청한다. <br/>
    /// 수신할 때까지 반복문을 돌며 비동기대기한다.
    /// </summary>
    /// <param name="charcode_"></param>
    /// <returns></returns>
    public async Task WaitUntilInvenLoaded(int charcode_)
    {
        if(!_CharInvenJstr.ContainsKey(charcode_))
        {
            // 혹시 누락되었을 수도 있으니 재요청한다.
            await ReqCharInven(charcode_);
        }

        while (!_CharInvenJstr.ContainsKey(charcode_))
        {
            await Task.Delay(100);
        }

        return;
    }

    /// <summary>
    /// CharCode가 아닌 index를 매개변수로 받는다.
    /// Slot에서 몇번 슬롯에 어떤 데이터를 넣어야하는지 정확히 인지하기 위함
    /// </summary>
    /// <param name="idx"></param>
    /// <returns></returns>
    public CharInfo GetCharInfo(int idx)
    {
        int charcode;
        try
        {
            charcode = _charlist[idx];
        }
        catch(Exception)
        {
            throw new CustomException.NotFoundInCharlistException();
        }
        return _CharInfos[charcode];
    }

    /// <summary>
    /// 선택된 캐릭터가 있을 때 해당 캐릭터의 정보를 가져오는 함수.
    /// </summary>
    /// <returns></returns>
    public CharInfo GetCharInfo()
    {
        return _CharInfos[_SelectedCharNo];
    }

    /// <summary>
    /// 인벤토리정보를 담은 JSON문자열 요청
    /// </summary>
    /// <param name="charcode_"></param>
    /// <returns>string.Empty : 해당하는 정보 없음. <br/>string : InvenJstr</returns>
    public string GetInvenJstr(int charcode_)
    {
        string str;
        if(_CharInvenJstr.TryGetValue(charcode_, out str))
        {
            return str;
        }

        return string.Empty;
    }

    /// <summary>
    /// CharCode가 아닌 index를 매개변수로 받는다. <br/>
    /// CharSlot에서 몇번 슬롯이 어떤 CharInfo와 연결되는지는 몰라도 되도록 만들기 위함
    /// </summary>
    /// <param name="idx_"></param>
    public int SetSelectedChar(int idx_)
    {
        return _SelectedCharNo = _charlist[idx_];
    }


    /// <summary>
    /// 현재 선택된 캐릭터의 CharCode를 리턴한다.
    /// </summary>
    /// <returns>선택된 캐릭터의 CharCode</returns>
    public int GetSelectedChar()
    {
        return _SelectedCharNo;
    }

    /// <summary>
    /// 캐릭터 선택요청에 대한 응답메시지를 처리에 사용되는 함수 <br/>
    /// 캐릭터 선택에 성공한 경우 게임씬에 진입하며 선택된 캐릭터정보를 다시 확인하고 씬전환한다.
    /// </summary>
    /// <param name="charno_">CharCode</param>
    public void SetSelectedCharNo(int charno_)
    {
        CharInfo tmp;
        if(!_CharInfos.TryGetValue(charno_, out tmp))
        {
            Debug.Log($"UserData::SetSelectedCharNo : invalid charno.");
            return;
        }

        _SelectedCharNo = charno_;
        return;
    }


    /// <summary>
    /// CharInfo의 맵코드를 리턴한다.
    /// </summary>
    /// <returns></returns>
    /// <exception cref="CustomException.CantGetMapCodeException">CharInfo를 찾을 수 없음</exception>
    public int GetMapCode()
    {
        CharInfo tmp;
        if(!_CharInfos.TryGetValue(_SelectedCharNo, out tmp))
        {
            throw new CustomException.CantGetMapCodeException();
        }

        return tmp.LastMapCode;
    }


    /// <summary>
    /// CharInfo의 맵코드를 수정한다.
    /// </summary>
    /// <param name="mapcode"></param>
    /// <exception cref="CustomException.CantGetMapCodeException">CharInfo를 찾을 수 없음</exception>
    public void SetMapCode(int mapcode)
    {
        CharInfo tmp;
        if(!_CharInfos.TryGetValue(_SelectedCharNo, out tmp))
        {
            throw new CustomException.CantGetMapCodeException();
        }

        tmp.LastMapCode = mapcode;
        return;
    }
}
