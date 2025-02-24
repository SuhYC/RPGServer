using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
using System.Threading.Tasks;


/// <summary>
/// �������� ������ ������ �� ��µǴ� â.
/// �ش� â���� ������ �Է��ϰ� ���͸� �Է��ϰų� ���Ź�ư�� Ŭ���Ͽ� ���ſ�û�� ������ ������.
/// </summary>
public class BuyItemPanel : MonoBehaviour
{
    int npccode;
    int itemcode;
    TMP_InputField input;
    public class BuyItemParam
    {
        public int NPCCode;
        public int ItemCode;
        public int Count;
        public BuyItemParam(int NPCCode_, int itemCode_, int count_)
        {
            NPCCode = NPCCode_;
            ItemCode = itemCode_;
            Count = count_;
        }
    }

    private void Awake()
    {
        npccode = 0;
        itemcode = 0;

        input = transform.GetChild(1)?.GetComponent<TMP_InputField>();

        if(input == null)
        {
            Debug.Log($"BuyItemPanel::Awake : input null ref");
            return;
        }

        input.onEndEdit.AddListener(EndEditBuy);
    }

    /// <summary>
    /// �ش� ����â�� � �������� �����ϱ� ���� ����â���� �ʱ�ȭ.
    /// �ݵ�� �ش� �������� ������ ��� ȣ���ؾ��Ѵ�.
    /// </summary>
    /// <param name="npccode_"></param>
    /// <param name="itemcode_"></param>
    public void Init(int npccode_, int itemcode_)
    {
        npccode = npccode_;
        itemcode = itemcode_;
        return;
    }

    /// <summary>
    /// inputField���� ����Ű�� �Է½� ȣ���Ѵ�.
    /// ��Ŀ���� �Ҿ���� ��쿡�� ȣ��ǹǷ� Ű�Է��� Ȯ���Ѵ�.
    /// Return : ����, KeypadEnter : ��Ű ����
    /// </summary>
    /// <param name="inputText">inputField�� �Էµ� ��</param>
    public async void EndEditBuy(string inputText)
    {
        if (inputText == string.Empty)
        {
            return;
        }

        if(!Input.GetKeyDown(KeyCode.Return) && !Input.GetKeyDown(KeyCode.KeypadEnter))
        {
            return;
        }

        try
        {
            int count = int.Parse(inputText);
            await ReqBuy(count);
        }
        catch (System.Exception e)
        {
            Debug.Log($"BuyItemPanel::EndEditBuy : {e.Message}");
        }
    }

    /// <summary>
    /// ��ư�� ���� ���� ��쿡 ����ȴ�.
    /// </summary>
    public async void Buy()
    {
        if(input.text == string.Empty)
        {
            return;
        }

        try
        {
            int count = int.Parse(input.text);
            await ReqBuy(count);
        }
        catch (System.Exception e)
        {
            Debug.Log($"BuyItemPanel::Buy : {e.Message}");
        }
    }

    private async Task ReqBuy(int count)
    {
        if(itemcode == 0 || npccode == 0)
        {
            Debug.Log($"BuyItemPanel::ReqBuy : Not Initialized. Must Execute Init().");
            return;
        }

        if(count < 1 || count > InventorySlot.MaxSlots)
        {
            TextMessage.CreateTextPanel($"1�� �̻�, {InventorySlot.MaxSlots}�� ���ϸ� ������ �� �ֽ��ϴ�.");
            return;
        }

        try
        {
            BuyItemParam stParam = new BuyItemParam(npccode, itemcode, count);
            Debug.Log($"npc : {npccode}, item : {itemcode}, count : {count}");
            string str = JsonUtility.ToJson(stParam);

            string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.BUY_ITEM, str);
            await NetworkManager.Instance.SendMsg(msg);

            UserData.CharInfo info = UserData.instance.GetCharInfo();

            if(info == null)
            {
                Debug.Log($"");
                return;
            }

            await UserData.instance.ReqCharInfo(info.CharNo);
        }
        catch (System.Exception e)
        {
            Debug.Log($"BuyItemPanel::ReqBuy : {e.Message}");
        }
    }

    /// <summary>
    /// ����â�� �����ϴ� ��ư�� ������ ��.
    /// ���Ÿ� ������ �ʴ� ��� ȣ��
    /// ���� escŰ�� ���� ���ῡ�� ����� �� �ִ�.
    /// �� ��� focus�� Ȯ���ؾ��Ѵ�. �˴� ����Ǹ� �ȵǴ�
    /// </summary>
    public void Exit()
    {
        Destroy(gameObject);
    }
}
