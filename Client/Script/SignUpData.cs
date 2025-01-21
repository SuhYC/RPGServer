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
    /// 
    /// </summary>
    /// <returns></returns>
    /// <exception cref="CustomException.NotFoundInputObjectException">�Է°�ü�� ã�� �� ����.</exception>
    /// <exception cref="CustomException.NotSelectedException">�߸��� �Է�</exception>
    public string GetData()
    {
        if(IDInput == null || PWInput == null || AnsInput == null || HintInput == null || QuestNoDropdown == null)
        {
            Debug.Log($"SignUpData::GetData : �Է°�ü�� ã�� �� �����ϴ�.");
            throw new CustomException.NotFoundInputObjectException();
        }

        _param.ID = IDInput.text;
        _param.PW = PWInput.text;
        _param.QuestNo = QuestNoDropdown.value;
        _param.Answer = AnsInput.text;
        _param.Hint = HintInput.text;

        if(_param.ID.Length < 3 || _param.ID.Length > 10 || !IsValidStr(_param.ID))
        {
            throw new CustomException.NotSelectedException("ID�� 3���� �̻� 10���� ������ �����ڿ� ���ڷ� �������ּ���.");
        }

        if(_param.PW.Length < 3 || _param.PW.Length > 10 || !IsValidStr(_param.PW))
        {
            throw new CustomException.NotSelectedException("��й�ȣ�� 3���� �̻� 10���� ���� �����ڿ� ���ڷ� �������ּ���.");
        }

        if(_param.QuestNo == 0)
        { 
            throw new CustomException.NotSelectedException("��й�ȣ ã�� ������ �������ּ���.");
        }

        if(_param.Answer == string.Empty)
        {
            throw new CustomException.NotSelectedException("��й�ȣ ã�� ������ ���� �亯�� �Է����ּ���.");
        }

        if(_param.Answer.Length > 10)
        {
            throw new CustomException.NotSelectedException("��й�ȣ ã�� ������ ���� �亯�� 10�� ���Ϸ� �Է����ּ���.");
        }

        if(_param.Hint == string.Empty)
        {
            throw new CustomException.NotSelectedException("��й�ȣ ã�� ������ ��Ʈ�� �Է����ּ���.");
        }

        if(_param.Hint.Length > 10)
        {
            throw new CustomException.NotSelectedException("��й�ȣ ã�� ������ ��Ʈ�� 10�� ���Ϸ� �Է����ּ���.");
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
