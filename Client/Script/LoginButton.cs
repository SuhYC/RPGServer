using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LoginButton : MonoBehaviour
{
    private LoginData _loginData;

    void Awake()
    {
        _loginData = transform.parent.GetComponent<LoginData>();
        if( _loginData == null )
        {
            Debug.Log("LoginButton::Awake : 로그인데이터를 찾을 수 없습니다.");
        }
    }
    public async void OnClick()
    {
        if( _loginData == null )
        {
            throw new CustomException.NotFoundInputObjectException();
        }

        string str;

        try
        {
            str = _loginData.GetData();
        }
        catch(CustomException.NotSelectedException e)
        {
            Debug.Log($"LoginButton::OnClick : {e.Message}");
            TextMessage.CreateTextPanel(e.Message);
            return;
        }
        catch(CustomException.NotFoundInputObjectException e)
        {
            Debug.Log($"LoginButton::OnClick : {e.Message}");
            return;
        }

        string msg;

        try
        {
            msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.SIGNIN, str);
        }
        catch (Exception e)
        {
            Debug.Log($"LoginButton::OnClick : {e.Message}");
            return;
        }

        await NetworkManager.Instance.SendMsg(msg);

        return;
    }
}
