using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading.Tasks;
using TMPro;
using UnityEngine;


/// <summary>
/// ĳ���� ����â�� ĳ���� ���Կ� �����Ǵ� ��ũ��Ʈ.
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
            // �ش� ������ ���° �������� Ȯ�� !!�ݵ�� �� ���� ���� ���� ������ �־���Ѵ�!!
            _slotIdx = transform.parent.GetSiblingIndex() * transform.parent.childCount + transform.GetSiblingIndex();
            _charinfo = UserData.instance.GetCharInfo(_slotIdx);
        }
        catch (CustomException.NotFoundInCharlistException)
        {
            // �� �̻� ĳ���Ͱ� ���� - �󽽷�
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
    /// ���õǾ��ٴ� ���� ǥ���� �ؽ�Ʈ ���� ���� �� <br/>
    /// ���� ���� ������ ��Ŭ���� ��� ������ ĳ���� ���� ��û
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
                // ������ ���õǾ��� ���԰� ���� ������ ������ ��ġ -> �ش� ĳ���ͷ� ����
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
    /// ĳ���� ���� �ؽ�Ʈ ����
    /// </summary>
    public void SetSelected()
    {
        _selectedText.text = "Selected";
    }

    /// <summary>
    /// ĳ���� ���� �ؽ�Ʈ ����
    /// </summary>
    public void SetUnSelected()
    {
        _selectedText.text = string.Empty;
    }
}
