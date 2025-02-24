using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// Login Scene의 Login을 요청하는 버튼에 부착되는 스크립트.
/// 
/// 버튼이 클릭되면 LoginData의 정보를 받아와 서버에 요청한다.
/// </summary>
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
