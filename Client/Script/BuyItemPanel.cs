using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
using System.Threading.Tasks;


/// <summary>
/// 상점에서 물건을 선택한 후 출력되는 창.
/// 해당 창에서 갯수를 입력하고 엔터를 입력하거나 구매버튼을 클릭하여 구매요청을 서버로 보낸다.
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
    /// 해당 구매창이 어떤 아이템을 구매하기 위한 구매창인지 초기화.
    /// 반드시 해당 프리팹을 생성할 경우 호출해야한다.
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
    /// inputField에서 엔터키를 입력시 호출한다.
    /// 포커스를 잃어버린 경우에도 호출되므로 키입력을 확인한다.
    /// Return : 엔터, KeypadEnter : 텐키 엔터
    /// </summary>
    /// <param name="inputText">inputField에 입력된 값</param>
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
    /// 버튼을 직접 누른 경우에 실행된다.
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
            TextMessage.CreateTextPanel($"1개 이상, {InventorySlot.MaxSlots}개 이하만 구매할 수 있습니다.");
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
    /// 구매창을 종료하는 버튼이 눌렸을 때.
    /// 구매를 원하지 않는 경우 호출
    /// 추후 esc키를 통한 종료에도 사용할 수 있다.
    /// 이 경우 focus를 확인해야한다. 죄다 종료되면 안되니
    /// </summary>
    public void Exit()
    {
        Destroy(gameObject);
    }
}
