using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// 아이템을 구매할 수 있는 상점패널에 부착되는 스크립트이다. <br/>
/// npc를 클릭하였을 때 해당 npccode에 맞는 saleslist가 있을 경우 해당 패널이 생성된다. <br/>
/// 현재 어떤 npc의 상점을 보고있는지 npccode를 저장한다.
/// 
/// data : int npccode, string SalesListJstr 의 키 밸류 쌍으로 이루어진 Dictionary.
/// 
/// </summary>
public class ShopPanel : MonoBehaviour
{
    private static ShopPanel instance;
    private static Dictionary<int, string> data; // <npccode, SalesListJstr>
    private int m_NowNPCCode;

    public static ShopPanel Instance
    {
        get { return instance; }
    }

    private GameObject slotObj;
    private Transform BuyPanel;

    [Serializable]
    public class Item
    {
        public int ItemCode;
        public int Price;
    }

    [Serializable]
    public class SalesListWrapper
    {
        public List<Item> SalesList;
    }

    private void Awake()
    {
        m_NowNPCCode = 0;

        if(data == null)
        {
            data = new Dictionary<int, string>();
        }

        instance = this;

        slotObj = Resources.Load<GameObject>("Prefabs/UI/ShopSlot");

        if(slotObj == null )
        {
            Debug.Log($"ShopPanel::Awake : slot null ref");
        }

        BuyPanel = transform.GetChild(0)?.GetChild(0)?.GetChild(0);

        if(BuyPanel == null)
        {
            Debug.Log($"ShopPanel::Awake : buypanel transform null ref.");
        }

        gameObject.SetActive(false);
    }


    /// <summary>
    /// 서버로부터 수신한 npccode-SalesListJstr 쌍을 static Dictionary에 저장한다.
    /// </summary>
    /// <param name="npccode"></param>
    /// <param name="jsonstr">SalesListJSONStr</param>
    public static void AddData(int npccode, string jsonstr)
    {
        data.TryAdd(npccode, jsonstr);
    }

    /// <summary>
    /// 해당 npccode에 맞는 SalesListJstr이 static Dictionary에 있는지 확인한다.
    /// </summary>
    /// <param name="npccode"></param>
    /// <returns></returns>
    public static bool HasData(int npccode)
    {
        return data.ContainsKey(npccode);
    }


    /// <summary>
    /// 패널 인스턴스에 구매가능목록을 초기화한다. <br/>
    /// static Dictionary에서 Jstr을 가져와 역직렬화 후 각각의 요소를 CreateSlot을 요청한다.
    /// </summary>
    /// <param name="npccode"></param>
    public void InitSalesList(int npccode)
    {
        if(m_NowNPCCode == npccode)
        {
            return;
        }

        m_NowNPCCode = npccode;

        foreach(Transform child in BuyPanel) // 기존 슬롯 제거.
        {
            Destroy(child.gameObject);
        }

        string jsonstr;

        if (!data.TryGetValue(npccode, out jsonstr))
        {
            Debug.Log($"ShopPanel::InitSalesList : no data.");
            return;
        }

        SalesListWrapper saleslist = JsonUtility.FromJson<SalesListWrapper>(jsonstr);

        foreach(Item item in saleslist.SalesList)
        {
            CreateSlot(item.ItemCode, "임시명명", item.Price);
        }
    }


    /// <summary>
    /// 구매목록의 슬롯을 생성한다. <br/>
    /// 현재는 아이템의 이름까지는 서버에서 가져오지 않는다. 임시 명명으로 처리했으나 나중에 수정하자.
    /// </summary>
    /// <param name="code">아이템의 코드</param>
    /// <param name="name">아이템의 이름</param>
    /// <param name="price">아이템의 가격</param>
    private void CreateSlot(int code, string name, int price)
    {
        GameObject obj = Instantiate(slotObj, BuyPanel);

        if(obj == null )
        {
            Debug.Log($"ShopPanel::CreateSlot : obj null ref");
            return;
        }

        ShopSlot slot = obj.GetComponent<ShopSlot>();

        if(slot == null)
        {
            Debug.Log($"ShopPanel::CreateSlot : slot component null ref");
            return;
        }

        slot.Init(code, name, price);
        return;
    }

    public int GetNPCCode()
    {
        return m_NowNPCCode;
    }
}
