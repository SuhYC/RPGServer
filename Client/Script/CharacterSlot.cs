using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading.Tasks;
using TMPro;
using UnityEngine;


/// <summary>
/// 캐릭터 선택창의 캐릭터 슬롯에 부착되는 스크립트.
/// </summary>
public class CharacterSlot : MonoBehaviour
{
    private static readonly AsyncLock _asyncLock = new AsyncLock();

    private int _slotIdx;
    UserData.CharInfo _charinfo;
    private TMP_Text _text;
    private TMP_Text _selectedText;

    private class SelectCharParam
    {
        public int Charcode;
    }


    void OnEnable()
    {
        _text = transform.GetChild(1)?.GetComponent<TMP_Text>();
        _selectedText = transform.GetChild(2)?.GetComponent<TMP_Text>();

        SetUnSelected();

        if( _text == null || _selectedText == null)
        {
            Debug.Log($"CharacterSlot::Awake : text null ref.");
            return;
        }

        try
        {
            // 해당 슬롯이 몇번째 슬롯인지 확인 !!반드시 두 행은 같은 수의 슬롯이 있어야한다!!
            _slotIdx = transform.parent.GetSiblingIndex() * transform.parent.childCount + transform.GetSiblingIndex();
            _charinfo = UserData.instance.GetCharInfo(_slotIdx);
        }
        catch (CustomException.NotFoundInCharlistException)
        {
            // 더 이상 캐릭터가 없음 - 빈슬롯
            _text.text = "Empty";
            return;
        }
        catch (Exception e)
        {
            Debug.Log($"CharacterSlot[{_slotIdx}]::Awake : {e.Message}");
            return;
        }

        _text.text = $"Name: {_charinfo.CharName}\nLv : {_charinfo.Level}";
    }


    /// <summary>
    /// 선택되었다는 것을 표시할 텍스트 정보 수정 및 <br/>
    /// 기존 선택 슬롯을 재클릭한 경우 서버에 캐릭터 선택 요청
    /// </summary>
    public async void OnClick()
    {
        if(_charinfo == null)
        {
            return;
        }

        Transform line1 = transform.parent.parent.GetChild(0);
        Transform line2 = transform.parent.parent.GetChild(1);

        if(line1 == null || line2 == null )
        {
            Debug.Log("CharacterSlot::OnClick : line null ref.");
            return;
        }

        for (int i = 0; i < line1.childCount; i++)
        {
            line1.GetChild(i)?.GetComponent<CharacterSlot>()?.SetUnSelected();
        }

        for (int i = 0; i < line2.childCount; i++)
        {
            line2.GetChild(i)?.GetComponent<CharacterSlot>()?.SetUnSelected();
        }

        SetSelected();


        using (await _asyncLock.LockAsync())
        {
            int charno = UserData.instance.GetSelectedChar();

            try
            {
                // 기존에 선택되었던 슬롯과 지금 선택한 슬롯이 일치 -> 해당 캐릭터로 시작
                if (charno == UserData.instance.SetSelectedChar(_slotIdx))
                {
                    SelectCharParam param = new SelectCharParam();
                    param.Charcode = charno;

                    string str = JsonUtility.ToJson(param);

                    string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.SELECT_CHAR, str);

                    await NetworkManager.Instance.SendMsg(msg);
                }
            }
            catch (Exception e)
            {
                Debug.Log($"CharacterSlot::OnClick : {e.Message}");
            }
            
        }
    }


    /// <summary>
    /// 캐릭터 선택 텍스트 수정
    /// </summary>
    public void SetSelected()
    {
        _selectedText.text = "Selected";
    }

    /// <summary>
    /// 캐릭터 선택 텍스트 수정
    /// </summary>
    public void SetUnSelected()
    {
        _selectedText.text = string.Empty;
    }
}
