using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;


/// <summary>
/// Login에 필요한 입력을 받는 오브젝트들을 연결하여 정보를 하나의 문자열로 만드는 스크립트.
/// </summary>
public class LoginData : MonoBehaviour
{
    TMP_InputField IDInput;
    TMP_InputField PWInput;
    LoginParameter _param;

    public class LoginParameter
    { 
        public string ID;
        public string PW;
    }


    void Awake()
    {
        IDInput = transform.GetChild(0)?.GetChild(0)?.GetComponent<TMP_InputField>();
        PWInput = transform.GetChild(0)?.GetChild(1)?.GetComponent<TMP_InputField>();

        if (IDInput == null)
        {
            LogParents("IDInput을 찾을 수 없습니다.");
        }
        if (PWInput == null)
        {
            LogParents("PWInput을 찾을 수 없습니다.");
        }

        _param = new LoginParameter();
    }

    /// <summary>
    /// 입력 오브젝트 들로부터 정보를 가져와 적합여부를 판단한 후 <br/>
    /// JSON문자열로 만들어 반환한다.
    /// </summary>
    /// <returns></returns>
    /// <exception cref="CustomException.NotFoundInputObjectException"></exception>
    /// <exception cref="CustomException.NotSelectedException"></exception>
    public string GetData()
    {
        if(IDInput == null || PWInput == null)
        {
            throw new CustomException.NotFoundInputObjectException();
        }

        if(IDInput.text.Length < 3 || IDInput.text.Length > 10 || !IsValidStr(IDInput.text))
        {
            throw new CustomException.NotSelectedException("ID는 3글자 이상 10글자 이하의 영문자와 숫자로 구성해주세요.");
        }

        if(PWInput.text.Length < 3 || PWInput.text.Length > 10 || !IsValidStr(PWInput.text))
        {
            throw new CustomException.NotSelectedException("비밀번호는 3글자 이상 10글자 이하의 영문자와 숫자로 구성해주세요.");
        }

        _param.ID = IDInput.text;
        _param.PW = PWInput.text;

        return JsonUtility.ToJson(_param);
    }


    /// <summary>
    /// 영문 대소문자, 숫자로 이루어져 있는지 판단한다.
    /// </summary>
    /// <param name="str"></param>
    /// <returns>true : 영문 혹은 숫자로 구성된 문자열임.<br/> false : 영문,숫자를 제외한 문자가 포함되어있음.</returns>
    private bool IsValidStr(string str)
    {
        for (int i = 0; i < str.Length; i++)
        {
            if (str[i] > 'z' ||
                (str[i] < 'a' && str[i] > 'Z') ||
                (str[i] < 'A' && str[i] > '9') ||
                str[i] < '0')
            {
                return false;
            }
        }
        return true;
    }


    /// <summary>
    /// 디버깅을 위해 작성.
    /// 상위 하이어라키를 모두 출력한다.
    /// 앞에 오류메시지msg를 붙여 출력.
    /// </summary>
    /// <param name="msg">오류메시지</param>
    private void LogParents(string msg = "")
    {
        Transform obj = transform;
        string str = obj.name;

        while (obj.parent != null)
        {
            str = obj.parent.name + "->" + str;
        }
        
        Debug.Log(msg + ": " + str);
    }
}
