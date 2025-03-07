using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using System.Threading.Tasks;

public class DropItemPanel : MonoBehaviour
{
    private static GameObject m_prefab;
    private static Canvas canvas;
    private int m_index;
    private TMPro.TMP_InputField inputField;

    public static void CreateDropItemPanel(int idx_)
    {
        if (m_prefab == null)
        {
            m_prefab = Resources.Load<GameObject>("Prefabs/UI/DropItemPanel");

            if (m_prefab == null)
            {
                Debug.Log($"DropItemPanel::CreateDropItemPanel : prefab null ref");
            }
        }

        if(canvas == null)
        {
            canvas = FindAnyObjectByType<Canvas>();

            if(canvas == null)
            {
                Debug.Log($"DropItemPanel::CreateDropItemPanel : canvas null ref");
                return;
            }
        }

        GameObject obj = Instantiate(m_prefab, canvas.transform);

        DropItemPanel script = obj.GetComponent<DropItemPanel>();

        script.Init(idx_);
    }

    public async void ClickedButton()
    {
        string text = inputField.text;

        if(text == string.Empty)
        {
            TextMessage.CreateTextPanel("���ڸ� �Է����ּ���.");
            return;
        }

        try
        {
            int count = int.Parse(text);

            await Drop(count);
        }
        catch (System.FormatException)
        {
            TextMessage.CreateTextPanel("���ڸ� �Է����ּ���.");
        }
        catch (System.Exception e)
        {
            Debug.Log($"DropItemPanel::ClickedButton : {e.Message}");
        }
    }

    public async void OnEndEdit(string text)
    {
        if(text == string.Empty)
        {
            return;
        }

        if(!Input.GetKeyDown(KeyCode.Return) && !Input.GetKeyDown(KeyCode.KeypadEnter))
        {
            // �̰� ��Ŀ���� ����Ŵ�.
            return;
        }

        try
        {
            int count = int.Parse(text);

            await Drop(count);
        }
        catch (System.FormatException)
        {
            TextMessage.CreateTextPanel("���ڸ� �Է����ּ���.");
        }
        catch (System.Exception e)
        {
            Debug.Log($"DropItemPanel::OnEndEdit : {e.Message}");
        }
    }

    public void Init(int index_)
    {
        m_index = index_;

        int itemcode = InventoryPanel.Instance.GetItemCode(index_);

        Sprite sprite = Resources.Load<Sprite>($"Item/{itemcode}");

        if(sprite == null)
        {
            Debug.Log($"DropItemPanel::Init : sprite null ref.");
            return;
        }

        Image image = transform.GetChild(1)?.GetComponent<Image>();

        if (image == null)
        {
            Debug.Log($"DropItemPanel::Init : image null ref.");
            return;
        }

        image.sprite = sprite;

        inputField = transform.GetChild(3)?.GetComponent<TMPro.TMP_InputField>();

        if(inputField == null)
        {
            Debug.Log($"DropItemPanel::Init : inputField null ref.");
            return;
        }
        inputField.onEndEdit.AddListener(OnEndEdit);
    }

    public void Exit()
    {
        // �ٽ� �κ��丮�� ���۰����ϰ� �Ѵ�.
        InventoryPanel.SetInteractable(true);
        Destroy(gameObject);
    }



    public class DropItemParam
    {
        public int MapCode;
        public int SlotIdx;
        public int Count;
        public float PosX;
        public float PosY;

        public DropItemParam(int mapcode, int slotIdx, int count, float posX, float posY)
        {
            MapCode = mapcode;
            SlotIdx = slotIdx;
            Count = count;
            PosX = posX;
            PosY = posY;
        }
    }

    public async Task Drop(int count_)
    {
        int MaxCount = InventoryPanel.Instance.GetCount(m_index);

        if (MaxCount < count_)
        {
            TextMessage.CreateTextPanel($"{MaxCount}�� ���ϸ� ���� �� �ֽ��ϴ�.");
            return;
        }

        Vector3 pos = PlayerCharacter.Instance.GetPosition();
        int mapcode = UserData.instance.GetMapCode();

        // y�� 0.6 ��°� ĳ���� ũ�� ����. ���� �Ʒ��� ������� �ؼ�.
        // ĳ���� ũ�⸦ ���� �޾Ƽ� �ص� �Ǵµ� �ϴ� �̷��� ������.
        DropItemParam param = new DropItemParam(mapcode, m_index, count_, pos.x, pos.y - 0.6f);

        string req = JsonUtility.ToJson(param);

        string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.DROP_ITEM, req);

        await NetworkManager.Instance.SendMsg(msg);

        Destroy(gameObject);
    }
}
