using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// �������� ������ �� �ִ� �����гο� �����Ǵ� ��ũ��Ʈ�̴�. <br/>
/// npc�� Ŭ���Ͽ��� �� �ش� npccode�� �´� saleslist�� ���� ��� �ش� �г��� �����ȴ�. <br/>
/// ���� � npc�� ������ �����ִ��� npccode�� �����Ѵ�.
/// 
/// data : int npccode, string SalesListJstr �� Ű ��� ������ �̷���� Dictionary.
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
    /// �����κ��� ������ npccode-SalesListJstr ���� static Dictionary�� �����Ѵ�.
    /// </summary>
    /// <param name="npccode"></param>
    /// <param name="jsonstr">SalesListJSONStr</param>
    public static void AddData(int npccode, string jsonstr)
    {
        data.TryAdd(npccode, jsonstr);
    }

    /// <summary>
    /// �ش� npccode�� �´� SalesListJstr�� static Dictionary�� �ִ��� Ȯ���Ѵ�.
    /// </summary>
    /// <param name="npccode"></param>
    /// <returns></returns>
    public static bool HasData(int npccode)
    {
        return data.ContainsKey(npccode);
    }


    /// <summary>
    /// �г� �ν��Ͻ��� ���Ű��ɸ���� �ʱ�ȭ�Ѵ�. <br/>
    /// static Dictionary���� Jstr�� ������ ������ȭ �� ������ ��Ҹ� CreateSlot�� ��û�Ѵ�.
    /// </summary>
    /// <param name="npccode"></param>
    public void InitSalesList(int npccode)
    {
        if(m_NowNPCCode == npccode)
        {
            return;
        }

        m_NowNPCCode = npccode;

        foreach(Transform child in BuyPanel) // ���� ���� ����.
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
            CreateSlot(item.ItemCode, "�ӽø��", item.Price);
        }
    }


    /// <summary>
    /// ���Ÿ���� ������ �����Ѵ�. <br/>
    /// ����� �������� �̸������� �������� �������� �ʴ´�. �ӽ� ������� ó�������� ���߿� ��������.
    /// </summary>
    /// <param name="code">�������� �ڵ�</param>
    /// <param name="name">�������� �̸�</param>
    /// <param name="price">�������� ����</param>
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
