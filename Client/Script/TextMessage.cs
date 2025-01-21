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

            // 화면 중앙에 배치
            rectTransform.SetParent(canvas.transform, false);  // Canvas의 자식으로 설정
            rectTransform.anchoredPosition = Vector2.zero;  // 중앙에 위치하도록 설정
        }
    }

    public void Destroy()
    {
        Destroy(gameObject);
    }
}
