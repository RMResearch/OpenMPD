using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_PRIMITIVES_STRING + "Vector3", AEConsts.MENU_PRIMITIVES_OFFSET + 1)]
[NodeTint(220, 180, 80)]
// This node combines 3 float values to create a Vector3.
public class Vector3Node : DataNode
{

	[Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public Vector3 vector;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float x = 0;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float y = 0;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float z = 0;

	// If the inputs have changed, update the stored values.
	protected override void OnDirtyUpdate()
	{
		x = GetInputValue<float>("x", x);
		y = GetInputValue<float>("y", y);
		z = GetInputValue<float>("z", z);
	}

	// Whenever the output is requested, create a new Vector3.
	public override object GetValue(NodePort port) {
		vector = new Vector3(x, y, z);
		return vector;
	}
}