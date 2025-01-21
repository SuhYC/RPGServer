using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

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

    public Charlist(int size)
    {
        _charcodes = new int[size];
    }

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
