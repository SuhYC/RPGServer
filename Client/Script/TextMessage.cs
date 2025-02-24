using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

/// <summary>
/// 사용자에게 특정 정보를 일방적으로 알리는 스크립트.
/// 
/// CreateTextPanel : 특정 문구를 입력받아 TextPanel 프리팹을 생성하고
/// 입력받았던 문구를 TextPanel에 출력한다.
/// </summary>
public class TextMessage : MonoBehaviour
{
    static GameObject textPanel;

    public static Canvas canvas;

    /// <summary>
    /// 특정 문구를 입력받아 프리팹을 생성하는 함수.
    /// 완전한 문장으로 입력할 것. (별도의 처리 없이 입력받은 그대로 출력한다.)
    /// </summary>
    /// <param name="text_">알림 문구</param>
    public static void CreateTextPanel(string text_)
    {
        if (textPanel == null)
        {
            textPanel = Resources.Load<GameObject>("Prefabs/UI/TextPanel");
        }

        GameObject obj = Instantiate(textPanel);

        obj.GetComponent<TextMessage>().Init(text_);
    }

    /// <summary>
    /// CreateTextPanel로 입력받은 문구를 TMP_Text의 text에 입력해주는 함수.
    /// </summary>
    /// <param name="msg_">출력할 메시지</param>
    private void Init(string msg_)
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
