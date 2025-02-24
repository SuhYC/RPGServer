using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// 오로지 디버깅용으로 정보를 수정하는 요청을 서버로 보내는 경우에 쓰이는 스크립트.
/// 이후 배포시 당연히 제거할 것.
/// 서버 파트도 제거하는 것이 좋다.
/// </summary>
public class DebugTool : MonoBehaviour
{
    
    /// <summary>
    /// 보유한 골드를 10000으로 설정하는 요청을 서버로 보냄.
    /// </summary>
    public async void SetGold()
    {
        string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.DEBUG_SET_GOLD, string.Empty);
        
        await NetworkManager.Instance.SendMsg(msg);
    }
}
