using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SignUpButton : MonoBehaviour
{
    SignUpData signUpData;

    void Awake()
    {
        signUpData = transform.parent.parent.GetChild(0)?.GetComponent<SignUpData>();

        if(signUpData == null)
        {
            Debug.Log("SignUpData를 찾을 수 없음.");
        }
    }

    public async void OnClick()
    {
        string str;
        if (signUpData == null)
        {
            Debug.Log("SignUpButton::OnClick : No SignUpData Instance.");
            return;
        }

        try
        {
            str = signUpData.GetData();
        }
        catch(SignUpData.NotSelectedException)
        {
            Debug.Log("SignUpButton::OnClick : QuestNo. Not Selected.");
            return;
        }

        string msg;

        try
        {
            msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.SIGNUP, str);
        }
        catch(Exception e)
        {
            Debug.Log($"SignUpButton::OnClick : {e.Message}");
            return;
        }

        await NetworkManager.Instance.SendMsg(msg);
    }
}
