using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;


/// <summary>
/// Login�� �ʿ��� �Է��� �޴� ������Ʈ���� �����Ͽ� ������ �ϳ��� ���ڿ��� ����� ��ũ��Ʈ.
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
            LogParents("IDInput�� ã�� �� �����ϴ�.");
        }
        if (PWInput == null)
        {
            LogParents("PWInput�� ã�� �� �����ϴ�.");
        }

        _param = new LoginParameter();
    }

    /// <summary>
    /// �Է� ������Ʈ ��κ��� ������ ������ ���տ��θ� �Ǵ��� �� <br/>
    /// JSON���ڿ��� ����� ��ȯ�Ѵ�.
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
            throw new CustomException.NotSelectedException("ID�� 3���� �̻� 10���� ������ �����ڿ� ���ڷ� �������ּ���.");
        }

        if(PWInput.text.Length < 3 || PWInput.text.Length > 10 || !IsValidStr(PWInput.text))
        {
            throw new CustomException.NotSelectedException("��й�ȣ�� 3���� �̻� 10���� ������ �����ڿ� ���ڷ� �������ּ���.");
        }

        _param.ID = IDInput.text;
        _param.PW = PWInput.text;

        return JsonUtility.ToJson(_param);
    }


    /// <summary>
    /// ���� ��ҹ���, ���ڷ� �̷���� �ִ��� �Ǵ��Ѵ�.
    /// </summary>
    /// <param name="str"></param>
    /// <returns>true : ���� Ȥ�� ���ڷ� ������ ���ڿ���.<br/> false : ����,���ڸ� ������ ���ڰ� ���ԵǾ�����.</returns>
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
    /// ������� ���� �ۼ�.
    /// ���� ���̾��Ű�� ��� ����Ѵ�.
    /// �տ� �����޽���msg�� �ٿ� ���.
    /// </summary>
    /// <param name="msg">�����޽���</param>
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
