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

        // 이걸 하지 않으면 OnDrop 이벤트를 드래깅 오브젝트가 다 먹어서 꺼줘야한다.
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
        // 마우스 위치를 얻어온다
        Vector3 mousePosition = Input.mousePosition;

        // Z값을 0으로 설정 (2D에서는 Z축 값이 중요하지 않음)
        mousePosition.z = 0;

        // 오브젝트 위치를 마우스 위치로 설정
        transform.position = mousePosition;

        return;
    }

    /// <summary>
    /// 인벤토리 슬롯을 드래그할 경우
    /// 어떤 슬롯을 드래그 했는지 기록하기 위함
    /// </summary>
    /// <param name="idx"></param>
    public void SetIdx(int idx)
    {
        m_index = idx;
    }

    public int GetIdx() { return m_index; }

    /// <summary>
    /// 해당 스프라이트로 오브젝트를 표시하고
    /// A값을 1로 설정.
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
    /// A값을 0으로 설정하여 안보이게 함.
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
