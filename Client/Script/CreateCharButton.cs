using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

/// <summary>
/// CheckNicknamePanel에 설명 참조.
/// 
/// 예약 -> 재확인창 -> 생성 혹은 거부
/// 위 절차 중의 생성에 해당하는 버튼의 스크립트.
/// 
/// 생성할 캐릭터의 닉네임을 가지고 있으며
/// 버튼이 눌렸을 때, 서버에 생성을 요청한다.
/// </summary>
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

        GameObject checkpanel = transform.parent?.parent?.gameObject;

        if(checkpanel != null)
        {
            Destroy(checkpanel);
        }
        else
        {
            Debug.Log($"CreateCharButton::OnClick : nullref.");
        }

        return;
    }
}
