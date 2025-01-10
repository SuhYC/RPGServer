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
            // 몬스터가 진행할 방향 정해주기. (flying을 고려하지 않으면 그냥 좌우만 하면 될듯)
        }

        
    }
    public void RemoveMonster(int idx)
    {
        // 해당 몬스터 사망처리

        monsters[idx] = null;
    }
}
