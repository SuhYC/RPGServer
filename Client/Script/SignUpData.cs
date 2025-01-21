using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;


public class SignUpData : MonoBehaviour
{
    TMPro.TMP_InputField IDInput;
    TMPro.TMP_InputField PWInput;
    TMPro.TMP_Dropdown QuestNoDropdown;
    TMPro.TMP_InputField AnsInput;
    TMPro.TMP_InputField HintInput;

    private SignUpParameter _param;

    [System.Serializable]
    public class SignUpParameter
    {
        public string ID;
        public string PW;
        public int QuestNo;
        public string Answer;
        public string Hint;
    }
    
    

    void Awake()
    {
        IDInput = transform.GetChild(0)?.GetComponent<TMPro.TMP_InputField>();
        if (IDInput == null )
        {
            Debug.Log(transform.GetChild(0).name);
            Debug.Log("SignUpData::Awake : IDInput을 찾을 수 없습니다.");
        }
        PWInput = transform.GetChild(1)?.GetComponent<TMPro.TMP_InputField>();
        if ( PWInput == null )
        {
            Debug.Log("SignUpData::Awake : PWInput을 찾을 수 없습니다.");
        }
        QuestNoDropdown = transform.GetChild(2)?.GetComponent<TMPro.TMP_Dropdown>();
        if (QuestNoDropdown == null)
        {
            Debug.Log("SignUpData::Awake : Recovery Question Dropdown을 찾을 수 없습니다.");
        }
        AnsInput = transform.GetChild(3)?.GetComponent<TMPro.TMP_InputField>();
        if( AnsInput == null )
        {
            Debug.Log("SignUpData::Awake : AnsInput을 찾을 수 없습니다.");
        }
        HintInput = transform.GetChild(4)?.GetComponent<TMPro.TMP_InputField>();
        if (HintInput == null)
        {
            Debug.Log("SignUpData::Awake : HintInput을 찾을 수 없습니다.");
        }

        _param = new SignUpParameter();
    }


    /// <summary>
    /// 
    /// </summary>
    /// <returns></returns>
    /// <exception cref="CustomException.NotFoundInputObjectException">입력객체를 찾을 수 없음.</exception>
    /// <exception cref="CustomException.NotSelectedException">잘못된 입력</exception>
    public string GetData()
    {
        if(IDInput == null || PWInput == null || AnsInput == null || HintInput == null || QuestNoDropdown == null)
        {
            Debug.Log($"SignUpData::GetData : 입력객체를 찾을 수 없습니다.");
            throw new CustomException.NotFoundInputObjectException();
        }

        _param.ID = IDInput.text;
        _param.PW = PWInput.text;
        _param.QuestNo = QuestNoDropdown.value;
        _param.Answer = AnsInput.text;
        _param.Hint = HintInput.text;

        if(_param.ID.Length < 3 || _param.ID.Length > 10 || !IsValidStr(_param.ID))
        {
            throw new CustomException.NotSelectedException("ID는 3글자 이상 10글자 이하의 영문자와 숫자로 구성해주세요.");
        }

        if(_param.PW.Length < 3 || _param.PW.Length > 10 || !IsValidStr(_param.PW))
        {
            throw new CustomException.NotSelectedException("비밀번호는 3글자 이상 10글자 이하 영문자와 숫자로 구성해주세요.");
        }

        if(_param.QuestNo == 0)
        { 
            throw new CustomException.NotSelectedException("비밀번호 찾기 질문을 선택해주세요.");
        }

        if(_param.Answer == string.Empty)
        {
            throw new CustomException.NotSelectedException("비밀번호 찾기 질문에 대한 답변을 입력해주세요.");
        }

        if(_param.Answer.Length > 10)
        {
            throw new CustomException.NotSelectedException("비밀번호 찾기 질문에 대한 답변은 10자 이하로 입력해주세요.");
        }

        if(_param.Hint == string.Empty)
        {
            throw new CustomException.NotSelectedException("비밀번호 찾기 질문의 힌트를 입력해주세요.");
        }

        if(_param.Hint.Length > 10)
        {
            throw new CustomException.NotSelectedException("비밀번호 찾기 질문의 힌트는 10자 이하로 입력해주세요.");
        }

        string ret = JsonUtility.ToJson(_param);
        return ret;
    }

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
}
