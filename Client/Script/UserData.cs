using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Threading.Tasks;
using static PacketMaker;
using UnityEngine.SceneManagement;


/// <summary>
/// ������� ������ �����ϴ� Ŭ����.
/// CharList, CharInfo, CharInvenJSONstr�� �����Ѵ�.
/// ���õ� ĳ���Ϳ� �°� ������ �ҷ��� �� �ִ�.
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
    /// CharList������ �ʱ�ȭ�ϸ鼭 CharInfo�� ������ ��û�Ѵ�. <br/>
    /// �ش� �޼ҵ嶧���� PacketMaker�� _resHandleFuncs�� async�޼ҵ尡 �ȴ�. (���� �ƽ�..) 
    /// </summary>
    /// <param name="jsonstr">CharList JSON���ڿ�</param>
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
    /// CreateChar �� ��ȯ������ �� Charinfo jstr�� �״�� �ִ´�.
    /// CharInfo�� �Ľ��Ͽ� �����͸� ó���Ѵ�.
    /// </summary>
    /// <param name="jsonstr">CharInfo JSON ���ڿ�</param>
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
    /// �Ķ���ʹ� CharInfo�� JSONSTR.
    /// 
    /// �ε����� ����Ͽ� ��� �߰� �� ����.
    /// ���� ���� �������� �� ����. ����.
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
    /// �κ��丮 ������ ���� JSON ���ڿ��� �޾� �����Ѵ�.
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
    /// �κ��丮 ������ ���� JSON���ڿ��� �޾� �����Ѵ�.
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
    /// ������ CharInfo�� ��û�Ѵ�.
    /// </summary>
    /// <param name="charNo_">ĳ�����ڵ�</param>
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
    /// �κ��丮 ������ ������ ��û�Ѵ�.
    /// </summary>
    /// <param name="charNo_">ĳ�����ڵ�.</param>
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
    /// ���ĳ���Ͱ� CharInfo�� �����ߴ��� Ȯ���ϴ� �Լ�. <br/>
    /// LoginScene���� SelectCharScene���� �Ѿ�� ���� Ȯ���Ѵ�.
    /// </summary>
    /// <returns>��� CharInfo�� �����ߴ��� ����</returns>
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
    /// SelectChar�� ������ �� �ش� ĳ������ �κ��丮������ �޾Ҵ��� Ȯ���ϴ� �Լ�. <br/>
    /// ���� �������� ���ߴٸ� ���û�Ѵ�. <br/>
    /// ������ ������ �ݺ����� ���� �񵿱����Ѵ�.
    /// </summary>
    /// <param name="charcode_"></param>
    /// <returns></returns>
    public async Task WaitUntilInvenLoaded(int charcode_)
    {
        if(!_CharInvenJstr.ContainsKey(charcode_))
        {
            // Ȥ�� �����Ǿ��� ���� ������ ���û�Ѵ�.
            await ReqCharInven(charcode_);
        }

        while (!_CharInvenJstr.ContainsKey(charcode_))
        {
            await Task.Delay(100);
        }

        return;
    }

    /// <summary>
    /// CharCode�� �ƴ� index�� �Ű������� �޴´�.
    /// Slot���� ��� ���Կ� � �����͸� �־���ϴ��� ��Ȯ�� �����ϱ� ����
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
    /// ���õ� ĳ���Ͱ� ���� �� �ش� ĳ������ ������ �������� �Լ�.
    /// </summary>
    /// <returns></returns>
    public CharInfo GetCharInfo()
    {
        return _CharInfos[_SelectedCharNo];
    }

    /// <summary>
    /// �κ��丮������ ���� JSON���ڿ� ��û
    /// </summary>
    /// <param name="charcode_"></param>
    /// <returns>string.Empty : �ش��ϴ� ���� ����. <br/>string : InvenJstr</returns>
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
    /// CharCode�� �ƴ� index�� �Ű������� �޴´�. <br/>
    /// CharSlot���� ��� ������ � CharInfo�� ����Ǵ����� ���� �ǵ��� ����� ����
    /// </summary>
    /// <param name="idx_"></param>
    public int SetSelectedChar(int idx_)
    {
        return _SelectedCharNo = _charlist[idx_];
    }


    /// <summary>
    /// ���� ���õ� ĳ������ CharCode�� �����Ѵ�.
    /// </summary>
    /// <returns>���õ� ĳ������ CharCode</returns>
    public int GetSelectedChar()
    {
        return _SelectedCharNo;
    }

    /// <summary>
    /// ĳ���� ���ÿ�û�� ���� ����޽����� ó���� ���Ǵ� �Լ� <br/>
    /// ĳ���� ���ÿ� ������ ��� ���Ӿ��� �����ϸ� ���õ� ĳ���������� �ٽ� Ȯ���ϰ� ����ȯ�Ѵ�.
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
    /// CharInfo�� ���ڵ带 �����Ѵ�.
    /// </summary>
    /// <returns></returns>
    /// <exception cref="CustomException.CantGetMapCodeException">CharInfo�� ã�� �� ����</exception>
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
    /// CharInfo�� ���ڵ带 �����Ѵ�.
    /// </summary>
    /// <param name="mapcode"></param>
    /// <exception cref="CustomException.CantGetMapCodeException">CharInfo�� ã�� �� ����</exception>
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
