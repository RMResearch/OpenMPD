using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATHS_STRING + "Oval Path", AEConsts.MENU_OPENMPD_PATHS_OFFSET + (int)PathTypes.OvalPath)]
// This class describes a circlular path.
// The path is defined as one full rotation around given axis, centered on a given position, with a set radial offset from the axis.
// The path startPosition can be adjusted via the offsetDegrees value.
public class OvalPathNode : PathNode {

	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 centre = Vector3.zero;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float radius1 = 0.01f;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float radius2 = 0.01f;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 axis = new Vector3(0, 1, 0);
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float offsetDegrees = 0.0f;

    public override Vector3 getPositionAtPercent(float p)
    {
        // Convert percentage P and offsetDegrees to rotation in radians
        float currentAngleRad = ((offsetDegrees/360.0f) + p) * 2 * Mathf.PI;
        // Create position from rotation and radii
        Vector3 pos = new Vector3(radius1 * Mathf.Sin(currentAngleRad), radius2 * Mathf.Cos(currentAngleRad), 0);
        // Calculate the rotation required for the rotational axis to match the parameter
        Quaternion rotation = Quaternion.FromToRotation(new Vector3(0, 0, 1), axis.normalized);
        // Combine rotation with position
        pos = rotation * pos;
        // Shift the centre of the generated circle
        pos += centre;
        return pos;
    }

    protected override void OnDirtyUpdate(){
        centre = GetInputValue<Vector3>("centre", this.centre);
        radius1 = GetInputValue<float>("radius1", this.radius1);
        radius2 = GetInputValue<float>("radius2", this.radius2);
        axis = GetInputValue<Vector3>("axis", this.axis);
        offsetDegrees = GetInputValue<float>("offsetDegrees", this.offsetDegrees);
    }

    // Use this for initialization
    protected override void Init() {
		base.Init();
	}
}