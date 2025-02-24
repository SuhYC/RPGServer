using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using TMPro;

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
public class InventoryPanel : MonoBehaviour
{
    private static InventoryPanel instance;
    private static Inventory g_Inven = new Inventory();
    private static bool NeedToRenew;

    private Transform Content;
    private TMP_Text GoldText;

    // �κ��丮 ������ ���� ������ �־�ߵȴ�.
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

    /// <summary>
    /// �κ�â �ϴ��� ��� �ؽ�Ʈ�� �����Ѵ�.
    /// </summary>
    /// <param name="gold">������ ���</param>
    public void SetGold(int gold)
    {
        GoldText.text = gold.ToString();
    }
}
