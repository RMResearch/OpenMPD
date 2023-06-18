using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATHS_STRING + "Circle Path", AEConsts.MENU_OPENMPD_PATHS_OFFSET + (int)PathTypes.ArcPath)]
public class ArcPathNode : PathNode {

	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 centre = Vector3.zero;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float radius = 0.01f;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 axis = new Vector3(0, 1, 0);
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float startDegrees = 0.0f;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float endDegrees = 360.0f;

    public override Vector3 getPositionAtPercent(float p)
    {
        float currentAngleDeg = startDegrees + (p*(endDegrees - startDegrees));
        float currentAngleRad = (currentAngleDeg / 360.0f) * 2 * Mathf.PI;
        Vector3 pos = radius * new Vector3(Mathf.Sin(currentAngleRad), Mathf.Cos(currentAngleRad), 0);
        Quaternion rotation = Quaternion.FromToRotation(new Vector3(0, 0, 1), axis.normalized);
        pos = rotation * pos;
        pos += centre;
        return pos;
    }

    protected override void OnDirtyUpdate(){
        centre = GetInputValue<Vector3>("centre", this.centre);
        radius = GetInputValue<float>("radius", this.radius);
        axis = GetInputValue<Vector3>("axis", this.axis);
        startDegrees = GetInputValue<float>("startDegrees", this.startDegrees);
        endDegrees = GetInputValue<float>("endDegrees", this.endDegrees);
    }

    // Use this for initialization
    protected override void Init() {
		base.Init();
	}
}