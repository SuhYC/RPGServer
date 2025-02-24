using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// �ش� ������Ʈ�� ���� ���Ƿ� �ۼ��� ��������ǿ��ܿ� ���� ������ ��� ��ũ��Ʈ.
/// </summary>
public class CustomException : MonoBehaviour
{
    /// <summary>
    /// QuestNo�� ���õ��� ����
    /// </summary>
    public class NotSelectedException : Exception
    {
        public NotSelectedException(string str) : base(str) { }
    }

    /// <summary>
    /// �Է°�ü�� ã�� �� ����.
    /// </summary>
    public class NotFoundInputObjectException : Exception
    {
        public NotFoundInputObjectException() : base("InputObject Not Found.") { }
    }

    public class NotFoundInCharlistException : Exception
    {
        public NotFoundInCharlistException() : base("CharSlot Not Found.") { }
    }

    public class CantGetMapCodeException : Exception
    {
        public CantGetMapCodeException() : base("Cant Get Map Code.") { }
    }
}
