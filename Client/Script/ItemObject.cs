using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.SceneManagement;

/// <summary>
/// �ʵ忡 �����ϴ� �����ۿ� �����Ǵ� ��ũ��Ʈ�̴�.
/// ����, ����, �Ҹ� ó�� �ʿ�
/// </summary>
public class ItemObject : MonoBehaviour
{
    private static GameObject g_itemPrefab;
    private static Dictionary<uint, ItemObject> g_items = new Dictionary<uint, ItemObject>();
    private static Dictionary<int, Sprite> g_sprites = new Dictionary<int, Sprite>();
    private SpriteRenderer m_spriteRenderer;
    private const float fadeDuration = 2.0f;
    private const float moveSpeed = 3.0f;

    private uint m_index;
    private int m_Code;
    private long m_exTime;
    private int m_count;

    private float m_LastTimeTryToGet = 0.0f;

    // ���氡�� ����
    private bool m_active;

    private void Awake()
    {

        m_spriteRenderer = GetComponent<SpriteRenderer>();
    }

    /// <summary>
    /// �ʸ��� ObjectIdx�� ���� ���� ������
    /// ���� �̵��� ������
    /// �浹�� �����ϱ� ���� �ʱ�ȭ�Ѵ�.
    /// </summary>
    public static void InitDictionary()
    {
        g_items = new Dictionary<uint, ItemObject>();

        return;
    }

    public int GetItemCode() { return m_Code; }
    public int GetItemCount() { return m_count; }
    public long GetExTime() {  return m_exTime; }

    public class GetObjectParam
    {
        public int MapCode;
        public uint ItemIdx;

        public GetObjectParam(int mapCode, uint itemIdx)
        {
            MapCode = mapCode;
            ItemIdx = itemIdx;
        }
    }

    public async Task TryToGet()
    {
        // �̹� ����ó�� Ȥ�� �Ҹ��� �������̴�.
        if(m_active == false)
        {
            return;
        }

        // ����õ��� �������� ���������� ����� ��ٸ��� ���� �����ð� �Ŀ� ����õ��ϵ��� �Ѵ�.
        if(Time.time - m_LastTimeTryToGet < 3.0f)
        {
            return;
        }
        m_LastTimeTryToGet = Time.time;
        
        try
        {
            string mapname = SceneManager.GetActiveScene().name;
            int mapcode = int.Parse(mapname);

            GetObjectParam stparam = new GetObjectParam(mapcode, m_index);

            string req = JsonUtility.ToJson(stparam);

            string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.GET_OBJECT, req);
            await NetworkManager.Instance.SendMsg(msg);
        }
        catch(System.Exception e)
        {
            Debug.Log($"ItemObject::TryToGet : {e.Message}");
        }
        
    }

    /// <summary>
    /// �ʿ� �������� �����ϴ� �Լ�
    /// </summary>
    /// <param name="itemIdx_"></param>
    /// <param name="itemCode_"></param>
    /// <param name="expirationTime_"></param>
    /// <param name="count_"></param>
    /// <param name="position_"></param>
    public static void CreateItem(uint itemIdx_, int itemCode_, long expirationTime_, int count_, Vector3 position_)
    {
        if (itemCode_ == 0)
        {
            return;
        }

        if (g_itemPrefab == null)
        {
            g_itemPrefab = Resources.Load<GameObject>("Prefabs/ItemObject");

            if (g_itemPrefab == null)
            {
                Debug.Log($"ItemObject::CreateItem : prefab null ref");
            }
        }

        try
        {
            GameObject obj = Instantiate(g_itemPrefab, position_, Quaternion.identity);

            ItemObject script = obj.GetComponent<ItemObject>();

            script.m_index = itemIdx_;
            script.m_count = count_;
            script.m_Code = itemCode_;
            script.m_exTime = expirationTime_;
            script.m_active = true;

            if (!g_items.TryAdd(itemIdx_, script))
            {
                Debug.Log($"ItemObject::CreateItem : Failed To Add");
            }

            SpriteRenderer sr = obj.GetComponent<SpriteRenderer>();

            Sprite sprite;

            if(!g_sprites.TryGetValue(script.m_Code, out sprite))
            {
                sprite = Resources.Load<Sprite>($"Item/{script.m_Code}");

                if(sprite == null)
                {
                    Debug.Log($"ItemObject::CreateItem : Sprite null ref");
                }

                g_sprites.TryAdd(script.m_Code, sprite);
            }
            sr.sprite = sprite;
        }
        catch (System.Exception e)
        {
            Debug.Log($"ItemObject::CreateItem : {e.Message}");
        }
        
    }

    /// <summary>
    /// ������ ���濡 �����Ͽ� �ش� �������� �÷��̾� �������� �ű�� �����ϰ� ����� �Ҹ�
    /// �κ��丮�� �ݿ�
    /// </summary>
    /// <param name="itemIdx_"></param>
    public static void GetItem(uint itemIdx_)
    {
        ItemObject script;
        if (g_items.TryGetValue(itemIdx_, out script))
        {
            g_items.Remove(itemIdx_);
            script.m_active = false;

            InventoryPanel.Add(script.m_Code, script.m_exTime, script.m_count);

            script.StartCoroutine(script.Move(PlayerCharacter.Instance.gameObject));
            script.StartCoroutine(script.Discard());
        }
    }

    /// <summary>
    /// �ٸ� ������ ������ ��� ������Ʈ�� ��ġ�� �ش� ������������ �ű�� �����ϰ� ����� �Ҹ�
    /// </summary>
    /// <param name="itemIdx_"></param>
    /// <param name="charIdx_"></param>
    public static void SetObtained(uint itemIdx_, int charIdx_)
    {
        ItemObject script;
        if(g_items.TryGetValue(itemIdx_, out script))
        {
            g_items.Remove(itemIdx_);
            script.m_active = false;
            //script.StartCoroutine(script.Move());
            script.StartCoroutine(script.Discard());
        }
    }


    /// <summary>
    /// Ư�� �ε����� �������� �Ҹ��û�ϴ� �Լ�
    /// </summary>
    /// <param name="itemIdx_"></param>
    public static void Discard(uint itemIdx_)
    {
        ItemObject script;
        if(g_items.TryGetValue(itemIdx_, out script))
        {
            g_items.Remove(itemIdx_);
            script.m_active = false;
            script.StartCoroutine(script.Discard());
        }

        return;
    }

    /// <summary>
    /// Ÿ�÷��̾� ��ġ�� �̵��ϸ� �����ϰ� ����� �Ҹ�
    /// </summary>
    /// <param name="charcode_"></param>
    /// <returns></returns>
    IEnumerator Move(GameObject obj_)
    {
        float timeElapsed = 0f;

        Vector3 target = obj_.transform.position;

        while (timeElapsed < fadeDuration)
        {
            // ������ ������Ʈ�� �̹� �Ҹ�� ��ü�� ��� ����.
            if (gameObject == null)
            {
                yield break;
            }

            timeElapsed += Time.deltaTime;

            // �ش� �÷��̾� ������Ʈ�� �Ҹ����� �ʾҴٸ� ��ǥ��ġ�� ����
            if (obj_ != null)
            {
                target = obj_.transform.position;
            }
            transform.position = Vector3.Lerp(transform.position, target, moveSpeed * Time.deltaTime);

            yield return null; // ���������ӱ��� ���
        }
    }

    /// <summary>
    /// ������ ������Ʈ�� ������ �����ϰ� ����� �Ҹ��Ų��.
    /// </summary>
    IEnumerator Discard()
    {
        Color color = m_spriteRenderer.color;
        float startAlpha = color.a;

        float timeElapsed = 0f;
        while(timeElapsed < fadeDuration)
        {
            timeElapsed += Time.deltaTime;
            float alpha = Mathf.Lerp(startAlpha, 0f, timeElapsed / fadeDuration);
            color.a = alpha;
            m_spriteRenderer.color = color;

            yield return null; // ���������ӱ��� ���
        }

        Destroy(gameObject); // �Ҹ�
    }
}
