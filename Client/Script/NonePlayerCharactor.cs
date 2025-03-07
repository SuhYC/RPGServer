using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using UnityEngine.UI;


/// <summary>
/// 상인 역할로 배치할 NPC의 스크립트.
/// 
/// npccode를 가지고 있으며
/// 해당 npccode에 맞게 ShopPanel의 SalesList로 초기화할 수 있도록 한다.
/// 
/// 해당 스크립트가 부착된 오브젝트는 UI오브젝트가 아니므로 Collider를 이용해 클릭이벤트를 처리하는 ClickManager의 도움을 받는다.
/// </summary>
public class NonePlayerCharactor : MonoBehaviour
{
    private static Dictionary<int, NonePlayerCharactor> scripts;

    public int m_npccode;

    private GetSalesListParam m_stParam;

    public class GetSalesListParam
    {
        public int NPCCode;

        public GetSalesListParam(int npccode_) { NPCCode = npccode_; }
    }

    private void Awake()
    {
        if(m_npccode == 0)
        {
            Debug.Log($"NPC::Awake : npccode is invalid.");
            return;
        }

        scripts = new Dictionary<int, NonePlayerCharactor>();

        scripts.TryAdd(m_npccode, this);
    }

    private void OnDestroy()
    {
        scripts.Remove(m_npccode);
    }

    /// <summary>
    /// OnClick에서 npccode에 맞는 상점데이터를 찾지 못한 경우 서버에 요청한다.
    /// 요청 후 응답을 수신하게 되면 호출되어 다시 ShopPanel을 초기화할 수 있도록 한다.
    /// </summary>
    /// <param name="npccode"></param>
    public static void CallBackAfterNetworkResponse(int npccode)
    {
        if(!ShopPanel.HasData(npccode))
        {
            Debug.Log($"NPC::CallBackAfterNetworkResponse : ShopPanel Has No Data on {npccode}");
            return;
        }

        if (ShopPanel.Instance == null)
        {
            ShopPanel.CreateInstance();
        }

        ShopPanel.Instance.gameObject.SetActive(true);

        ShopPanel.Instance.InitSalesList(npccode);
    }

    /// <summary>
    /// npc가 클릭되었을 때 호출된다.
    /// npccode에 맞는 정보가 없다면 서버에 SalesList를 요청하고 종료하며,
    /// 응답이 수신되면 CallBackAfterNetworkResponse를 호출한다.
    /// 
    /// 만약 npccode에 맞는 정보가 있다면 ShopPanel을 적절히 초기화한다.
    /// </summary>
    public async void OnClick()
    {
        Debug.Log($"NPC::OnClick : Clicked!");

        if(!ShopPanel.HasData(m_npccode))
        {
            m_stParam = new GetSalesListParam(m_npccode);

            string jsonstr = JsonUtility.ToJson(m_stParam);

            string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.GET_SALESLIST, jsonstr);

            await NetworkManager.Instance.SendMsg(msg);

            return;
        }

        if (ShopPanel.Instance == null)
        {
            ShopPanel.CreateInstance();
        }

        ShopPanel.Instance.gameObject.SetActive(true);

        ShopPanel.Instance.InitSalesList(m_npccode);
    }
}
