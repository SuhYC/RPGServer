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
    private byte[] _buffer;

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

        await RecvMsg();

        return;
    }

    private void OnApplicationQuit()
    {
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
            //byte[] tmp = Encoding.UTF8.GetBytes(msg);
            byte[] tmp = Encoding.GetEncoding("euc-kr").GetBytes(msg);

            // 송신크기 헤더
            string[] strings = { "[", tmp.Length.ToString(), "]", msg };
            msg = string.Join("", strings); //msg = "[" + tmp.Length.ToString() + "]" + msg;

            //byte[] data = Encoding.UTF8.GetBytes(msg);
            byte[] data = Encoding.GetEncoding("euc-kr").GetBytes(msg);

            await _stream.WriteAsync(data, 0, data.Length);
            Debug.Log($"NetworkManager::SendMsg : 데이터 전송: {msg}");
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
                    //string receivedData = Encoding.UTF8.GetString(_buffer, 0, bytesRead);
                    string receivedData = Encoding.GetEncoding("euc-kr").GetString(_buffer, 0, bytesRead);

                    Debug.Log($"NetworkManager::RecvMsg : 수신: {receivedData}");
                    await ResHandler.Instance.HandlePacketByte(receivedData);
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
