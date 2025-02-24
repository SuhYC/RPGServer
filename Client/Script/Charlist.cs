using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// CharCode��� ������ �迭
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
                throw new ArgumentNullException("�迭�� null�� �� �����ϴ�.");
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
    /// �迭 ���� �����ϳ� �߰�.
    /// size+1�� ������ ���� �Ҵ��ϰ� �����ϱ� ������ ��ȿ����������
    /// ���� �Ҹ� �޼ҵ�� �ƴѰ� ���Ƽ� �̷��� ����.
    /// </summary>
    /// <param name="data"></param>
    public void Add(int data)
    {
        Array.Resize<int>(ref _charcodes, _charcodes.Length + 1);
        
        _charcodes[^1] = data; // C# 8.0 �̻�. System.Index
    }

    public Charlist(int size)
    {
        _charcodes = new int[size];
    }


    /// <summary>
    /// [�̻��]
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
