using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// �÷��̾�� ĳ������ �������θ� �����ϱ� ���� ��ũ��Ʈ.
/// </summary>
public class PlayerFoot : MonoBehaviour
{
    PlayerCharacter player;

    void Awake()
    {
        player = transform.parent.GetComponent<PlayerCharacter>();
    }

    void OnTriggerStay2D(Collider2D other)
    {
        player.SetGrounded();
    }
}
