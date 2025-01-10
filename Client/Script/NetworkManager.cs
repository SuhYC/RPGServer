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

    public async Task SendMsg(string msg)
    {
        if(_tcpClient != null && _tcpClient.Connected)
        {
            // 송신크기 헤더
            msg = "[" + msg.Length.ToString() + "]" + msg;

            byte[] data = Encoding.UTF8.GetBytes(msg);
            await _stream.WriteAsync(data, 0, data.Length);
            Debug.Log($"NetworkManager::SendMsg : 데이터 전송: {msg}");
        }
    }
    private async Task RecvMsg()
    {
        try
        {
            while (true)
            {
                int bytesRead = await _stream.ReadAsync(_buffer);
                if (bytesRead > 0)
                {
                    string receivedData = Encoding.UTF8.GetString(_buffer, 0, bytesRead);
                    Debug.Log($"NetworkManager::RecvMsg : 수신: {receivedData}");
                    ResHandler.Instance.HandlePacket(receivedData);
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
            Debug.LogError($"NetworkManager::RecvMsg : {e.Message}");
        }
    }

}
