using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

/// <summary>
/// CheckNicknamePanel�� ���� ����.
/// 
/// ���� -> ��Ȯ��â -> ���� Ȥ�� �ź�
/// �� ���� ���� ������ �ش��ϴ� ��ư�� ��ũ��Ʈ.
/// 
/// ������ ĳ������ �г����� ������ ������
/// ��ư�� ������ ��, ������ ������ ��û�Ѵ�.
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
