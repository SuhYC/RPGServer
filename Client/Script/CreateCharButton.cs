using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class CreateCharButton : MonoBehaviour
{
    private string _data;

    public void Init(string data)
    {
        _data = data;
        return;
    }

    public async void OnClick()
    {
        string req = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.CREATE_CHAR, _data);

        await NetworkManager.Instance.SendMsg(req);

        return;
    }
}
