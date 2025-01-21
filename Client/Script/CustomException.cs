using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CustomException : MonoBehaviour
{
    /// <summary>
    /// QuestNo가 선택되지 않음
    /// </summary>
    public class NotSelectedException : Exception
    {
        public NotSelectedException(string str) : base(str) { }
    }

    /// <summary>
    /// 입력객체를 찾을 수 없음.
    /// </summary>
    public class NotFoundInputObjectException : Exception
    {
        public NotFoundInputObjectException() : base("InputObject Not Found.") { }
    }
}
