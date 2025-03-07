using System.Collections;
using System.Collections.Generic;
using UnityEditor.UIElements;
using UnityEngine;

/// <summary>
/// 플레이어블 캐릭터의 접지여부를 설정하기 위한 스크립트.
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
        if (other.gameObject.layer == LayerMask.NameToLayer("Platform"))
        {
            player.SetGrounded();
        }        
    }
}
