using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class TextMessage : MonoBehaviour
{
    static GameObject textPanel;

    public static Canvas canvas;

    public static void CreateTextPanel(string text_)
    {
        if (textPanel == null)
        {
            textPanel = Resources.Load<GameObject>("Prefabs/TextPanel");
        }

        GameObject obj = Instantiate(textPanel);

        obj.GetComponent<TextMessage>().Init(text_);
    }

    public void Init(string msg_)
    {
        TMP_Text txt = transform.GetChild(0)?.GetComponent<TMP_Text>();

        if (txt != null)
        {
            txt.text = msg_;
        }

        RectTransform rectTransform = transform.GetComponent<RectTransform>();
        if (rectTransform != null)
        {
            if(canvas == null)
            {
                canvas = FindAnyObjectByType<Canvas>();
            }

            // ȭ�� �߾ӿ� ��ġ
            rectTransform.SetParent(canvas.transform, false);  // Canvas�� �ڽ����� ����
            rectTransform.anchoredPosition = Vector2.zero;  // �߾ӿ� ��ġ�ϵ��� ����
        }
    }

    public void Destroy()
    {
        Destroy(gameObject);
    }
}
