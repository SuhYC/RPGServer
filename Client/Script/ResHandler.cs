using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;
using static PacketMaker;


/// <summary>
/// �����κ��� ������ �޽������� ���̷ε� �κи� �и��ϴ� Ŭ����. <br/>
/// ���Ÿ޽����� ������ '[' + size + ']' + ResJsonStr �̴�. <br/>
/// [size]�κ��� ���ó���� ������ ��Ŷ������ �и��Ͽ� ResJsonStr�� �̾Ƴ���, <br/>
/// �ش� JsonStr�� PacketMaker�� HandleServerResponse�� �Ѱ� ó���Ѵ�.
/// 
/// ������ �޽����� MTU�� ���� Layer2�� ����ϱ� ���� ���ҵǾ��� �� �ִ�. <br/>
/// ������ �޽����� �޽��� ����(����� �ܼ��ϰ� string ���� �ϳ��� ��� ���� �����Ѵ�.)�� ��ġ�� <br/>
/// ������ �޽����� payload�κ��� size�� ����� �ѱ���� Ȯ���Ͽ� ����� ����� �Ǿ��ٸ� �и��Ѵ�. <br/>
/// 
/// �ѹ��� ����ó���� ������ ������ �ٸ� �����帧�� �����ϸ� �ȵǹǷ� AsyncLockŬ������ SemaphoreSlim�� ����Ѵ�. <br/>
/// ���ν����� �ϳ��� �����ߴ��� Task������ �����ϸ� �ΰ� �̻��� �����帧�� ������ �� �ִ� �Ӱ迵���̴�. <br/>
/// 
/// ������ �޽����� ���ҵǴ� �� �Ӹ� �ƴ϶� �������� ��Ŷ�� �������� ���� ��쵵 �����Ͽ����Ѵ�. <br/>
/// ���������� ����ť�� �̿��� �����۸� �����صξ���. <br/>
/// �ϴ� �����ۿ� �����͸� ��� �۽ſ�û�� �ϰ� �۽ſ�û�� �������Ǹ� �����ۿ� ��� �ٸ� �����͸� Ȯ���ϱ� ������ <br/>
/// �۽ſ������� ���� �ټ��� ��Ŷ�� �����ۿ� ���ٰ� �ѹ��� �۽ŵ� �� �ִ�. <br/>
/// �׷��Ƿ� �ݺ����� ���� ����� �����ϴ� ������ ������ ȣ�����ش�.
/// </summary>
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


    /// <summary>
    /// [�̻��]
    /// ������ �޽����� ����� �����ϴ� �Լ�. <br/>
    /// AsyncLock�� ����Ͽ� ������ Task�� �������� �ʵ��� ��ȣ�����Ѵ�. <br/>
    /// </summary>
    /// <param name="msg"></param>
    /// <returns></returns>
    public async Task HandlePacket(string msg)
    {

        using (await _lockObject.LockAsync())
        {
            string str = _remainMsg + msg;
            _remainMsg = string.Empty;
            
            try
            {
                while(true)
                {
                    if(str == string.Empty)
                    {
                        return;
                    }

                    if (str[0] != '[')
                    {
                        return;
                    }

                    int index = str.IndexOf("]");

                    // ]�� ã�� ����
                    if(index == -1)
                    {
                        break;
                    }

                    if (index < 2)
                    {
                        return;
                    }

                    string header = str.Substring(1, index - 1);

                    int size = int.Parse(header);

                    if (str.Length > size + index)
                    {
                        string req = str.Substring(index + 1, size);
                        str = str.Substring(size + index + 1);

                        //Debug.Log($"ResHandler::HandlePacket : remain : {str}");
                        Debug.Log($"ResHandler::HandlerPacket : req : {req}");
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
                Debug.Log($"ResHandler::HandlePacket : {e.Message}, {e.StackTrace}");
                _remainMsg = str;
            }
        }
    }

    /// <summary>
    /// HandlePacket�Լ����� string ������ ��Ŷ���̰���� byte ������ ��Ŷ���� ������� ���� <br/>
    /// c++�� std::string�� C#�� string�� ���ڵ��� �ٸ��� �ѱ۹��ڴ� �Ҵ�� ���̵� �ٸ���.
    /// </summary>
    /// <param name="msg"></param>
    /// <returns></returns>
    public async Task HandlePacketByte(string msg)
    {
        byte[] targetByte = Encoding.UTF8.GetBytes("]");

        using (await _lockObject.LockAsync())
        {
            string str = _remainMsg + msg;

            byte[] bytes = Encoding.GetEncoding("euc-kr").GetBytes(str);
            //byte[] bytes = Encoding.UTF8.GetBytes(str);

            _remainMsg = string.Empty;

            try
            {
                while (true)
                {
                    if (bytes.Length == 0)
                    {
                        return;
                    }

                    if (bytes[0] != '[')
                    {
                        return;
                    }

                    int index = FindByteArray(bytes, targetByte);

                    // ]�� ã�� ����
                    if (index == -1)
                    {
                        break;
                    }

                    if (index < 2)
                    {
                        return;
                    }

                    // bytes�迭�� 1 ~ index-1 �ε����� �����̷� ����
                    byte[] headerByte = bytes.Skip(1).Take(index - 1).ToArray();

                    //string header = Encoding.UTF8.GetString(headerByte);
                    string header = Encoding.GetEncoding("euc-kr").GetString(headerByte);

                    int size = int.Parse(header);

                    if (bytes.Length > size + index)
                    {
                        // ������� ����

                        byte[] subBytes = bytes.Skip(index + 1).Take(size).ToArray();
                        bytes = bytes.Skip(size + index + 1).ToArray();

                        //string req = Encoding.UTF8.GetString(subBytes); // ó�� �޽���
                        //str = Encoding.UTF8.GetString(bytes); // �ܿ� �޽���

                        string req = Encoding.GetEncoding("euc-kr").GetString(subBytes); // ó�� �޽���
                        str = Encoding.GetEncoding("euc-kr").GetString(bytes); // �ܿ� �޽���

                        //Debug.Log($"ResHandler::HandlePacket : remain : {str}");
                        Debug.Log($"ResHandler::HandlerPacket : req : {req}");
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
                Debug.Log($"ResHandler::HandlePacket : {e.Message}, {e.StackTrace}");
                _remainMsg = str;
            }
        }
    }

    static int FindByteArray(byte[] byteArray, byte[] targetByte)
    {
        for (int i = 0; i <= byteArray.Length - targetByte.Length; i++)
        {
            bool match = true;
            for (int j = 0; j < targetByte.Length; j++)
            {
                if (byteArray[i + j] != targetByte[j])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                return i; // ã�� ��ġ�� ��ȯ
            }
        }
        return -1; // ã�� ���� ��� -1 ��ȯ

    }
}
