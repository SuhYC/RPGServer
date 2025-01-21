using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CancelReserveNicknameButton : MonoBehaviour
{
    private string _data;

    public void Init(string data)
    {
        _data = data;
        return;
    }

    public async void OnClick()
    {
        string req = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.CANCEL_CHAR_NAME_RESERVE, _data);

        await NetworkManager.Instance.SendMsg(req);

        return;
    }
}
