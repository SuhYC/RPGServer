using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// <summary>
/// UI가 아닌 오브젝트를 클릭할 수 있도록 하는 스크립트.
/// 해당 스크립트를 부착한 오브젝트가 씬에 존재해야 클릭을 인식할 수 있다.
/// 
/// Clickable Layer에 존재하는 오브젝트만 검사하도록 했다. Layer를 반드시 확인할 것.
/// 이후에 만약 다른 레이어도 포함하여 검사하게 만들기 위해서는 bitwise OR 연산자를 사용하여 처리.
/// (아마도 캐릭터 정보를 확인하는 경우에 쓰일것같음)
/// </summary>
public class ClickManager : MonoBehaviour
{
    private LayerMask layer;

    private void Awake()
    {
        // 추후 다른 레이어도 필요하면 | 를 통해 bitwise or로 입력해주자.
        layer = 1 << LayerMask.NameToLayer("Clickable");
    }

    void Update()
    {
        if (Input.GetMouseButtonDown(0)) // 마우스 좌클릭
        {
            //Debug.Log($"ClickManager::Update : Click!");

            // 화면 상의 마우스 위치에서 Ray를 쏩니다.
            Ray ray = Camera.main.ScreenPointToRay(Input.mousePosition);
            RaycastHit hit;

            // Ray가 Collider와 충돌한 경우
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

                //... 다른 클릭 이벤트가 필요한 스크립트가 있다면 입력할 것
            }
            else
            {
                //Debug.Log($"ClickManager::Update : Catched None");
            }
        }

        
        if (Input.GetMouseButtonDown(1)) // 마우스 우클릭
        {

        }
    }
}
