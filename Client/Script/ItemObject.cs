using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.SceneManagement;

/// <summary>
/// 필드에 존재하는 아이템에 부착되는 스크립트이다.
/// 생성, 습득, 소멸 처리 필요
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

    // 습득가능 여부
    private bool m_active;

    private void Awake()
    {

        m_spriteRenderer = GetComponent<SpriteRenderer>();
    }

    /// <summary>
    /// 맵마다 ObjectIdx를 따로 세기 때문에
    /// 맵을 이동할 때마다
    /// 충돌을 방지하기 위해 초기화한다.
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
        // 이미 습득처리 혹은 소멸한 아이템이다.
        if(m_active == false)
        {
            return;
        }

        // 습득시도한 아이템은 서버응답을 충분히 기다리기 위해 일정시간 후에 습득시도하도록 한다.
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
    /// 맵에 아이템을 생성하는 함수
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
    /// 아이템 습득에 성공하여 해당 아이템을 플레이어 방향으로 옮기며 투명하게 만들어 소멸
    /// 인벤토리에 반영
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
    /// 다른 유저가 습득한 경우 오브젝트의 위치를 해당 유저방향으로 옮기며 투명하게 만들어 소멸
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
    /// 특정 인덱스의 아이템을 소멸요청하는 함수
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
    /// 타플레이어 위치로 이동하며 투명하게 만들다 소멸
    /// </summary>
    /// <param name="charcode_"></param>
    /// <returns></returns>
    IEnumerator Move(GameObject obj_)
    {
        float timeElapsed = 0f;

        Vector3 target = obj_.transform.position;

        while (timeElapsed < fadeDuration)
        {
            // 아이템 오브젝트가 이미 소멸된 객체인 경우 종료.
            if (gameObject == null)
            {
                yield break;
            }

            timeElapsed += Time.deltaTime;

            // 해당 플레이어 오브젝트가 소멸하지 않았다면 목표위치를 갱신
            if (obj_ != null)
            {
                target = obj_.transform.position;
            }
            transform.position = Vector3.Lerp(transform.position, target, moveSpeed * Time.deltaTime);

            yield return null; // 다음프레임까지 대기
        }
    }

    /// <summary>
    /// 아이템 오브젝트를 서서히 투명하게 만들며 소멸시킨다.
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

            yield return null; // 다음프레임까지 대기
        }

        Destroy(gameObject); // 소멸
    }
}
