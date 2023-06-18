using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATHS_STRING + "Fixed Point", AEConsts.MENU_OPENMPD_PATHS_OFFSET + (int)PathTypes.FixedPosition)]
// This node describes a fixed point
public class FixedPositionPathNode : PathNode {

	[Input] public Vector3 fixedPosition = Vector3.zero;

    public override Vector3 getPositionAtPercent(float p)
    {
        return fixedPosition;
    }

    protected override void OnDirtyUpdate(){
        fixedPosition = GetInputValue<Vector3>("fixedPosition", this.fixedPosition);
    }

    // Use this for initialization
    protected override void Init() {
		base.Init();
	}
}