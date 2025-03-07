using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using TMPro;
using UnityEngine.EventSystems;
using UnityEngine.UIElements;
using Unity.VisualScripting;

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
public class InventoryPanel : MonoBehaviour, IDropHandler
{
    private static GameObject prefab;
    private static Canvas canvas;

    private static InventoryPanel instance;
    private static Inventory g_Inven = new Inventory();
    private static bool NeedToRenew;
    private static bool m_bIsInteractable;

    // 인벤토리 슬롯 최대 갯수
    public const int MAX_SLOT = 64;

    private Transform Content;
    private TMP_Text GoldText;

    // 인벤토리 정보를 따로 가지고 있어야된다.
    [System.Serializable]
    public class Item
    {
        // ItemCode
        public int I;

        // Extime
        public long E;

        // Count
        public int C; 
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

    public static void CreateInstance()
    {
        if(prefab == null)
        {
            prefab = Resources.Load<GameObject>("Prefabs/UI/InventoryPanel");
            if(prefab == null)
            {
                Debug.Log($"InventoryPanel::CreateInstance : prefab nullref");
                return;
            }
        }

        if(canvas == null)
        {
            canvas = FindAnyObjectByType<Canvas>();

            if(canvas == null)
            {
                Debug.Log($"InventoryPanel::CreateInstance : canvas nullref");
                return;
            }
        }

        GameObject obj = Instantiate(prefab, canvas.transform);
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

        try
        {
            UserData.CharInfo info = UserData.instance.GetCharInfo();
            SetGold(info.Gold);
        }
        catch (Exception e)
        {
            Debug.Log($"InventoryPanel::Awake : {e.Message}");
        }

        m_bIsInteractable = true;

        gameObject.SetActive(false);
    }

    private void Start()
    {
        InitSlot();
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

        SetNeedToRenew();

        return;
    }

    /// <summary>
    /// 추후 캐릭터 선택창으로 돌아가는 동작도 만들기 위해
    /// 현재 인벤토리를 jstr으로 만들어서 UserData에 저장하는 함수.
    /// 
    /// 서버에 요청해도 되지만 그건 NetworkIO를 굳이 늘리는 거 같다.
    /// </summary>
    public static void SaveInvenJstr()
    {
        int charcode = UserData.instance.GetSelectedChar();

        if(charcode == -1)
        {
            return;
        }

        string invenJstr = JsonUtility.ToJson(g_Inven);
        UserData.instance.RenewInvenJstr(charcode, invenJstr);

        return;
    }

    private static Inventory GetInventory()
    {
        int charcode = UserData.instance.GetSelectedChar();

        if(charcode == -1)
        {
            return null;
        }

        string invenJstr = UserData.instance.GetInvenJstr(charcode);

        return JsonUtility.FromJson<Inventory>(invenJstr);
    }

    public static void Add(int itemCode_, long exTime_, int count_)
    {
        if (count_ > InventorySlot.MaxSlots)
        {
            Debug.Log($"InventoryPanel::Add : count is bigger than MaxSlot");
            return;
        }

        // 깊은 복사를 위해 Jstr으로 저장한 후 다시 불러온다.
        SaveInvenJstr();
        Inventory tmp = GetInventory();

        if(tmp == null)
        {
            Debug.Log($"InventoryPanel::Add : Cant Get InvenJstr");
            return;
        }

        int remaincnt = count_;

        // 기존에 해당 아이템을 보유하고 있었다면 해당 슬롯에 우선 채운다.
        for(int i = 0; i < MAX_SLOT && remaincnt > 0; i++)
        {
            if (tmp.Inven[i] == null)
            {
                Debug.Log($"InventoryPanel::Add : slot[{i}] null ref");
                return;
            }

            if(tmp.Inven[i].I == itemCode_ && tmp.Inven[i].E == exTime_)
            {
                int available = InventorySlot.MaxSlots - tmp.Inven[i].C;

                if(available >= remaincnt)
                {
                    tmp.Inven[i].C += remaincnt;
                    remaincnt = 0;
                }
                else
                {
                    remaincnt -= available;
                    tmp.Inven[i].C = InventorySlot.MaxSlots;
                }
            }
        }

        // Add가 성공했으니 반영 후 리턴
        if(remaincnt < 1)
        {
            g_Inven = tmp;
            SaveInvenJstr();
            SetNeedToRenew();
            return;
        }

        for(int i = 0; i < MAX_SLOT && remaincnt > 0; i++)
        {
            if (tmp.Inven[i].I == 0)
            {
                tmp.Inven[i].I = itemCode_;
                tmp.Inven[i].E = exTime_;
                tmp.Inven[i].C = remaincnt;

                // Add가 성공했으니 반영 후 리턴
                g_Inven = tmp;
                SaveInvenJstr();
                SetNeedToRenew();
                return;
            }
        }

        // 수정 불가
        return;
    }

    public static void Remove(int slotIdx_, int count_)
    {
        if (count_ > InventorySlot.MaxSlots)
        {
            Debug.Log($"InventoryPanel::Remove : count is bigger than MaxSlot");
            return;
        }

        // 깊은 복사를 위해 Jstr으로 저장한 후 다시 불러온다.
        SaveInvenJstr();
        Inventory tmp = GetInventory();

        if (tmp == null)
        {
            Debug.Log($"InventoryPanel::Remove : Cant Get InvenJstr");
            return;
        }

        if(tmp.Inven[slotIdx_] == null)
        {
            Debug.Log($"InventoryPanel::Remove : slot null ref");
        }

        if (tmp.Inven[slotIdx_].C <= count_)
        {
            tmp.Inven[slotIdx_].I = 0;
            tmp.Inven[slotIdx_].E = 0;
            tmp.Inven[slotIdx_].C = 0;
        }
        else
        {
            tmp.Inven[slotIdx_].C -= count_;
        }

        g_Inven = tmp;
        SaveInvenJstr();
        SetNeedToRenew();

        return;
    }

    public class SwapInvenParam
    {
        public int Idx1;
        public int Idx2;

        public SwapInvenParam(int idx1, int idx2)
        {
            Idx1 = idx1;
            Idx2 = idx2;
        }
    }

    /// <summary>
    /// 해당 함수에서는 서버에 스왑 요청만 보낸다.
    /// ACK가 오면 해당 처리를 HandleSwapInventory함수에서 수행한다.
    /// </summary>
    /// <param name="idx1"></param>
    /// <param name="idx2"></param>
    public static async void SwapInventorySlot(int idx1, int idx2)
    {
        if(idx1 == idx2)
        {
            Debug.Log($"InventoryPanel::SwapInventorySlot : same idx.");
            return;
        }

        if(idx1 < 0 || idx2 < 0 || idx1 >= MAX_SLOT || idx2 >= MAX_SLOT)
        {
            Debug.Log($"InventoryPanel::SwapInventorySlot : invalid idx.");
            return;
        }

        try
        {
            SwapInvenParam param = new SwapInvenParam(idx1, idx2);
            string req = JsonUtility.ToJson(param);
            string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.SWAP_INVENTORY, req);

            await NetworkManager.Instance.SendMsg(msg);
        }
        catch(Exception e)
        {
            Debug.Log($"InventoryPanel::SwapInventorySlot : {e.Message}");
        }

        return;
    }

    /// <summary>
    /// 서버의 ACK를 받으면 실질적으로 해당 함수에서 Swap하여 보여준다.
    /// </summary>
    /// <param name="idx1"></param>
    /// <param name="idx2"></param>
    public static void HandleSwapInventory(int idx1, int idx2)
    {
        if (idx1 == idx2)
        {
            Debug.Log($"InventoryPanel::HandleSwapInventory : same idx.");
            return;
        }

        if (idx1 < 0 || idx2 < 0 || idx1 >= MAX_SLOT || idx2 >= MAX_SLOT)
        {
            Debug.Log($"InventoryPanel::HandleSwapInventory : invalid idx.");
            return;
        }

        int code = g_Inven.Inven[idx1].I;
        int count = g_Inven.Inven[idx1].C;
        long extime = g_Inven.Inven[idx1].E;

        g_Inven.Inven[idx1].I = g_Inven[idx2].I;
        g_Inven.Inven[idx1].C = g_Inven[idx2].C;
        g_Inven.Inven[idx1].E = g_Inven[idx2].E;

        g_Inven.Inven[idx2].I = code;
        g_Inven.Inven[idx2].C = count;
        g_Inven.Inven[idx2].E = extime;

        SetNeedToRenew();
    }

    private static void SetNeedToRenew()
    {
        if (instance == null)
        {
            NeedToRenew = true;
        }
        else if (instance.gameObject.activeInHierarchy)
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

    public int GetItemCode(int slotidx_)
    {
        return g_Inven.Inven[slotidx_].I;
    }

    public int GetCount(int slotidx_)
    {
        return g_Inven.Inven[slotidx_].C;
    }

    /// <summary>
    /// 인벤창 하단의 골드 텍스트를 수정한다.
    /// </summary>
    /// <param name="gold">소지한 골드</param>
    public void SetGold(int gold)
    {
        GoldText.text = gold.ToString();
    }

    public void OnDrop(PointerEventData eventData)
    {
        int targetIdx = DraggingObject.Instance.GetIdx();
        Debug.Log($"InventoryPanel::OnDrop : From [{targetIdx}] Drop");
        if (targetIdx == -1)
        {
            return;
        }

        // 드래그중이던 정보 초기화
        DraggingObject.Instance.SetIdx(-1);
        DraggingObject.Instance.Hide();
    }


    /// <summary>
    /// 인벤토리 조작 가능 여부를 설정
    /// </summary>
    /// <param name="b"></param>
    public static void SetInteractable(bool b)
    {
        Debug.Log($"InventoryPanel::SetInteractable : Set {b}");
        m_bIsInteractable = b;
        return;
    }

    /// <summary>
    /// 현재 인벤토리가 조작가능한가 (아이템 위치 변경등의 조작이 가능한가)
    /// </summary>
    /// <returns></returns>
    public static bool IsInteractable()
    {
        return m_bIsInteractable;
    }

}
