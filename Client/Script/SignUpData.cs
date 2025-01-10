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
    /// QuestNo�� ���õ��� ����
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
            Debug.Log("SignUpData::Awake : IDInput�� ã�� �� �����ϴ�.");
        }
        PWInput = transform.GetChild(1)?.GetComponent<TMPro.TMP_InputField>();
        if ( PWInput == null )
        {
            Debug.Log("SignUpData::Awake : PWInput�� ã�� �� �����ϴ�.");
        }
        QuestNoDropdown = transform.GetChild(2)?.GetComponent<TMPro.TMP_Dropdown>();
        if (QuestNoDropdown == null)
        {
            Debug.Log("SignUpData::Awake : Recovery Question Dropdown�� ã�� �� �����ϴ�.");
        }
        AnsInput = transform.GetChild(3)?.GetComponent<TMPro.TMP_InputField>();
        if( AnsInput == null )
        {
            Debug.Log("SignUpData::Awake : AnsInput�� ã�� �� �����ϴ�.");
        }
        HintInput = transform.GetChild(4)?.GetComponent<TMPro.TMP_InputField>();
        if (HintInput == null)
        {
            Debug.Log("SignUpData::Awake : HintInput�� ã�� �� �����ϴ�.");
        }

        _param = new SignUpParameter();
    }

    /// <summary>
    /// QuestNo�� �������� ������ ���� �߻�.
    /// </summary>
    /// <returns></returns>
    /// <exception cref="NotSelectedException">QuestNo�� �������� ������ ���� �߻�</exception>
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
