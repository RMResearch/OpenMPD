using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATH_UTILITIES_STRING + "Path Merger", AEConsts.MENU_OPENMPD_PATH_UTILITIES_OFFSET + (int)PathUtilities.PathRotatorNode)]
// This node takes a path and 'rotates' the list of sample positions, allowing the primitive
// to start part-way into a cyclical path and return to the part-way start position.
// Note, if the path is not cyclical then there will be a sudden transition in position
// which may drop a levitated particle
public class PathRotatorNode : PathNode {

    [Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public PathNode path = null;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float percentage = 1.0f;


    public override Vector3 getPositionAtPercent(float p)
    {
        // Start at percentage when p == 0
        // (0 + percentage) % 1.0f
        // End at percentage when p == 1
        // (1 + percentage) % 1.0f
        float adjustedP = (p + percentage) % 1.0f;
        return path.getPositionAtPercent(adjustedP);
    }

    protected override void OnDirtyUpdate(){
        path = GetInputValue<PathNode>("path", this.path);
        percentage = GetInputValue<float>("percentage", this.percentage);
    }
}