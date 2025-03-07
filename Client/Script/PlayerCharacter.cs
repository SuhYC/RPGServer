using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.Threading.Tasks;

/// <summary>
/// �÷��̾�� ĳ������ ������ ó���ϴ� ��ũ��Ʈ. <br/>
/// ������ٵ� ó��
/// Ű�����Է� ó��
/// ���� Ű���� �ý����� ���� ���ɼ��� ���� KeyCode�� ���� �����صΰ� �������.
/// </summary>
public class PlayerCharacter : MonoBehaviour
{
    private static PlayerCharacter m_instance;
    public static PlayerCharacter Instance
    {
        get { return m_instance; }
    }

    private bool _isGrounded;
    private float PlayerSpeed = 140f;
    private float PlayerJump = 100f;
    Vector2 LastPoint;
    Vector2 _distance;
    Vector2 _velocity;
    Vector2 _slowdownVec;
    Vector2 _addForceVec;

    private KeySetting _keySetting;
    private Dictionary<KeyCode, System.Action> keyBinding;

    Rigidbody2D rb;
    SpriteRenderer sr;

    Vector2 CharSize;

    const float MAX_SPEED = 1.0f;
    void Awake()
    {
        m_instance = this;

        _isGrounded = false;
        rb = GetComponent<Rigidbody2D>();
        sr = GetComponent<SpriteRenderer>();
        LastPoint = rb.position;
        
        BoxCollider2D boxCollider2D = GetComponent<BoxCollider2D>();
        CharSize = boxCollider2D.size;

        RenewKeySetting(new KeySetting());
    }

    void Update()
    {
        try
        {
            foreach (var KeyPair in keyBinding)
            {
                if (Input.GetKeyDown(KeyPair.Key))
                {
                    KeyPair.Value.Invoke();
                }
            }
        }
        catch (Exception e)
        {
            Debug.Log($"PlayerCharacter::Update : {e.Message}");
        }

        CharacterImageFlip();
    }

    private void RenewKeySetting(KeySetting set)
    {
        keyBinding = new Dictionary<KeyCode, System.Action>
        {
            {set.JumpKey, Jump },
            {set.AttackKey, Attack},
            {KeyCode.UpArrow, UpArrowKey},
            {set.InvenKey, ToggleInventoryPanel },
            {set.GetKey, GetObject }

        };
    }

    void FixedUpdate()
    {
        Move();
    }

    public void SetGrounded()
    {
        _isGrounded = true;
    }

    private void CharacterImageFlip()
    {
        if(Input.GetButton("Horizontal"))
        {
            float fret = Input.GetAxisRaw("Horizontal");

            if(fret != 0f) // �Ѵ� �Է��Ѱ� ���� (�� ������ �Ȱɸ� �Ѵ� �Է½� ������ �������� ���Ե�.)
            {
                sr.flipX = Input.GetAxisRaw("Horizontal") == -1;
            }
        }
    }

    private void SlowDownX(Vector2 vel_)
    {
        _slowdownVec.x = -vel_.normalized.x * PlayerSpeed * 0.02f;
        _slowdownVec.y = 0f;
        rb.AddForce(_slowdownVec, ForceMode2D.Force);
    }

    private void AddForceByInput()
    {
        if(!_isGrounded)
        {
            return;
        }

        if(rb == null)
        {
            Debug.Log("PlayerCharacter : no rigidbody.");
            return;
        }

        if (_velocity == null)
        {
            _velocity = rb.velocity;
        }

        float fret = Input.GetAxisRaw("Horizontal");

        if (fret == 0f)
        {
            if (_velocity.sqrMagnitude > 0.01)
            {
                SlowDownX(_velocity);
            }
            else
            {
                rb.velocity = Vector2.zero;
            }
        }
        else if(fret < 0f)
        {
            if (_velocity.x > -MAX_SPEED)
            {
                _addForceVec = Vector2.left * PlayerSpeed * 0.1f;
                rb.AddForce(_addForceVec, ForceMode2D.Force);
            }
            else if(_velocity.x < -MAX_SPEED)
            {
                SlowDownX(_velocity);
            }
        }
        else if (fret > 0f)
        {
            if (_velocity.x < MAX_SPEED)
            {
                _addForceVec = Vector2.right * PlayerSpeed * 0.1f;
                rb.AddForce(_addForceVec, ForceMode2D.Force);
            }
            else if (_velocity.x > MAX_SPEED)
            {
                SlowDownX(_velocity);
            }
        }
    }

    private void Move()
    {
        AddForceByInput();

        _distance.x = LastPoint.x - rb.position.x;
        _distance.y = LastPoint.y - rb.position.y;
    }
    private void Jump()
    {
        if(_isGrounded == false)
        {
            return;
        }

        _isGrounded = false;
        rb.AddForce(Vector2.up * PlayerJump * 0.1f, ForceMode2D.Impulse);
    }

    private void GetObject()
    {
        LayerMask layerMask = LayerMask.GetMask("ItemObject");

        Collider2D[] colliders = Physics2D.OverlapBoxAll(transform.position, CharSize, 0f, layerMask);

        foreach (Collider2D collider in colliders)
        {
            if (!collider.isTrigger)
            {
                continue;
            }

            ItemObject script = collider.GetComponent<ItemObject>();

            if(script == null)
            {
                continue; 
            }

            try
            {
                Task task = script.TryToGet();
            }
            catch (Exception e)
            {
                Debug.Log($"PlayerCharacter::GetObject : {e.Message}");
            }

            return;
        }
    }

    private void Attack()
    {
        // ���� �����ϱ�
        // ������ �����ϱ�
        // ������ ������ ������ ��������ŭ�� ���� �ܾ����
        // �ܾ�� ���Ϳ� ������ ������ �־��ٰ� ������ �����ϱ� (���� ��������� ������ �� �� Ŭ���̾�Ʈ�� ����)
    }

    public class MoveMapParameter
    {
        public int MapCode;

        public MoveMapParameter(int mapCode)
        {
            MapCode = mapCode;
        }
    }

    private void UpArrowKey()
    {
        LayerMask layerMask = LayerMask.GetMask("Portal");

        Collider2D[] colliders = Physics2D.OverlapBoxAll(transform.position, CharSize, 0f, layerMask);

        foreach (Collider2D collider in colliders)
        {
            if (!collider.isTrigger)
            {
                continue;
            }

            if (collider.transform.name == "Portal")
            {
                Portal portal = collider.transform.GetComponent<Portal>();
                if (portal != null)
                {
                    MoveMapParameter req = new MoveMapParameter(portal._toMapCode);
                    string str = JsonUtility.ToJson(req);
                    string msg = PacketMaker.instance.ToReqMessage(PacketMaker.ReqType.MOVE_MAP, str);

                    try
                    {
                        Task task = NetworkManager.Instance.SendMsg(msg);
                    }
                    catch (Exception e)
                    {
                        Debug.Log($"PlayerCharacter::UpArrowKey : {e.Message}");
                        return;
                    }
                }
                break;
            }
        }
    }


    private void ToggleInventoryPanel()
    {
        if(InventoryPanel.Instance == null)
        {
            InventoryPanel.CreateInstance();
        }

        bool b = InventoryPanel.Instance.gameObject.activeSelf;

        InventoryPanel.Instance.gameObject.SetActive(!b);
    }

    public Vector3 GetPosition()
    {
        return transform.position;
    }
}
