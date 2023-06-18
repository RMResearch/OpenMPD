using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATHS_STRING + "Line Path", AEConsts.MENU_OPENMPD_PATHS_OFFSET + (int)PathTypes.LinePath)]
// This node describes the a line between two coordinates
public class LinePathNode : PathNode {

	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 startPosition = Vector3.zero;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 endPosition = Vector3.zero;

    // Returns points on a line between startPosition and endPosition based on parameter p.
    // Points are calculated by linear interpolation ((1-p) * startPos) + (p * endPos)
    // Note: p + (1-p) == 1
    public override Vector3 getPositionAtPercent(float p)
    {
        startPosition = GetInputValue<Vector3>("startPosition", this.startPosition);
        endPosition = GetInputValue<Vector3>("endPosition", this.endPosition);
        return Vector3.Lerp(startPosition, endPosition, p);
    }

    protected override void OnDirtyUpdate(){
        startPosition = GetInputValue<Vector3>("startPosition", this.startPosition);
        endPosition = GetInputValue<Vector3>("endPosition", this.endPosition);
    }
}