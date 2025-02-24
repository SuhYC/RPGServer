using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// CharCode들로 구성된 배열
/// </summary>
public struct Charlist
{
    private int[] _charcodes;

    public int[] array
    {
        get { return _charcodes; }
        set
        {
            if(value == null)
            {
                throw new ArgumentNullException("배열은 null일 수 없습니다.");
            }
            _charcodes = value;
        }
    }

    public int this[int index]
    {
        get
        {
            return _charcodes[index];
        }

        set
        {
            if(index >= _charcodes.Length)
            {
                Array.Resize<int>(ref _charcodes, index + 1);
            }
            _charcodes[index] = value;
        }
    }

    /// <summary>
    /// 배열 끝에 원소하나 추가.
    /// size+1의 공간을 새로 할당하고 복사하기 때문에 비효율적이지만
    /// 자주 불릴 메소드는 아닌거 같아서 이렇게 설정.
    /// </summary>
    /// <param name="data"></param>
    public void Add(int data)
    {
        Array.Resize<int>(ref _charcodes, _charcodes.Length + 1);
        
        _charcodes[^1] = data; // C# 8.0 이상. System.Index
    }

    public Charlist(int size)
    {
        _charcodes = new int[size];
    }


    /// <summary>
    /// [미사용]
    /// For Debug.
    /// </summary>
    public void PrintList()
    {
        if(array == null)
        {
            Debug.Log($"CharList::PrintList : null array");
            return;
        }

        Debug.Log($"Charlist::PrintList : length : {_charcodes.Length}");
        for (int i = 0; i < _charcodes.Length; i++)
        {
            Debug.Log($"Charlist::PrintList : [{i}] : {_charcodes[i]}");
        }
        return;
    }

}
