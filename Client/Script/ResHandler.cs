using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading.Tasks;
using UnityEngine;
using static PacketMaker;


/// <summary>
/// 서버로부터 수신한 메시지에서 페이로드 부분만 분리하는 클래스. <br/>
/// 수신메시지의 포맷은 '[' + size + ']' + ResJsonStr 이다. <br/>
/// [size]부분의 헤더처리를 끝내고 패킷단위로 분리하여 ResJsonStr을 뽑아내고, <br/>
/// 해당 JsonStr을 PacketMaker의 HandleServerResponse로 넘겨 처리한다.
/// 
/// 수신한 메시지는 MTU에 의해 Layer2를 통과하기 위해 분할되었을 수 있다. <br/>
/// 수신한 메시지는 메시지 버퍼(현재는 단순하게 string 변수 하나에 계속 합쳐 저장한다.)에 합치고 <br/>
/// 합쳐진 메시지가 payload부분이 size를 충분히 넘기는지 확인하여 충분한 사이즈가 되었다면 분리한다. <br/>
/// 
/// 한번의 수신처리가 끝나기 전까지 다른 실행흐름이 방해하면 안되므로 AsyncLock클래스의 SemaphoreSlim을 사용한다. <br/>
/// 메인스레드 하나로 구성했더라도 Task단위로 생각하면 두개 이상의 실행흐름이 접근할 수 있는 임계영역이다. <br/>
/// 
/// 수신한 메시지가 분할되는 것 뿐만 아니라 여러개의 패킷이 합쳐져서 오는 경우도 생각하여야한다. <br/>
/// 서버측에서 원형큐를 이용한 링버퍼를 구현해두었다. <br/>
/// 일단 링버퍼에 데이터를 담고 송신요청을 하고 송신요청이 마무리되면 링버퍼에 담긴 다른 데이터를 확인하기 때문에 <br/>
/// 송신오버헤드로 인해 다수의 패킷이 링버퍼에 담겼다가 한번에 송신될 수 있다. <br/>
/// 그러므로 반복문을 통해 헤더를 제거하는 과정을 여러번 호출해준다.
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
    /// 수신한 메시지의 헤더를 제거하는 함수. <br/>
    /// AsyncLock을 사용하여 복수의 Task가 접근하지 않도록 상호배제한다. <br/>
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

                    // ]를 찾지 못함
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
}
