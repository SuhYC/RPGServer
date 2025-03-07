using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
using System;
using UnityEngine.EventSystems;
using Unity.VisualScripting;


/// <summary>
/// �κ�â�� ������ ���Կ� �����Ǵ� ��ũ��Ʈ.
/// �ش� ���Կ� ������������ ����Ǹ�
/// ������ �°� �ؽ�ó�� ����Ѵ�.
/// </summary>
public class InventorySlot : MonoBehaviour, IBeginDragHandler, IEndDragHandler, IDropHandler
{
    public const int MaxSlots = 100; // �� ���Կ� ������ �� �ִ� �ִ� ������ ����

    private int m_ItemCode; // �������� ����
    private int m_count; // �������� ����
    private bool m_HasExDate; // �������� ����ð�

    private Image image; // �������� �ؽ�ó
    private TMP_Text text; // �������� ���� ��� �ؽ�Ʈ
    private GameObject ExImage; // �������� ���Ῡ�� ��� �̹����� ���ӿ�����Ʈ


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
    /// ������������ �Է¹޾� �ؽ�ó���� �ҷ��´�.
    /// Expire��ũ�� ���Ŀ� �߰��Ͽ� InventorySlot �������� ExpireImage�� ������.
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
            SetAlpha(1f); // ���� �Ƶ� ���� �� ���̰� �ϱ� ���� �ϴ� ���İ��� 1�� ����
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
    /// Ư�� �ε����� ��ġ�� �����Ѵ�.
    /// ���� �ۼ�.
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
    /// ������ ���� ��� �ؽ�Ʈ�� �����ϴ� �Լ�.
    /// </summary>
    /// <param name="count">�������� ����</param>
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
    /// �󽽷��� �ڵ��� ��� �ؽ�ó�� ���� �������� �ʰ�
    /// ���İ��� �����Ͽ� �����ϰ� ����ų�
    /// �ٽ� �������� �ڵ带 �޾� �ؽ�ó�� ���̰� �ϱ� ���� �������ϰ� �ٲٱ� ���� �Լ�.
    /// </summary>
    /// <param name="alpha">1 -> ������, 0 -> ����</param>
    private void SetAlpha(float alpha)
    {
        Color currentColor = image.color;
        currentColor.a = alpha;
        image.color = currentColor;

        return;
    }

    /// <summary>
    /// ����Ⱓ�� �ִ� �������� ��� expire�� true�� �Է��Ͽ� ȣ���� ��.
    /// ����Ⱓ�� ���� �������� ��� expire�� false�� �Է��Ͽ� ȣ���� ��.
    /// </summary>
    /// <param name="expire">�������� ����Ⱓ ���� ����</param>
    private void SetExpire(bool expire)
    {
        ExImage.SetActive(expire);

        return;
    }

    /// <summary>
    /// � ������ �巡���ߴ��� ���.
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

        // �󽽷��̸� �Ѿ��.
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
    /// OnDrop���� ���������� �ʱ�ȭ���� ���ߴٸ�
    /// �ش� �Լ����� DropItem�� ������ ��û.
    /// 
    /// �ݵ�� InventorySlot�� OnDrop�� �ƴϾ �ȴ�.
    /// ŸUI������ OnDrop�� �߻��ϸ� �ϴ� DropItem�� �ǵ��� ���� �ƴ� �� ������
    /// ŸUI���� OnDrop�� �����Ͽ� ������ ���ִ� �۾��� �� ��.
    /// </summary>
    /// <param name="eventData"></param>
    public void OnEndDrag(PointerEventData eventData)
    {
        Debug.Log($"InventorySlot[{transform.GetSiblingIndex()}]::OnEndDrag : End");

        int idx = DraggingObject.Instance.GetIdx();

        // OnDrop���� ó���ߴ�.
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


        // �巡�����̴� ���� �ʱ�ȭ
        DraggingObject.Instance.SetIdx(-1);
        DraggingObject.Instance.Hide();

        // �� ������ ���� ��ȯ
        swap(targetIdx);
    }
}
