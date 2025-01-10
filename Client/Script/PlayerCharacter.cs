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

    KeyCode JumpKey = KeyCode.Space; // 나중에 키세팅도 만들기 위함
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

            if(fret != 0f) // 둘다 입력한건 제외 (이 조건을 안걸면 둘다 입력시 무조건 오른쪽을 보게됨.)
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
        // 범위 산정하기
        // 마릿수 산정하기
        // 정해진 범위의 정해진 마릿수만큼의 몬스터 긁어오기
        // 긁어온 몬스터에 정해진 데미지 주었다고 서버에 전달하기 (몬스터 사망판정은 서버가 한 후 클라이언트에 전달)
    }
}
