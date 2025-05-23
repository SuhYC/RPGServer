using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Net;
using Unity.VisualScripting;


/// <summary>
/// 네트워크 송수신 처리를 위한 스크립트.
/// 
/// MTU로 인한 분할 문제를 해결하기 위해
/// 전송할 메시지의 크기를 헤더로 부착하여 송수신한다.
/// 
/// '[' + size + ']' + payload
/// 
/// 수신한 메시지의 헤더를 제거하는건 ResHandler가 수행한다.
/// </summary>
public class NetworkManager : MonoBehaviour
{
    private const int port = 12345;
    private const string serverIp = "127.0.0.1";
    private NetworkStream _stream;
    private TcpClient _tcpClient;

    private bool m_IsRun;

    private byte[] _buffer;

    private RingBuffer _SendBuffer;
    private byte[] _SendingData;

    private static NetworkManager _instance;

    public static NetworkManager Instance
    {
        get
        {
            if(_instance == null)
            {
                _instance = FindAnyObjectByType<NetworkManager>();
                if(_instance == null)
                {
                    GameObject netManagerObj = new GameObject("NetworkManager");
                    _instance = netManagerObj.AddComponent<NetworkManager>();
                }
            }
            return _instance;
        }
    }

    async void Awake()
    {
        if( _instance != null && _instance != this)
        {
            Destroy(this.gameObject);
            return;
        }
        else
        {
            _instance = this;
            DontDestroyOnLoad(this.gameObject);
        }

        _buffer = new byte[1024];
        _SendingData = new byte[1024];
        _SendBuffer = new RingBuffer();

        await ConnectToTcpServer(serverIp, port);
    }

    /// <summary>
    /// 서버 연결.
    /// 이후 서버응답수신함수RecvMsg()를 호출한다. RecvMsg는 연결이 끊어질 때까지 내부에서 반복.
    /// </summary>
    /// <param name="host"></param>
    /// <param name="port"></param>
    /// <returns></returns>
    private async Task ConnectToTcpServer(string host, int port)
    {
        try
        {
            _tcpClient = new TcpClient();
            await _tcpClient.ConnectAsync(host, port);
            _stream = _tcpClient.GetStream();
        }
        catch(Exception e)
        {
            Debug.Log($"NetworkManager::ConnectToTcpServer : 연결 오류. {e.Message}");
            return;
        }

        Debug.Log($"NetworkManager::ConnectToTcpServer : 연결 완료");

        m_IsRun = true;


        await Task.WhenAll(RecvMsg(), SendIO());
        //await RecvMsg();

        return;
    }

    private void OnApplicationQuit()
    {
        End();
    }

    public void End()
    {
        m_IsRun = false;

        _stream?.Close();
        _tcpClient?.Close();
    }

    /// <summary>
    /// PacketMaker.instance.ToReqMessage를 통해 만든 패킷을 전송할 것.
    /// </summary>
    /// <param name="msg">jsonstr of ReqMessage</param>
    /// <returns></returns>
    public async Task SendMsg(string msg)
    {
        if(_tcpClient != null && _tcpClient.Connected)
        {
            byte[] tmp = Encoding.GetEncoding("euc-kr").GetBytes(msg);

            int size = tmp.Length;

            byte[] data = new byte[size + sizeof(uint)];

            data[0] = (byte)(size & 0xFF);
            data[1] = (byte)((size >> 8) & 0xFF);
            data[2] = (byte)((size >> 16) & 0xFF);
            data[3] = (byte)((size >> 24) & 0xFF);

            Buffer.BlockCopy(tmp, 0, data, sizeof(uint), size);

            await _SendBuffer.EnqueueWithLock(data, size + sizeof(uint));
        }
    }

    private async Task SendIO()
    {
        while (m_IsRun)
        {
            int size = await _SendBuffer.DequeueWithLock(_SendingData);

            if (size == 0)
            {
                await Task.Delay(30);
                continue;
            }

            await _stream.WriteAsync(_SendingData, 0, size);
        }
    }

    /// <summary>
    /// 지속적으로 서버로부터의 응답을 체크.
    /// </summary>
    /// <returns></returns>
    private async Task RecvMsg()
    {
        try
        {
            while (true)
            {
                int bytesRead = await _stream.ReadAsync(_buffer);
                if (bytesRead > 0)
                {
                    //string receivedData = Encoding.GetEncoding("euc-kr").GetString(_buffer, 0, bytesRead);

                    //Debug.Log($"NetworkManager::RecvMsg : 수신: {receivedData}");
                    //await ResHandler.Instance.HandlePacketByte(receivedData);
                    await ResHandler.Instance.HandlePacket(_buffer, bytesRead);
                }
                // 연결종료, 오류 발생
                else
                {
                    break;
                }
            }
        }
        catch (Exception e)
        {
            Debug.Log($"NetworkManager::RecvMsg : {e.Message}");
        }
    }

}
