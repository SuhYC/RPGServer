using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;


/// <summary>
/// CheckNicknamePanel에 설명 참조.
/// 
/// 예약 -> 재확인창 -> 생성 혹은 거부
/// 윗 단계 중에 예약을 수행하는 버튼의 스크립트이다.
/// 
/// 캐릭터 생성화면에서 닉네임을 입력한 후 바로 오른쪽에 있는 생성버튼.
/// 닉네임 예약을 서버에 요청하는 버튼.
/// 이후 온 응답을 처리하여 생성가능하면 CheckNickPanel을 생성한다.
/// 생성불가하면 TextMessage만 출력한다.
/// 
/// </summary>
public class ReserveNicknameButton : MonoBehaviour
{
    private CreateCharData _data;
    private Button _Button;

    private void Awake()
    {
        _data = transform.parent?.GetComponent<CreateCharData>();
        _Button = GetComponent<Button>();
        if( _data == null )
        {
            Debug.Log("ReserveNicknameButton::Awake : data nullref.");
        }
        if(_Button == null )
        {
            Debug.Log("ReserveNicknameButton::Awake : button nullref.");
        }
    }
    public async void OnClick()
    {
        if(_data == null )
        {
            return;
        }

        if(_Button == null)
        {
            return;
        }

        //_Button.interactable = false;

        string str;

        try
        {
            str = _data.GetData();
        }
        catch (Exception e)
        {
            Debug.Log($"ReserveNicknameButton::OnClick : {e.Message}");
            //_Button.interactable = true;
            return;
        }

        string req = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.RESERVE_CHAR_NAME, str);

        await NetworkManager.Instance.SendMsg(req);

        return;
    }
}
