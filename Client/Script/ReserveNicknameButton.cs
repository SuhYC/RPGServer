using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ReserveNicknameButton : MonoBehaviour
{
    private CreateCharData _data;
    private Button _Button;

    private void Awake()
    {
        _data = transform.parent?.GetComponent<CreateCharData>();
        _Button = GetComponent<Button>();
        if( _data == null )
        {
            Debug.Log("ReserveNicknameButton::Awake : data nullref.");
        }
        if(_Button == null )
        {
            Debug.Log("ReserveNicknameButton::Awake : button nullref.");
        }
    }
    public async void OnClick()
    {
        if(_data == null )
        {
            return;
        }

        if(_Button == null)
        {
            return;
        }

        //_Button.interactable = false;

        string str;

        try
        {
            str = _data.GetData();
        }
        catch (Exception e)
        {
            Debug.Log($"ReserveNicknameButton::OnClick : {e.Message}");
            //_Button.interactable = true;
            return;
        }

        string req = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.RESERVE_CHAR_NAME, str);

        await NetworkManager.Instance.SendMsg(req);

        return;
    }
}
