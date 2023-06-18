using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATHS_STRING + "Circle Path", AEConsts.MENU_OPENMPD_PATHS_OFFSET + (int)PathTypes.CirclePath)]
// This class describes a circlular path.
// The path is defined as one full rotation around given axis, centered on a given position, with a set radial offset from the axis.
// The path startPosition can be adjusted via the offsetDegrees value.
public class CirclePathNode : PathNode {

	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 centre = Vector3.zero;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float radius = 0.01f;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 axis = new Vector3(0, 1, 0);
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float offsetDegrees = 0.0f;

    public override Vector3 getPositionAtPercent(float p)
    {
        // Convert percentage P and offsetDegrees to rotation in radians
        float currentAngleRad = ((offsetDegrees/360.0f) + p) * 2 * Mathf.PI;
        // Create position from rotation and radius
        Vector3 pos = radius * new Vector3(Mathf.Sin(currentAngleRad), Mathf.Cos(currentAngleRad), 0);
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
        radius = GetInputValue<float>("radius", this.radius);
        axis = GetInputValue<Vector3>("axis", this.axis);
        offsetDegrees = GetInputValue<float>("offsetDegrees", this.offsetDegrees);
    }
}