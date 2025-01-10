using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using static Monster;

public class MonsterManager : MonoBehaviour
{
    Monster[] monsters;
    const int MAX_MONSTER_COUNT = 10;
    GameObject prefab;

    void Awake()
    {
        monsters = new Monster[MAX_MONSTER_COUNT];
        prefab = Resources.Load<GameObject>("Prefabs/Monster");

        if(prefab == null )
        {
            Debug.Log("a");
        }
    }

    public void SpawnMonster(int idx, int monsterCode_, long healthPoint_, Vector2 position_, int target = 0)
    {
        GameObject instance = Instantiate(prefab);

        instance.transform.SetParent(this.transform);
        instance.transform.position = position_;
        instance.AddComponent<Monster>();

        Monster monster = instance.GetComponent<Monster>();

        monster.Init(monsterCode_, healthPoint_);

        monsters[idx] = monster;
    }

    public void RenewMonster(int idx, long healthPoint_, Vector2 position_, int target = 0)
    {
        GameObject instance = monsters[idx].gameObject;
        monsters[idx].RenewHealth(healthPoint_);

        Vector2 distance = new Vector2(instance.transform.position.x - position_.x, instance.transform.position.y - position_.y);

        if(distance.sqrMagnitude > 1)
        {
            instance.transform.position = position_;
        }
        else
        {
            // ���Ͱ� ������ ���� �����ֱ�. (flying�� ������� ������ �׳� �¿츸 �ϸ� �ɵ�)
        }

        
    }
    public void RemoveMonster(int idx)
    {
        // �ش� ���� ���ó��

        monsters[idx] = null;
    }
}
