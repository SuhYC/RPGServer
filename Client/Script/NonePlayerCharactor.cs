using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using UnityEngine.UI;


/// <summary>
/// ���� ���ҷ� ��ġ�� NPC�� ��ũ��Ʈ.
/// 
/// npccode�� ������ ������
/// �ش� npccode�� �°� ShopPanel�� SalesList�� �ʱ�ȭ�� �� �ֵ��� �Ѵ�.
/// 
/// �ش� ��ũ��Ʈ�� ������ ������Ʈ�� UI������Ʈ�� �ƴϹǷ� Collider�� �̿��� Ŭ���̺�Ʈ�� ó���ϴ� ClickManager�� ������ �޴´�.
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
    /// OnClick���� npccode�� �´� ���������͸� ã�� ���� ��� ������ ��û�Ѵ�.
    /// ��û �� ������ �����ϰ� �Ǹ� ȣ��Ǿ� �ٽ� ShopPanel�� �ʱ�ȭ�� �� �ֵ��� �Ѵ�.
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
    /// npc�� Ŭ���Ǿ��� �� ȣ��ȴ�.
    /// npccode�� �´� ������ ���ٸ� ������ SalesList�� ��û�ϰ� �����ϸ�,
    /// ������ ���ŵǸ� CallBackAfterNetworkResponse�� ȣ���Ѵ�.
    /// 
    /// ���� npccode�� �´� ������ �ִٸ� ShopPanel�� ������ �ʱ�ȭ�Ѵ�.
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
