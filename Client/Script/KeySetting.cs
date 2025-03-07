using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class KeySetting
{
    public KeyCode JumpKey = KeyCode.Space; 
    public KeyCode AttackKey = KeyCode.A;
    public KeyCode InvenKey = KeyCode.I;
    public KeyCode GetKey = KeyCode.Z;

    public KeySetting()
    {
        JumpKey = KeyCode.Space;
        AttackKey = KeyCode.A;
        InvenKey = KeyCode.I;
        GetKey = KeyCode.Z;
    }
}
