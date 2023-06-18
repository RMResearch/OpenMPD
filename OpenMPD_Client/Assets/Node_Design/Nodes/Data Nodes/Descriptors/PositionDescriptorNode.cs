using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[NodeTint(180, 70, 180)]
[CreateNodeMenu(AEConsts.MENU_OPENMPD_STRING + "Position Descriptor", AEConsts.MENU_OPENMPD_OFFSET + 1)]
// This class takes a list of Vector3 positions, and registers them with OpenMPD to create a PositionDescriptor
public class PositionDescriptorNode : DataNode {

	Positions_Descriptor positionDescriptor = null;
	float[] samplePositions;

	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3[] positionList = new Vector3[0];
	[Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public uint positionDescriptorID = 0;

	// If our inputs have changed, we probably want to regenerate our descriptor.
	protected override void OnDirtyUpdate()
	{
		GenerateDescriptor();
	}

	public void OnDestroy()
	{
		if (positionDescriptor != null && positionDescriptor.IsDisposable())
		{
			OpenMPD_ContextManager.Instance().RemoveDescriptor(positionDescriptor);
		}
	}

	// Tries to generate descriptor if none exists,
	// If it does then it returns the Descriptor ID
	public override object GetValue(NodePort port)
	{
		if (positionDescriptor == null)
		{
			GenerateDescriptor();
		}
		if (positionDescriptor != null)
		{
			return positionDescriptor.positionsDescriptorID;
		}
		return null;
	}

	// Discards any existing descriptor and then
	// Registers a new descriptor with OpenMPD
	public void GenerateDescriptor()
	{
		if(OpenMPD_PresentationManager.Instance() == null)
		{
			return;
		}

		if (positionDescriptor != null && positionDescriptor.IsDisposable())
		{
			OpenMPD_ContextManager.Instance().RemoveDescriptor(positionDescriptor);
			positionDescriptor = null;
			positionDescriptorID = 0;
		}
		positionList = GetInputValue<Vector3[]>("positionList", null);

		if(positionList == null)
        {
			positionList = new Vector3[1];
			positionList[0] = Vector3.zero;

		}
		samplePositions = new float[4 * positionList.Length];
		
		for (int i = 0; i < positionList.Length; i++)
		{
			Vector3 thisPos = positionList[i];

			samplePositions[(4 * i) + 0] = thisPos.x;
			samplePositions[(4 * i) + 1] = thisPos.y;
			samplePositions[(4 * i) + 2] = thisPos.z;
			samplePositions[(4 * i) + 3] = 1;
		}

		positionDescriptor = new Positions_Descriptor(samplePositions);
		// Retrieve Desciptor ID
		positionDescriptorID = positionDescriptor.positionsDescriptorID;
	}
}