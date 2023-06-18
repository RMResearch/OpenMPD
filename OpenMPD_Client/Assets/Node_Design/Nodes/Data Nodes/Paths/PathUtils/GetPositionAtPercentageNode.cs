using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATH_UTILITIES_STRING + "Position At Percentage", AEConsts.MENU_OPENMPD_PATH_UTILITIES_OFFSET + (int)PathUtilities.PositionAtPercentage)]
[NodeWidth(300)]
// This node can be used extract the position on a path at a particular percentage.
// It simply calls the getPositionAtPercent(float p) function for the given parameter.
public class GetPositionAtPercentageNode : DataNode {

    [Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public PathNode path = null;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float percentage = 1.0f;
    [Output(ShowBackingValue.Always, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 sampledPosition = Vector3.zero;

    public override object GetValue(NodePort port)
    {
        path = GetInputValue<PathNode>("path", this.path);
        percentage = GetInputValue<float>("percentage", this.percentage);
        if (path != null)
        {
            sampledPosition = path.getPositionAtPercent(percentage);
        }
        else
        {
            sampledPosition = Vector3.zero;
        }
        return sampledPosition;
    }

    protected override void OnDirtyUpdate(){
        path = GetInputValue<PathNode>("path", this.path);
        percentage = GetInputValue<float>("percentage", this.percentage);
        if(path != null){
            sampledPosition = path.getPositionAtPercent(percentage);
        }else{
            sampledPosition = Vector3.zero;
        }
    }
}