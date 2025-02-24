using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// CheckNicknamePanel에 설명 참조.
/// 
/// 예약 -> 재확인창 -> 생성 혹은 거부
/// 위 절차 중의 거부에 해당하는 버튼의 스크립트이다.
/// 
/// 생성할 캐릭터의 닉네임을 가지고 있으며
/// 해당 버튼이 눌리면 해당 닉네임의 예약을 해제할 것을 서버에 요청한다.
/// </summary>
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
