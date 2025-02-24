using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using System.Threading.Tasks;
using UnityEngine.UI;

/// <summary>
/// ������ ��ȭ�� ���� ���ϴ� ä���г��� ��ũ��Ʈ.
/// 
/// �����κ��� ������ ä�������� ���๮�ڷ� �����Ͽ� �����ϸ�
/// ä�������� MAX_TEXT_SIZE�� ������ ������ �޽������� �����Ѵ�.
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
        // UI ������Ʈ�� DontDestroyOnLoad�� �����ϱ� �����ϰ� ������ ���� �� �ִ�.
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
    /// ���� ���̾��Ű�� ��� ��ȸ�Ͽ� str�� �̸��� ��ġ�� Ʈ�������� �����Ѵ�.
    /// </summary>
    /// <param name="now">�ڽĵ��� ��ȸ�� ���� Ʈ������</param>
    /// <param name="str">ã�� ���� Ʈ�������� �̸�</param>
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
    /// �����κ��� �� �޽����� ä�ø޽����� �����Ѵ�.
    /// MAX_TEXT_SIZE�� �ʰ��ߴٸ� ������ �޽����� �����ϴ� RemoveString�� ȣ���Ѵ�.
    /// </summary>
    /// <param name="str"></param>
    public void AddString(string str)
    {
        // ���� �׷����� ���ٸ�
        // �ʿ� ���Ե� ���ߴµ� ��û�� �� ��츦 ����.
        if(text == null)
        {
            return;
        }
        string[] words = { text.text, str };
        string totalText = string.Join("\n", words); // "\n"���� �����Ͽ� words�迭�� ���ڿ��� ��ģ��.

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
    /// ä�ø޽����� MAX_TEXT_SIZE�� �ʰ��� ���
    /// "\n"���� ���е� �޽��� �� ���� ������ �޽������� �����Ͽ� MAX_TEXT_SIZE �̳��� �޽����� �����.
    /// </summary>
    /// <param name="str">���̸� �����ϰ� ���� �Ѹ޽���</param>
    /// <returns>������ ���� �޽���</returns>
    private string RemoveString(string str)
    {
        int size = str.Length;

        // �߶��� �� �ִ� ���� ���ϰ� �� �� �ִ� ���� ������ '\n'�� ã�´�.
        int idx = str.IndexOf('\n', size - MAX_TEXT_SIZE); // [0]�� [1]�ε��� ���ĺ��� ã�´�.

        // �ϳ��� ä���� ���ζ� �߰ߵ��� �ʾҴ�???
        // �� ä���� �ִ���̸� �����صξ��ٰ� ����.
        // �ϳ��� ä������ ���̸� �Ѵ� ���� ���� �ؾ��Ѵ�.
        if(idx < 0)
        {
            if(size > MAX_TEXT_SIZE)
            {
                // ���� �׷����� �߻��ߴٸ� �׳� �� ���������.
                return string.Empty;
            }

            return str;
        }

        return str.Substring(idx + 1);
    }

    /// <summary>
    /// ���Ͱ� �ԷµǸ� �Է��� ���ڿ��� ������ ������.
    /// OnEndEdit�� ��Ŀ���� ���ܵ� ��츦 ��� �����ϹǷ�
    /// ����Ű�� �Է��Ѱ��� �´��� üũ�Ѵ�.
    /// </summary>
    /// <param name="inputText"></param>
    public async void OnEndEdit(string inputText)
    {
        if(!Input.GetKeyDown(KeyCode.Return) && !Input.GetKeyDown(KeyCode.KeypadEnter))
        {
            // ���ͷ� ��Ŀ���� ���ܵ� ���̽��� �ƴ�
            return;
        }

        string req = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.CHAT_EVERYONE, inputText);
        await NetworkManager.Instance.SendMsg(req);

        inputField.text = string.Empty;
    }
}
