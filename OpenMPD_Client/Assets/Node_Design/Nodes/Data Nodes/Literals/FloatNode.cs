using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_PRIMITIVES_STRING + "Float", AEConsts.MENU_PRIMITIVES_OFFSET + 0)]
[NodeTint(120, 120, 220)]
// This node stores and outputs a single float value.
public class FloatNode : DataNode {

	[Output(ShowBackingValue.Always, ConnectionType.Multiple, TypeConstraint.Inherited)] public float value;

	// Return the correct value of an output port when requested
	public override object GetValue(NodePort port) {
		return value;
	}
}