using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading.Tasks;
using UnityEngine;
using static PacketMaker;

public class ResHandler : MonoBehaviour
{
    private static ResHandler _instance;
    private readonly AsyncLock _lockObject = new AsyncLock();
    private string _remainMsg;


    public static ResHandler Instance
    {
        get
        {
            if (_instance == null)
            {
                _instance = FindFirstObjectByType<ResHandler>();
                if (_instance == null)
                {
                    GameObject packetManagerObj = new GameObject("ResultManager");
                    DontDestroyOnLoad(packetManagerObj);
                    _instance = packetManagerObj.AddComponent<ResHandler>();
                }
            }
            return _instance;
        }
    }

    void Awake()
    {
        if (_instance == null)
        {
            _instance = this;
            this.Init();
        }
        else if (_instance != this)
        {
            Destroy(this.gameObject);
        }
    }

    private void Init()
    {
        DontDestroyOnLoad(this.gameObject);

        _remainMsg = string.Empty;
    }

    public async Task HandlePacket(string msg)
    {
        using (await _lockObject.LockAsync())
        {
            string str = _remainMsg + msg;
            _remainMsg = string.Empty;

            if (str[0] != '[')
            {
                return;
            }

            int index = str.IndexOf("]");

            if (index < 2)
            {
                Debug.Log($"ResHandler::HandlePacket : Wrong Msg. {msg}");
                return;
            }

            string header = str.Substring(1, index - 1);

            try // 뭉쳐 온 데이터 일괄 처리 필요
            {
                while(true)
                {
                    int size = int.Parse(header);

                    if (str.Length > size + index)
                    {
                        string req = str.Substring(index + 1, size);
                        str = str.Substring(size + index + 1);

                        await PacketMaker.instance.HandleServerResponse(req);
                    }
                    else
                    {
                        break;
                    }
                }

                _remainMsg = str;
            }
            catch (Exception e)
            {
                Debug.Log($"ResHandler::HandlePacket : {e.Message}");
            }
        }
    }
}
