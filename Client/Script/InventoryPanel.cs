using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using TMPro;

/// <summary>
/// 인벤토리 창에 부착되는 스크립트.
/// 
/// 인벤토리 창 하위 하이어라키에 슬롯들이 존재하며
/// 인벤토리 정보를 서버로부터 받아 UserData클래스에 JSONStr형태로 저장해둔 뒤
/// 불러와 초기화한다.
/// 
/// NeedToRenew : 서버로부터 받은 새로운 정보가 있음. 초기화가 필요하므로 인벤창이 초기화 가능할 때 초기화한다.
/// g_Inven : UserData클래스에서 받아온 JSONStr을 역직렬화하여 저장할 변수.
/// Content : Slot들이 자식들로 부착되는 트랜스폼.
/// GoldText : 인벤토리 하단에 표시되는 현재 보유한 골드
/// </summary>
public class InventoryPanel : MonoBehaviour
{
    private static InventoryPanel instance;
    private static Inventory g_Inven = new Inventory();
    private static bool NeedToRenew;

    private Transform Content;
    private TMP_Text GoldText;

    // 인벤토리 정보를 따로 가지고 있어야된다.
    [System.Serializable]
    public class Item
    {
        public int I; // ItemCode
        public long E; // Extime
        public int C; // Count
    }

    [System.Serializable]
    public class Inventory
    {
        public Item[] Inven;

        public Item this[int index]
        {
            get { return Inven[index]; }
        }
    }


    public static InventoryPanel Instance
    {
        get { return instance; }
    }

    private void Awake()
    {
        instance = this;
        NeedToRenew = true;

        Content = transform.GetChild(1)?.GetChild(0)?.GetChild(0);

        if(Content == null)
        {
            Debug.Log($"InventoryPanel::Awake : content null ref.");
        }

        GoldText = transform.GetChild(2)?.GetChild(1)?.GetComponent<TMP_Text>();

        if(GoldText == null)
        {
            Debug.Log($"InventoryPanel::Awake : GoldText null ref.");
        }

        InitSlot();

        try
        {
            UserData.CharInfo info = UserData.instance.GetCharInfo();
            SetGold(info.Gold);
        }
        catch (Exception e)
        {
            Debug.Log($"InventoryPanel::Awake : {e.Message}");
        }
    }

    /// <summary>
    /// 인벤창이 다시 활성화 될 때 호출되어 갱신이 필요한 경우 InitSlot을 호출한다.
    /// </summary>
    private void OnEnable()
    {
        if(NeedToRenew)
        {
            InitSlot();
            NeedToRenew = false;
        }
    }

    /// <summary>
    /// 서버로부터 받아온 인벤토리 정보를 전역변수로 저장한다.
    /// 씬을 불러오기 전에 반드시 수행되어야한다.
    /// </summary>
    /// <param name="jsonstr_">Inventory의 JSON문자열</param>
    public static void InitInvenInfo(string jsonstr_)
    {
        Debug.Log($"InventoryPanel::InitInvenInfo : {jsonstr_}");
        g_Inven = JsonUtility.FromJson<Inventory>(jsonstr_);

        if(instance == null)
        {
            NeedToRenew = true;
            return;
        }
        else if(instance.gameObject.activeInHierarchy)
        {
            instance.InitSlot();
        }
        else
        {
            NeedToRenew = true;
        }

        return;
    }

    /// <summary>
    /// 전역변수로 가지고 있는 정보를 현재의 인벤토리 인스턴스에 반영한다.
    /// 씬이 불러와지기 전에는 호출하면 안된다.
    /// </summary>
    public void InitSlot()
    {
        for(int i = 0; i < Content.childCount; i++)
        {
            Transform child = Content.GetChild(i);
            InventorySlot slot = child.GetComponent<InventorySlot>();

            if(slot == null)
            {
                Debug.Log($"InventoryPanel::InitSlot[{i}] : slot null ref");
            }

            if(g_Inven == null)
            {
                g_Inven = new Inventory();
            }

            try
            {
                slot.Init(g_Inven[i].I, g_Inven[i].C, g_Inven[i].E != 0); // ItemCode, Count, Extime
            }
            catch(Exception e)
            {
                Debug.Log($"InventoryPanel::InitSlot[{i}] : {e.Message}");
            }            
        }
    }

    /// <summary>
    /// 인벤창 하단의 골드 텍스트를 수정한다.
    /// </summary>
    /// <param name="gold">소지한 골드</param>
    public void SetGold(int gold)
    {
        GoldText.text = gold.ToString();
    }
}
