using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

/// <summary>
/// 캐릭터의 이름을 입력한 후
/// 해당 캐릭터가 생성가능하면 출력되는 창의 스크립트.
/// 서버에 해당 캐릭터가 생성가능여부를 요청하면서 동시에 분산락을 이용해 해당 캐릭터이름에 예약을 걸게 되므로
/// 해당 예약이 지속되는 동안 다른 유저가 닉네임을 가로챌 수 없다.
/// 해당 창에서 캐릭터 생성에 동의하면 최종적으로 캐릭터를 생성한다.
/// 해당 창에서 캐릭터 생성을 거부하면 예약을 해제한다.
/// 
/// 해당 클래스는 예약을 건 닉네임을 입력받아
/// 닉네임으로 텍스트를 초기화하는 클래스이다.
/// 
/// 생성 동의버튼과 거부버튼에 동시에 닉네임으로 초기화할 수 있게하여 이후 서버요청을 가능하게 한다.
/// </summary>
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
    /// 파라미터는 CreateCharParam의 JSON문자열을 적을 것
    /// </summary>
    /// <param name="text_">CreateCharParam의 JSON문자열</param>
    public static void CreateTextPanel(string text_)
    {
        if (_nickPanel == null)
        {
            _nickPanel = Resources.Load<GameObject>("Prefabs/UI/NicknameCheckPanel");
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

        _Text.text = "\"" + _param.CharName + "\"으로 생성하시겠습니까?";

        RectTransform rectTransform = transform.GetComponent<RectTransform>();
        if (rectTransform != null)
        {
            if (canvas == null)
            {
                canvas = FindAnyObjectByType<Canvas>();
            }

            // 화면 중앙에 배치
            rectTransform.SetParent(canvas.transform, false);  // Canvas의 자식으로 설정
            rectTransform.anchoredPosition = Vector2.zero;  // 중앙에 위치하도록 설정
        }

        return;
    }

    public void Destroy()
    {
        Destroy(gameObject);
        return;
    }
}
