using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;


/// <summary>
/// �������� ������ �� �ִ� ������ ǰ�񽽷Կ� �����Ǵ� ��ũ��Ʈ
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
    /// ������ ������ ���� �ʱ�ȭ.
    /// </summary>
    /// <param name="code">�������ڵ�</param>
    /// <param name="name">�������̸�</param>
    /// <param name="price">�����۰���</param>
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
    /// �ش� �������� �����ϱ� ���� �����ϴ� ���� <br/>
    /// BuyItemPanel�� �����ϰ� � �������� �����ߴ����� �ʱ�ȭ�Ѵ�.
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
