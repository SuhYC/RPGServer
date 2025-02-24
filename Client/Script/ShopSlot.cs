using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;


/// <summary>
/// 아이템을 구매할 수 있는 상점의 품목슬롯에 부착되는 스크립트
/// </summary>
public class ShopSlot : MonoBehaviour
{
    private int m_ItemCode;
    private TMP_Text NameText;
    private TMP_Text CostText;
    private Image img;
    private static Canvas canvas;

    private static GameObject BuyPanel;

    void Awake()
    {
        NameText = transform.GetChild(1)?.GetChild(0)?.GetComponent<TMP_Text>();

        if(NameText == null)
        {
            Debug.Log($"ShopSlot::Awake : NameText null ref");
        }

        CostText = transform.GetChild(1)?.GetChild(1)?.GetComponent<TMP_Text>();

        if (CostText == null)
        {
            Debug.Log($"ShopSlot::Awake : CostText null ref");
        }

        img = transform.GetChild(0)?.GetComponent<Image>();

        if (img == null)
        {
            Debug.Log($"ShopSlot::Awake : Img null ref");
        }

        if(BuyPanel == null)
        {
            BuyPanel = Resources.Load<GameObject>("Prefabs/UI/BuyItemPanel");
            if (BuyPanel == null)
            {
                Debug.Log($"ShopSlot::Awake : BuyPanel null ref");
            }
        }

        if(canvas == null)
        {
            canvas = FindAnyObjectByType<Canvas>();

            if(canvas == null)
            {
                Debug.Log($"ShopSlot::Awake : canvas null ref");
            }
        }
    }


    /// <summary>
    /// 슬롯의 아이템 정보 초기화.
    /// </summary>
    /// <param name="code">아이템코드</param>
    /// <param name="name">아이템이름</param>
    /// <param name="price">아이템가격</param>
    public void Init(int code, string name, int price)
    {
        if(code == 0)
        {
            Debug.Log($"ShopSlot[{transform.GetSiblingIndex()}]::Init : Zero ItemCode");
            return;
        }
        
        m_ItemCode = code;

        string strItemCode = "Item/" + code.ToString();

        Sprite sprite = Resources.Load<Sprite>(strItemCode);

        if(sprite == null)
        {
            Debug.Log($"ShopSlot[{transform.GetSiblingIndex()}]::Init : sprite null ref");
            return;
        }

        img.sprite = sprite;

        NameText.text = name;

        CostText.text = price.ToString("N0") + " GOLD";
    }


    /// <summary>
    /// 해당 아이템을 구매하기 위해 선택하는 동작 <br/>
    /// BuyItemPanel을 생성하고 어떤 아이템을 선택했는지로 초기화한다.
    /// </summary>
    public void OnClick()
    {
        int npccode = ShopPanel.Instance.GetNPCCode();

        if(npccode == 0)
        {
            Debug.Log($"ShopSlot[{transform.GetSiblingIndex()}]::OnClick : Zero NPCCode");
            return;
        }

        GameObject obj = Instantiate(BuyPanel, canvas.transform);

        if(obj == null)
        {
            Debug.Log($"ShopSlot::OnClick : obj null ref.");
            return;
        }

        BuyItemPanel buyItemPanel = obj.GetComponent<BuyItemPanel>();

        if(buyItemPanel == null)
        {
            Debug.Log($"ShopSlot::OnClick : buypanel null ref");
            return;
        }

        buyItemPanel.Init(npccode, m_ItemCode);

        return;
    }
}
