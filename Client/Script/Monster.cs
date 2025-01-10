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

    private int _monsterCode; // ��ü���� ���͸� ����.
    private int _monsterLv; // ������ ����
    private long _healthPoint; // ����ü��
    private long _maxHealth; // �ִ�ü��
    private MonsterType _monsterType; // ������ Ÿ���� ����. �Ϲݸ���, �������� ...
    private float _defencePoint; // �������� �Ϻθ� �����մϴ�. ���ð� ������. def * (1 - (100-a)/100 * (100-b)/100)
    private int _resistance; // �������� �Ϻθ� �����մϴ�. ���׹��ð� ������. 100 - res + a + b, ������ 0���Ϸ� ������ �� ����.

    public void Init(int monsterCode_, long healthPoint_)
    {
        _monsterCode = monsterCode_;
        _healthPoint = healthPoint_;

        LoadMonsterData();
    }

    private void LoadMonsterData()
    {
        // �����ڵ带 ������� ���� ����ǥ�������� ���������� �ҷ�����
        // �ִ�ü��
        // ���ͷ���
        // ����Ÿ��
        // �����
        // ����
    }

    // ignoreDefence�� 1������ ��������Ѵ�. 0 -> 100%���� 1 -> 0%����
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
