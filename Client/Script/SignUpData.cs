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
    
    /// <summary>
    /// QuestNo가 선택되지 않음
    /// </summary>
    public class NotSelectedException : Exception
    {
        public NotSelectedException() : base("QuestNo Not Selected.") { }
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
    /// QuestNo를 선택하지 않으면 예외 발생.
    /// </summary>
    /// <returns></returns>
    /// <exception cref="NotSelectedException">QuestNo를 선택하지 않으면 예외 발생</exception>
    public string GetData()
    {
        _param.ID = IDInput.text;
        _param.PW = PWInput.text;
        _param.QuestNo = QuestNoDropdown.value;
        _param.Answer = AnsInput.text;
        _param.Hint = HintInput.text;

        if(_param.QuestNo == 0)
        { 
            throw new NotSelectedException();
        }

        string ret = JsonUtility.ToJson(_param);
        return ret;
    }
}
