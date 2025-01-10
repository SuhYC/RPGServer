using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerCharacter : MonoBehaviour
{
    private bool _isGrounded;
    private float PlayerSpeed = 140f;
    private float PlayerJump = 100f;
    Vector2 LastPoint;
    Vector2 _distance;
    Vector2 _velocity;
    Vector2 _slowdownVec;
    Vector2 _addForceVec;

    KeyCode JumpKey = KeyCode.Space; // ���߿� Ű���õ� ����� ����
    KeyCode AttackKey = KeyCode.A;

    Rigidbody2D rb;
    SpriteRenderer sr;

    const float MAX_SPEED = 1.0f;
    void Awake()
    {
        _isGrounded = false;
        rb = GetComponent<Rigidbody2D>();
        sr = GetComponent<SpriteRenderer>();
        LastPoint = rb.position;
    }

    // Update is called once per frame
    void Update()
    {
        if(Input.GetKeyDown(JumpKey) && _isGrounded)
        {
            Jump();
        }

        if(Input.GetKeyDown(AttackKey))
        {
            Attack();
        }

        CharacterImageFlip();
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
        _isGrounded = false;
        rb.AddForce(Vector2.up * PlayerJump * 0.1f, ForceMode2D.Impulse);
    }

    private void Attack()
    {
        // ���� �����ϱ�
        // ������ �����ϱ�
        // ������ ������ ������ ��������ŭ�� ���� �ܾ����
        // �ܾ�� ���Ϳ� ������ ������ �־��ٰ� ������ �����ϱ� (���� ��������� ������ �� �� Ŭ���̾�Ʈ�� ����)
    }
}
