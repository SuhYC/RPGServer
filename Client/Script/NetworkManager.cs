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
/// ��Ʈ��ũ �ۼ��� ó���� ���� ��ũ��Ʈ.
/// 
/// MTU�� ���� ���� ������ �ذ��ϱ� ����
/// ������ �޽����� ũ�⸦ ����� �����Ͽ� �ۼ����Ѵ�.
/// 
/// '[' + size + ']' + payload
/// 
/// ������ �޽����� ����� �����ϴ°� ResHandler�� �����Ѵ�.
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
    /// ���� ����.
    /// ���� ������������Լ�RecvMsg()�� ȣ���Ѵ�. RecvMsg�� ������ ������ ������ ���ο��� �ݺ�.
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
            Debug.Log($"NetworkManager::ConnectToTcpServer : ���� ����. {e.Message}");
            return;
        }

        Debug.Log($"NetworkManager::ConnectToTcpServer : ���� �Ϸ�");

        await RecvMsg();

        return;
    }

    private void OnApplicationQuit()
    {
        _stream?.Close();
        _tcpClient?.Close();
    }

    /// <summary>
    /// PacketMaker.instance.ToReqMessage�� ���� ���� ��Ŷ�� ������ ��.
    /// </summary>
    /// <param name="msg">jsonstr of ReqMessage</param>
    /// <returns></returns>
    public async Task SendMsg(string msg)
    {
        if(_tcpClient != null && _tcpClient.Connected)
        {
            //byte[] tmp = Encoding.UTF8.GetBytes(msg);
            byte[] tmp = Encoding.GetEncoding("euc-kr").GetBytes(msg);

            // �۽�ũ�� ���
            string[] strings = { "[", tmp.Length.ToString(), "]", msg };
            msg = string.Join("", strings); //msg = "[" + tmp.Length.ToString() + "]" + msg;

            //byte[] data = Encoding.UTF8.GetBytes(msg);
            byte[] data = Encoding.GetEncoding("euc-kr").GetBytes(msg);

            await _stream.WriteAsync(data, 0, data.Length);
            Debug.Log($"NetworkManager::SendMsg : ������ ����: {msg}");
        }
    }

    /// <summary>
    /// ���������� �����κ����� ������ üũ.
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

                    Debug.Log($"NetworkManager::RecvMsg : ����: {receivedData}");
                    await ResHandler.Instance.HandlePacketByte(receivedData);
                }
                // ��������, ���� �߻�
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
