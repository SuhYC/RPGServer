using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// 회원가입버튼. <br/>
/// 회원가입을 위해 입력한 정보를 담고있는 SignUpData로부터 정보를 가져와 서버에 요청한다.
/// </summary>
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

    /// <summary>
    /// SignUpData로부터 정보를 가져와 서버에 요청한다.
    /// </summary>
    public async void OnClick()
    {
        if (signUpData == null)
        {
            Debug.Log("SignUpButton::OnClick : No SignUpData Instance.");
            return;
        }

        string str;

        try
        {
            str = signUpData.GetData();
        }
        catch(CustomException.NotSelectedException e)
        {
            Debug.Log($"SignUpButton::OnClick : {e.Message}");
            TextMessage.CreateTextPanel(e.Message);
            return;
        }
        catch(CustomException.NotFoundInputObjectException e)
        {
            Debug.Log($"SignUpButton::OnClick : {e.Message}");
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

        return;
    }
}
