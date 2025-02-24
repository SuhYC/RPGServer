using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

/// <summary>
/// 캐릭터 생성을 위해 닉네임을 적는 InputField를 위한 스크립트.
/// InputField에 입력한 값을 가지고 다른 버튼과 상호작용한다.
/// </summary>
public class CreateCharData : MonoBehaviour
{
    private TMPro.TMP_InputField _charname;
    private CreateCharParam _data;
    private class CreateCharParam
    {
        public string CharName;
    }

    private void Awake()
    {
        _charname = transform.GetChild(0)?.GetComponent<TMPro.TMP_InputField>();

        if(_charname == null )
        {
            Debug.Log("CreateCharData::Awake : nullref.");
        }

        _data = new CreateCharParam();
    }

    /// <summary>
    /// InputField nullrefexcept 포함.
    /// </summary>
    /// <returns></returns>
    /// <exception cref="NullReferenceException"></exception>
    public string GetData()
    {
        if(_charname == null )
        {
            throw new NullReferenceException("CreateCharData::GetData : nullref.");
        }

        _data.CharName = _charname.text;

        return JsonUtility.ToJson(_data);
    }
}
