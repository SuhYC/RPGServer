using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// 해당 프로젝트를 위해 임의로 작성한 사용자정의예외에 대한 정보가 담긴 스크립트.
/// </summary>
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

    public class NotFoundInCharlistException : Exception
    {
        public NotFoundInCharlistException() : base("CharSlot Not Found.") { }
    }

    public class CantGetMapCodeException : Exception
    {
        public CantGetMapCodeException() : base("Cant Get Map Code.") { }
    }
}
