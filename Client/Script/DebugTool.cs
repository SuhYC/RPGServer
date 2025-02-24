using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// ������ ���������� ������ �����ϴ� ��û�� ������ ������ ��쿡 ���̴� ��ũ��Ʈ.
/// ���� ������ �翬�� ������ ��.
/// ���� ��Ʈ�� �����ϴ� ���� ����.
/// </summary>
public class DebugTool : MonoBehaviour
{
    
    /// <summary>
    /// ������ ��带 10000���� �����ϴ� ��û�� ������ ����.
    /// </summary>
    public async void SetGold()
    {
        string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.DEBUG_SET_GOLD, string.Empty);
        
        await NetworkManager.Instance.SendMsg(msg);
    }
}
