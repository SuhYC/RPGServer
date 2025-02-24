using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// UI�� �ƴ� ������Ʈ�� Ŭ���� �� �ֵ��� �ϴ� ��ũ��Ʈ.
/// �ش� ��ũ��Ʈ�� ������ ������Ʈ�� ���� �����ؾ� Ŭ���� �ν��� �� �ִ�.
/// 
/// Clickable Layer�� �����ϴ� ������Ʈ�� �˻��ϵ��� �ߴ�. Layer�� �ݵ�� Ȯ���� ��.
/// ���Ŀ� ���� �ٸ� ���̾ �����Ͽ� �˻��ϰ� ����� ���ؼ��� bitwise OR �����ڸ� ����Ͽ� ó��.
/// (�Ƹ��� ĳ���� ������ Ȯ���ϴ� ��쿡 ���ϰͰ���)
/// </summary>
public class ClickManager : MonoBehaviour
{
    private LayerMask layer;

    private void Awake()
    {
        // ���� �ٸ� ���̾ �ʿ��ϸ� | �� ���� bitwise or�� �Է�������.
        layer = 1 << LayerMask.NameToLayer("Clickable");
    }

    void Update()
    {
        if (Input.GetMouseButtonDown(0)) // ���콺 ��Ŭ��
        {
            //Debug.Log($"ClickManager::Update : Click!");

            // ȭ�� ���� ���콺 ��ġ���� Ray�� ���ϴ�.
            Ray ray = Camera.main.ScreenPointToRay(Input.mousePosition);
            RaycastHit hit;

            // Ray�� Collider�� �浹�� ���
            if (Physics.Raycast(ray, out hit, Mathf.Infinity, layer))
            {
                GameObject obj = hit.collider.gameObject;

                if (obj == null)
                {
                    Debug.Log($"ClickManager::Update : hit obj null ref");
                    return;
                }

                NonePlayerCharactor npc = obj.GetComponent<NonePlayerCharactor>();

                if (npc != null)
                {
                    npc.OnClick();
                }

                //... �ٸ� Ŭ�� �̺�Ʈ�� �ʿ��� ��ũ��Ʈ�� �ִٸ� �Է��� ��
            }
            else
            {
                //Debug.Log($"ClickManager::Update : Catched None");
            }
        }

        
        if (Input.GetMouseButtonDown(1)) // ���콺 ��Ŭ��
        {

        }
    }
}
