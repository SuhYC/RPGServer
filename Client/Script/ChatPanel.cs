using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using System.Threading.Tasks;
using UnityEngine.UI;

/// <summary>
/// 유저간 대화를 위한 좌하단 채팅패널의 스크립트.
/// 
/// 서버로부터 도착한 채팅정보를 개행문자로 구분하여 저장하며
/// 채팅정보가 MAX_TEXT_SIZE를 넘으면 오래된 메시지부터 삭제한다.
/// </summary>
public class ChatPanel : MonoBehaviour
{
    private static ChatPanel chatPanel;
    private static string g_chatStr = string.Empty;

    private TMP_Text text;
    private TMP_InputField inputField;
    private Scrollbar scrollbar;

    const int MAX_TEXT_SIZE = 1024;

    public static ChatPanel Instance
    {
        get
        {
            if(chatPanel == null)
            {
                return null;
            }

            return chatPanel;
        }
    }

    void Awake()
    {
        // UI 오브젝트는 DontDestroyOnLoad로 설정하기 복잡하고 문제가 생길 수 있다.
        chatPanel = this;

        Transform textChild = FindDeepChild(transform, "ChatText");
        if (textChild == null)
        {
            Debug.Log($"ChatPanel::Awake : textchild null ref.");
        }
        else
        {
            text = textChild.GetComponent<TMP_Text>();
            if (text == null)
            {
                Debug.Log($"ChatPanel::Awake : text null ref.");
            }
            else
            {
                text.text = g_chatStr;
            }
        }

        Transform inputChild = FindDeepChild(transform, "ChatInput");

        if(inputChild == null)
        {
            Debug.Log($"ChatPanel::Awake : inputchild null ref.");
        }
        else
        {
            inputField = inputChild.GetComponent<TMP_InputField>();

            if(inputField == null)
            {
                Debug.Log($"ChatPanel::Awake : input null ref.");
            }
            else
            {
                inputField.onEndEdit.AddListener(OnEndEdit);
            }
        }

        Transform scrollChild = FindDeepChild(transform, "Scrollbar Vertical");

        if(scrollChild == null)
        {
            Debug.Log($"ChatPanel::Awake : scrollchild null ref.");
        }
        else
        {
            scrollbar = scrollChild.GetComponent<Scrollbar>();

            if(scrollbar == null)
            {
                Debug.Log($"ChatPanel::Awake : scroll null ref.");
            }
            else
            {
                scrollbar.value = 0;
            }
        }
    }

    /// <summary>
    /// 하위 하이어라키를 모두 순회하여 str과 이름이 일치한 트랜스폼을 리턴한다.
    /// </summary>
    /// <param name="now">자식들을 조회할 현재 트랜스폼</param>
    /// <param name="str">찾고 싶은 트랜스폼의 이름</param>
    /// <returns></returns>
    private Transform FindDeepChild(Transform now, string str)
    {
        Transform found = now.Find(str);
        if(found != null)
        {
            return found;
        }

        foreach(Transform child in now)
        {
            found = FindDeepChild(child, str);
            if(found != null)
            {
                return found;
            }
        }

        return null;
    }

    /// <summary>
    /// 서버로부터 온 메시지를 채팅메시지에 포함한다.
    /// MAX_TEXT_SIZE를 초과했다면 오래된 메시지를 제거하는 RemoveString을 호출한다.
    /// </summary>
    /// <param name="str"></param>
    public void AddString(string str)
    {
        // 설마 그럴일은 없다만
        // 맵에 진입도 못했는데 요청이 온 경우를 배제.
        if(text == null)
        {
            return;
        }
        string[] words = { text.text, str };
        string totalText = string.Join("\n", words); // "\n"으로 구분하여 words배열의 문자열을 합친다.

        if(totalText.Length > MAX_TEXT_SIZE)
        {
            totalText = RemoveString(totalText);
        }
        
        text.text = totalText;
        g_chatStr = totalText;

        if(scrollbar != null)
        {
            scrollbar.value = 0;
        }

        return;
    }

    /// <summary>
    /// 채팅메시지가 MAX_TEXT_SIZE를 초과한 경우
    /// "\n"으로 구분된 메시지 중 가장 오래된 메시지부터 제거하여 MAX_TEXT_SIZE 이내의 메시지를 만든다.
    /// </summary>
    /// <param name="str">길이를 조정하고 싶은 총메시지</param>
    /// <returns>조정이 끝난 메시지</returns>
    private string RemoveString(string str)
    {
        int size = str.Length;

        // 잘랐을 때 최대 길이 이하가 될 수 있는 가장 앞쪽의 '\n'을 찾는다.
        int idx = str.IndexOf('\n', size - MAX_TEXT_SIZE); // [0]를 [1]인덱스 이후부터 찾는다.

        // 하나의 채팅이 전부라 발견되지 않았다???
        // 각 채팅의 최대길이를 제한해두었다고 가정.
        // 하나의 채팅으로 길이를 넘는 일은 없게 해야한다.
        if(idx < 0)
        {
            if(size > MAX_TEXT_SIZE)
            {
                // 정말 그런일이 발생했다면 그냥 다 비워버리자.
                return string.Empty;
            }

            return str;
        }

        return str.Substring(idx + 1);
    }

    /// <summary>
    /// 엔터가 입력되면 입력한 문자열을 서버로 보낸다.
    /// OnEndEdit은 포커스가 제외된 경우를 모두 포함하므로
    /// 엔터키를 입력한것이 맞는지 체크한다.
    /// </summary>
    /// <param name="inputText"></param>
    public async void OnEndEdit(string inputText)
    {
        if(!Input.GetKeyDown(KeyCode.Return) && !Input.GetKeyDown(KeyCode.KeypadEnter))
        {
            // 엔터로 포커스가 제외된 케이스가 아님
            return;
        }

        string req = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.CHAT_EVERYONE, inputText);
        await NetworkManager.Instance.SendMsg(req);

        inputField.text = string.Empty;
    }
}
