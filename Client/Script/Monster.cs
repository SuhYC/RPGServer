using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Monster : MonoBehaviour
{
    GameObject TopHealthBar;

    void Awake()
    {
        // SearchTopHealthBar
    }

    public enum MonsterType
    {
        Normal,
        Boss
    }

    private int _monsterCode; // 구체적인 몬스터를 구분.
    private int _monsterLv; // 몬스터의 레벨
    private long _healthPoint; // 현재체력
    private long _maxHealth; // 최대체력
    private MonsterType _monsterType; // 몬스터의 타입을 구분. 일반몬스터, 보스몬스터 ...
    private float _defencePoint; // 데미지의 일부를 무시합니다. 방어무시간 곱적용. def * (1 - (100-a)/100 * (100-b)/100)
    private int _resistance; // 데미지의 일부를 무시합니다. 저항무시간 합적용. 100 - res + a + b, 저항은 0이하로 떨어질 수 없음.

    public void Init(int monsterCode_, long healthPoint_)
    {
        _monsterCode = monsterCode_;
        _healthPoint = healthPoint_;

        LoadMonsterData();
    }

    private void LoadMonsterData()
    {
        // 몬스터코드를 기반으로 몬스터 정보표에서부터 고정데이터 불러오기
        // 최대체력
        // 몬스터레벨
        // 몬스터타입
        // 방어율
        // 저항
    }

    // ignoreDefence는 1이하의 양수여야한다. 0 -> 100%무시 1 -> 0%무시
    public void GetAttacked(long damage, float ignoreDefence, int ignoreResist, int characterLv)
    {
        long finalDamage = damage;
        int finalResist = _resistance - ignoreResist;

        if (finalResist > 0)
        {
            finalDamage = (long)(finalDamage * ((100f - finalResist) / 100));
        }

        float finalDefence = _defencePoint * ignoreDefence;

        finalDamage = (long)(finalDamage * (1f - finalDefence));

        // SendAttackDataToServer

        _healthPoint -= finalDamage;

        RenewHealthBar();
    }

    public void RenewHealth(long health_)
    {
        _healthPoint = health_;

        RenewHealthBar();
    }

    protected void RenewHealthBar()
    {
        if(_monsterType == MonsterType.Boss)
        {
            // RenewTopHealthBar
        }
        else
        {
            // RenewHealthBar
        }
    }
}
