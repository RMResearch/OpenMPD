using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[NodeTint(140, 100, 200)]
[CreateNodeMenu(AEConsts.MENU_OPENMPD_STRING + "State Collection", AEConsts.MENU_OPENMPD_OFFSET + 2)]
// This node is used to enqueue sequences of PrimitiveStates to the OpenMPD engine
// A dynamic port list is used to vary the number of input PrimitiveState ports.
public class StateCollectionNode : DataNode {

	[Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public bool isActiveState = false;
	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited, dynamicPortList = true)] public PrimitiveStateNode[] primitiveStates = new PrimitiveStateNode[0];

	public override object GetValue(NodePort port)
	{
		if(port.fieldName == "isActiveState")
		{
			return isActiveState;
		}
		return null;
	}

	public void EnterState()
	{
		isActiveState = true;
		MarkDirty();

		/*
		// Old approach (before dirty node propagation) was to forcibly push updates to all output connections,
		// Code kept for future reference
		// Set enable state of all connected primitives
		NodePort setEnabledPort = GetPort("onEnterSetEnabled");
		foreach (NodePort connectedToPorts in setEnabledPort.GetConnections())
		{
			Node connectedToNode = connectedToPorts.node;
			PrimitiveNode connectedToPrimitive = connectedToNode as PrimitiveNode;
			if(connectedToPrimitive != null)
			{
				connectedToPrimitive.setPrimitiveEnabled(true);
			}
			else
			{
				Debug.LogError("Set Enable connected to non-primitive node");
				return;
			}
		}
		*/

		foreach (PrimitiveStateNode n in primitiveStates)
		{
			if (n != null)
			{
				n.Enqueue();
			}
		}
	}

	public override void UpdatePorts()
	{
		base.UpdatePorts();
		for (int i = 0; i < primitiveStates.Length; i++)
		{
			NodePort primitiveStatePort = GetPort("primitiveStates " + i.ToString());
			if (primitiveStatePort != null)
			{
				PrimitiveStateNode primitiveStatePortValue = (PrimitiveStateNode)primitiveStatePort.GetInputValue();
				primitiveStates[i] = primitiveStatePortValue;
			}
			else
			{
				primitiveStates[i] = null;
			}
		}
	}

	public override void OnCreateConnection(NodePort from, NodePort to) {
		base.OnCreateConnection(from, to);

		for(int i = 0; i < primitiveStates.Length; i++){
			NodePort primitiveStatePort = GetPort("primitiveStates " + i.ToString());
			if (primitiveStatePort != null)
			{
				PrimitiveStateNode primitiveStatePortValue = (PrimitiveStateNode)primitiveStatePort.GetInputValue();
				primitiveStates[i] = primitiveStatePortValue;
			}
			else
			{
				primitiveStates[i] = null;
			}
		}
	}

	public override void OnRemoveConnection(NodePort port)
	{
		base.OnRemoveConnection(port);

		for (int i = 0; i < primitiveStates.Length; i++)
		{
			NodePort primitiveStatePort = GetPort("primitiveStates " + i.ToString());
			if (primitiveStatePort != null)
			{
				PrimitiveStateNode primitiveStatePortValue = (PrimitiveStateNode)primitiveStatePort.GetInputValue();
				primitiveStates[i] = primitiveStatePortValue;
			}
			else
			{
				primitiveStates[i] = null;
			}
		}
	}

}