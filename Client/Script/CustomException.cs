using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

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
}
