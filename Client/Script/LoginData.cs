using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LoginData : MonoBehaviour
{
    TextMesh IDInput;
    TextMesh PWInput;

    // Start is called before the first frame update
    void Awake()
    {
        IDInput = transform.GetChild(0)?.GetChild(0)?.GetComponent<TextMesh>();
        PWInput = transform.GetChild(0)?.GetChild(1)?.GetComponent<TextMesh>();

        if (IDInput == null)
        {
            LogParents("IDInput�� ã�� �� �����ϴ�.");
        }
        if (PWInput == null)
        {
            LogParents("PWInput�� ã�� �� �����ϴ�.");
        }
    }

    public string GetID()
    {
        return IDInput.text;
    }

    public string GetPW()
    {
        return PWInput.text;
    }

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
