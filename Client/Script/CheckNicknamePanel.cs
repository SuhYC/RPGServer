using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class CheckNicknamePanel : MonoBehaviour
{
    private TMP_Text _Text;
    private CreateCharParam _param;
    private CreateCharButton _createCharButton;
    private CancelReserveNicknameButton _cancelButton;

    private static GameObject _nickPanel;
    public static Canvas canvas;

    private class CreateCharParam
    {
        public string CharName;
    }

    /// <summary>
    /// �Ķ���ʹ� CreateCharParam�� JSON���ڿ��� ���� ��
    /// </summary>
    /// <param name="text_"></param>
    public static void CreateTextPanel(string text_)
    {
        if (_nickPanel == null)
        {
            _nickPanel = Resources.Load<GameObject>("Prefabs/NicknameCheckPanel");
        }

        GameObject obj = Instantiate(_nickPanel);

        obj.GetComponent<CheckNicknamePanel>().Init(text_);
    }

    void Awake()
    {
        _param = new CreateCharParam();
        _Text = transform.GetChild(0)?.GetComponent<TMP_Text>();   
        if(_Text == null )
        {
            Debug.Log($"CheckNicknamePanel::Awake : null ref on TMP_Text");
        }

        _createCharButton = transform.GetChild(1)?.GetChild(0)?.GetComponent<CreateCharButton>();
        if (_createCharButton == null)
        {
            Debug.Log($"CheckNicknamePanel::Awake : null ref on CreateCharButton");
        }

        _cancelButton = transform.GetChild(1)?.GetChild(1)?.GetComponent<CancelReserveNicknameButton>();
        if (_cancelButton == null)
        {
            Debug.Log($"CheckNicknamePanel::Awake : null ref on CancelButton");
        }
    }

    private void Init(string jsonstr_)
    {
        _param = JsonUtility.FromJson<CreateCharParam>(jsonstr_);

        if(_createCharButton == null)
        {
            Debug.Log($"CheckNicknamePanel::Init : null ref on createCharButton");
            return;
        }

        if(_cancelButton == null)
        {
            Debug.Log($"CheckNicknamePanel::Init : null ref on cancelButton");
        }

        _createCharButton.Init(jsonstr_);
        _cancelButton.Init(jsonstr_);

        if(_Text == null)
        {
            Debug.Log($"CheckNicknamePanel::Init : null ref on Text");
        }

        _Text.text = "\"" + _param.CharName + "\"���� �����Ͻðڽ��ϱ�?";

        RectTransform rectTransform = transform.GetComponent<RectTransform>();
        if (rectTransform != null)
        {
            if (canvas == null)
            {
                canvas = FindAnyObjectByType<Canvas>();
            }

            // ȭ�� �߾ӿ� ��ġ
            rectTransform.SetParent(canvas.transform, false);  // Canvas�� �ڽ����� ����
            rectTransform.anchoredPosition = Vector2.zero;  // �߾ӿ� ��ġ�ϵ��� ����
        }

        return;
    }

    public void OnClick()
    {
        if (canvas == null)
        {
            canvas = FindAnyObjectByType<Canvas>();
        }

        if (canvas == null)
        {
            Debug.Log($"CheckNicknamePanel::Onclick : nullref.");
            return;
        }

        Transform charlistPanel = canvas.transform.GetChild(0); // Charlist panel

        if (charlistPanel.name != "CharlistPanel")
        {
            Debug.Log($"CheckNicknamePanel::Onclick : Cant Find CharlistPanel");
            return;
        }
        charlistPanel.gameObject.SetActive(true);

        Transform createCharPanel = canvas.transform.GetChild(1); // CreateChar Panel

        if (createCharPanel.name != "CreateCharPanel")
        {
            Debug.Log($"CheckNicknamePanel::Onclick : Cant Find CreateCharPanel");
            return;
        }
    }
}