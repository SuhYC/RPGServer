using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
using System;
using UnityEngine.EventSystems;
using Unity.VisualScripting;


/// <summary>
/// 인벤창의 각각의 슬롯에 부착되는 스크립트.
/// 해당 슬롯에 아이템정보가 저장되며
/// 정보에 맞게 텍스처를 출력한다.
/// </summary>
public class InventorySlot : MonoBehaviour, IBeginDragHandler, IEndDragHandler, IDropHandler
{
    public const int MaxSlots = 100; // 한 슬롯에 보유할 수 있는 최대 아이템 갯수

    private int m_ItemCode; // 아이템의 종류
    private int m_count; // 아이템의 갯수
    private bool m_HasExDate; // 아이템의 만료시간

    private Image image; // 아이템의 텍스처
    private TMP_Text text; // 아이템의 갯수 출력 텍스트
    private GameObject ExImage; // 아이템의 만료여부 출력 이미지의 게임오브젝트


    private void Awake()
    {
        image = transform.GetChild(0)?.GetComponent<Image>();

        int idx = transform.GetSiblingIndex();

        if(image == null )
        {
            Debug.Log($"InventorySlot[{idx}]::Awake : image nullref.");
        }

        text = transform.GetChild(1)?.GetComponent<TMP_Text>();

        if( text == null )
        {
            Debug.Log($"InventorySlot[{idx}]::Awake : text nullref.");
        }

        ExImage = transform.GetChild(2)?.gameObject;

        if (ExImage == null)
        {
            Debug.Log($"InventorySlot[{idx}]::Awake : eximage nullref.");
        }
    }

    /// <summary>
    /// 아이템정보를 입력받아 텍스처까지 불러온다.
    /// Expire마크를 추후에 추가하여 InventorySlot 프리팹의 ExpireImage에 넣을것.
    /// </summary>
    /// <param name="itemcode_"></param>
    /// <param name="count_"></param>
    /// <param name="HasExpirationDate"></param>
    public void Init(int itemcode_, int count_, bool HasExpirationDate)
    {
        m_count = count_;
        m_HasExDate = HasExpirationDate;
        m_ItemCode = itemcode_;
        
        SetCount(m_count);

        if (m_count == 0 )
        {
            SetAlpha(0f);
            SetExpire(false);
            return;
        }

        Texture2D texture = null;

        try
        {
            texture = Resources.Load<Texture2D>("Item/" + itemcode_.ToString());
        }
        catch(Exception e)
        {
            Debug.Log($"{e.Message}");
        }

        if ( texture == null )
        {
            Debug.Log($"InventorySlot[{transform.GetSiblingIndex()}]::Init : Failed to load texture.");
            SetAlpha(1f); // 뭐가 됐든 눈에 잘 보이게 하기 위해 일단 알파값을 1로 설정
            SetExpire(false);
            return;
        }
        else
        {
            Sprite sprite = Sprite.Create(texture, new Rect(0, 0, texture.width, texture.height), new Vector2(0.5f, 0.5f));
            image.sprite = sprite;
            SetAlpha(1f);
            SetExpire(HasExpirationDate);
        }

        return;
    }

    /// <summary>
    /// 특정 인덱스와 위치를 변경한다.
    /// 추후 작성.
    /// 
    /// </summary>
    /// <param name="idx"></param>
    public void swap(int idx)
    {
        int now = transform.GetSiblingIndex();
        if(idx == now)
        {
            Debug.Log($"InventorySlot::swap : same idx");
            return;
        }

        InventoryPanel.SwapInventorySlot(idx, now);
    }

    /// <summary>
    /// 슬롯의 갯수 출력 텍스트를 수정하는 함수.
    /// </summary>
    /// <param name="count">아이템의 갯수</param>
    private void SetCount(int count)
    {
        if(count == 0)
        {
            text.text = string.Empty;
            return;
        }

        text.text = count.ToString();

        return;
    }

    /// <summary>
    /// 빈슬롯의 코드인 경우 텍스처를 따로 적용하지 않고
    /// 알파값을 수정하여 투명하게 만들거나
    /// 다시 아이템의 코드를 받아 텍스처를 보이게 하기 위해 불투명하게 바꾸기 위한 함수.
    /// </summary>
    /// <param name="alpha">1 -> 불투명, 0 -> 투명</param>
    private void SetAlpha(float alpha)
    {
        Color currentColor = image.color;
        currentColor.a = alpha;
        image.color = currentColor;

        return;
    }

    /// <summary>
    /// 만료기간이 있는 아이템의 경우 expire를 true로 입력하여 호출할 것.
    /// 만료기간이 없는 아이템의 경우 expire를 false로 입력하여 호출할 것.
    /// </summary>
    /// <param name="expire">아이템의 만료기간 보유 여부</param>
    private void SetExpire(bool expire)
    {
        ExImage.SetActive(expire);

        return;
    }

    /// <summary>
    /// 어떤 슬롯을 드래그했는지 기록.
    /// </summary>
    /// <param name="eventData"></param>
    public void OnBeginDrag(PointerEventData eventData)
    {
        int DragIndex = transform.GetSiblingIndex();

        if (!InventoryPanel.IsInteractable())
        {
            Debug.Log($"InventorySlot[{DragIndex}]::OnBeginDrag : Not Interactable");
            return;
        }

        Debug.Log($"InventorySlot[{DragIndex}]::OnBeginDrag : Start");

        // 빈슬롯이면 넘어간다.
        if (m_ItemCode == 0)
        {
            return;
        }

        if(DraggingObject.Instance == null)
        {
            DraggingObject.CreateInstance();
        }

        DraggingObject.Instance.SetIdx(DragIndex);

        DraggingObject.Instance.SetSprite(image.sprite);

        Debug.Log($"InventorySlot[{DragIndex}]::OnBeginDrag : Ready!");
        return;
    }

    /// <summary>
    /// OnDrop으로 슬롯정보를 초기화하지 못했다면
    /// 해당 함수에서 DropItem을 서버에 요청.
    /// 
    /// 반드시 InventorySlot의 OnDrop이 아니어도 된다.
    /// 타UI에서도 OnDrop이 발생하면 일단 DropItem을 의도한 것이 아닐 수 있으니
    /// 타UI에도 OnDrop을 구현하여 정보를 없애는 작업을 할 것.
    /// </summary>
    /// <param name="eventData"></param>
    public void OnEndDrag(PointerEventData eventData)
    {
        Debug.Log($"InventorySlot[{transform.GetSiblingIndex()}]::OnEndDrag : End");

        int idx = DraggingObject.Instance.GetIdx();

        // OnDrop에서 처리했다.
        if (idx == -1)
        {
            return;
        }

        InventoryPanel.SetInteractable(false);

        DraggingObject.Instance.SetIdx(-1);
        DraggingObject.Instance.Hide();

        DropItemPanel.CreateDropItemPanel(idx);
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="eventData"></param>
    public void OnDrop(PointerEventData eventData)
    {
        int targetIdx = DraggingObject.Instance.GetIdx();
        Debug.Log($"InventorySlot[{transform.GetSiblingIndex()}]::OnDrop : From [{targetIdx}] Drop");
        if ( targetIdx == -1 )
        {
            return;
        }


        // 드래그중이던 정보 초기화
        DraggingObject.Instance.SetIdx(-1);
        DraggingObject.Instance.Hide();

        // 두 슬롯의 정보 교환
        swap(targetIdx);
    }
}
