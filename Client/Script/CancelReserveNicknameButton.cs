using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// CheckNicknamePanel�� ���� ����.
/// 
/// ���� -> ��Ȯ��â -> ���� Ȥ�� �ź�
/// �� ���� ���� �źο� �ش��ϴ� ��ư�� ��ũ��Ʈ�̴�.
/// 
/// ������ ĳ������ �г����� ������ ������
/// �ش� ��ư�� ������ �ش� �г����� ������ ������ ���� ������ ��û�Ѵ�.
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
