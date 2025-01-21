using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Threading.Tasks;
using static PacketMaker;

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
    private class CharListMsg
    {
        public string Type;
        public int[] CharList;
    }

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
    }

    public async Task InitCharList(string jsonstr)
    {
        CharListMsg msg = JsonUtility.FromJson<CharListMsg>(jsonstr);
        _charlist.array = msg.CharList;

        //_charlist.PrintList();

        for(int i = 0; i < _charlist.array.Length; i++)
        {
            await ReqCharInfo(_charlist[i]);
        }

        return;
    }

    /// <summary>
    /// 파라미터는 CharInfo의 JSONSTR.
    /// 
    /// 인덱서를 사용하여 요소 추가 및 수정.
    /// 기존 값을 지워버릴 수 있음. 주의.
    /// </summary>
    /// <param name="jsonstr"></param>
    public void InitCharInfo(string jsonstr)
    {
        CharInfo charInfo = JsonUtility.FromJson<CharInfo>(jsonstr);

        Debug.Log($"UserData::InitCharInfo : charName : {charInfo.CharName}, charNo : {charInfo.CharNo}, classcode : {charInfo.ClassCode}" +
            $", level : {charInfo.Level}, Experience : {charInfo.Experience}, statpoint : {charInfo.StatPoint}, health : {charInfo.HealthPoint}" +
            $", mana : {charInfo.ManaPoint}, curHealth : {charInfo.CurrentHealth}, curMana : {charInfo.CurrentMana}, str : {charInfo.Strength}" +
            $", dex : {charInfo.Dexterity}, int : {charInfo.Intelligence}, men : {charInfo.Mentality}, gold : {charInfo.Gold}, map : {charInfo.LastMapCode}");

        _CharInfos[charInfo.CharNo] = charInfo;

        return;
    }

    public class GetCharInfoParam
    {
        public int Charcode;
        public GetCharInfoParam(int charcode)
        {
            Charcode = charcode;
        }
    }

    private async Task ReqCharInfo(int charNo_)
    {
        GetCharInfoParam param = new GetCharInfoParam(charNo_);
        string str = JsonUtility.ToJson(param);
        str = PacketMaker.instance.ToReqMessage(ReqType.GET_CHAR_INFO, str);
        await NetworkManager.Instance.SendMsg(str);

        return;
    }

    public bool IsCompletelyLoaded()
    {
        Debug.Log($"UserData::IsCompletelyLoaded : Infos.Count : {_CharInfos.Count}, list.length : {_charlist.array.Length}");

        if(_CharInfos.Count == _charlist.array.Length)
        {
            return true;
        }

        return false;
    }
}
