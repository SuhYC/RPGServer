using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class DraggingObject : MonoBehaviour
{
    private static DraggingObject instance;
    private static GameObject go;
    private static Canvas canvas;
    private Image image;
    private int m_index;


    public static DraggingObject Instance
    {
        get { return instance; }
    }

    public static void CreateInstance()
    {
        if(go == null)
        {
            go = Resources.Load<GameObject>("Prefabs/UI/DraggingImage");
            if(go == null)
            {
                Debug.Log($"DraggingObject::CreateInstance : go null ref");
                return;
            }
        }

        if(canvas == null)
        {
            canvas = FindAnyObjectByType<Canvas>();
            if(canvas == null)
            {
                Debug.Log($"DraggingObject::CreateInstance : canvas null ref");
                return;
            }
        }

        GameObject obj = Instantiate(go, canvas.transform);
        Graphic graphic = obj.GetComponent<Graphic>();

        // �̰� ���� ������ OnDrop �̺�Ʈ�� �巡�� ������Ʈ�� �� �Ծ ������Ѵ�.
        graphic.raycastTarget = false;
    }

    private void Awake()
    {
        instance = this;

        m_index = -1;

        image = GetComponent<Image>();
        if(image == null )
        {
            Debug.Log($"DraggingObject::Awake : image null ref");
        }

        Hide();
    }

    private void Update()
    {
        MoveToMousePosition();
    }

    private void MoveToMousePosition()
    {
        // ���콺 ��ġ�� ���´�
        Vector3 mousePosition = Input.mousePosition;

        // Z���� 0���� ���� (2D������ Z�� ���� �߿����� ����)
        mousePosition.z = 0;

        // ������Ʈ ��ġ�� ���콺 ��ġ�� ����
        transform.position = mousePosition;

        return;
    }

    /// <summary>
    /// �κ��丮 ������ �巡���� ���
    /// � ������ �巡�� �ߴ��� ����ϱ� ����
    /// </summary>
    /// <param name="idx"></param>
    public void SetIdx(int idx)
    {
        m_index = idx;
    }

    public int GetIdx() { return m_index; }

    /// <summary>
    /// �ش� ��������Ʈ�� ������Ʈ�� ǥ���ϰ�
    /// A���� 1�� ����.
    /// </summary>
    /// <param name="sprite_"></param>
    public void SetSprite(Sprite sprite_)
    {
        Debug.Log($"DraggingObject::SetSprite");

        image.sprite = sprite_;
        Color color = image.color;
        color.a = 1f;
        image.color = color;

        return;
    }

    /// <summary>
    /// A���� 0���� �����Ͽ� �Ⱥ��̰� ��.
    /// </summary>
    public void Hide()
    {
        Debug.Log($"DraggingObject::Hide");
        Color color = image.color;
        color.a = 0;
        image.color = color;

        return;
    }
}
