using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using TMPro;
using UnityEngine.EventSystems;
using UnityEngine.UIElements;
using Unity.VisualScripting;

/// <summary>
/// �κ��丮 â�� �����Ǵ� ��ũ��Ʈ.
/// 
/// �κ��丮 â ���� ���̾��Ű�� ���Ե��� �����ϸ�
/// �κ��丮 ������ �����κ��� �޾� UserDataŬ������ JSONStr���·� �����ص� ��
/// �ҷ��� �ʱ�ȭ�Ѵ�.
/// 
/// NeedToRenew : �����κ��� ���� ���ο� ������ ����. �ʱ�ȭ�� �ʿ��ϹǷ� �κ�â�� �ʱ�ȭ ������ �� �ʱ�ȭ�Ѵ�.
/// g_Inven : UserDataŬ�������� �޾ƿ� JSONStr�� ������ȭ�Ͽ� ������ ����.
/// Content : Slot���� �ڽĵ�� �����Ǵ� Ʈ������.
/// GoldText : �κ��丮 �ϴܿ� ǥ�õǴ� ���� ������ ���
/// </summary>
public class InventoryPanel : MonoBehaviour, IDropHandler
{
    private static GameObject prefab;
    private static Canvas canvas;

    private static InventoryPanel instance;
    private static Inventory g_Inven = new Inventory();
    private static bool NeedToRenew;
    private static bool m_bIsInteractable;

    // �κ��丮 ���� �ִ� ����
    public const int MAX_SLOT = 64;

    private Transform Content;
    private TMP_Text GoldText;

    // �κ��丮 ������ ���� ������ �־�ߵȴ�.
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
    /// �κ�â�� �ٽ� Ȱ��ȭ �� �� ȣ��Ǿ� ������ �ʿ��� ��� InitSlot�� ȣ���Ѵ�.
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
    /// �����κ��� �޾ƿ� �κ��丮 ������ ���������� �����Ѵ�.
    /// ���� �ҷ����� ���� �ݵ�� ����Ǿ���Ѵ�.
    /// </summary>
    /// <param name="jsonstr_">Inventory�� JSON���ڿ�</param>
    public static void InitInvenInfo(string jsonstr_)
    {
        Debug.Log($"InventoryPanel::InitInvenInfo : {jsonstr_}");
        g_Inven = JsonUtility.FromJson<Inventory>(jsonstr_);

        SetNeedToRenew();

        return;
    }

    /// <summary>
    /// ���� ĳ���� ����â���� ���ư��� ���۵� ����� ����
    /// ���� �κ��丮�� jstr���� ���� UserData�� �����ϴ� �Լ�.
    /// 
    /// ������ ��û�ص� ������ �װ� NetworkIO�� ���� �ø��� �� ����.
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

        // ���� ���縦 ���� Jstr���� ������ �� �ٽ� �ҷ��´�.
        SaveInvenJstr();
        Inventory tmp = GetInventory();

        if(tmp == null)
        {
            Debug.Log($"InventoryPanel::Add : Cant Get InvenJstr");
            return;
        }

        int remaincnt = count_;

        // ������ �ش� �������� �����ϰ� �־��ٸ� �ش� ���Կ� �켱 ä���.
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

        // Add�� ���������� �ݿ� �� ����
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

                // Add�� ���������� �ݿ� �� ����
                g_Inven = tmp;
                SaveInvenJstr();
                SetNeedToRenew();
                return;
            }
        }

        // ���� �Ұ�
        return;
    }

    public static void Remove(int slotIdx_, int count_)
    {
        if (count_ > InventorySlot.MaxSlots)
        {
            Debug.Log($"InventoryPanel::Remove : count is bigger than MaxSlot");
            return;
        }

        // ���� ���縦 ���� Jstr���� ������ �� �ٽ� �ҷ��´�.
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
    /// �ش� �Լ������� ������ ���� ��û�� ������.
    /// ACK�� ���� �ش� ó���� HandleSwapInventory�Լ����� �����Ѵ�.
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
    /// ������ ACK�� ������ ���������� �ش� �Լ����� Swap�Ͽ� �����ش�.
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
    /// ���������� ������ �ִ� ������ ������ �κ��丮 �ν��Ͻ��� �ݿ��Ѵ�.
    /// ���� �ҷ������� ������ ȣ���ϸ� �ȵȴ�.
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
    /// �κ�â �ϴ��� ��� �ؽ�Ʈ�� �����Ѵ�.
    /// </summary>
    /// <param name="gold">������ ���</param>
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

        // �巡�����̴� ���� �ʱ�ȭ
        DraggingObject.Instance.SetIdx(-1);
        DraggingObject.Instance.Hide();
    }


    /// <summary>
    /// �κ��丮 ���� ���� ���θ� ����
    /// </summary>
    /// <param name="b"></param>
    public static void SetInteractable(bool b)
    {
        Debug.Log($"InventoryPanel::SetInteractable : Set {b}");
        m_bIsInteractable = b;
        return;
    }

    /// <summary>
    /// ���� �κ��丮�� ���۰����Ѱ� (������ ��ġ ������� ������ �����Ѱ�)
    /// </summary>
    /// <returns></returns>
    public static bool IsInteractable()
    {
        return m_bIsInteractable;
    }

}
