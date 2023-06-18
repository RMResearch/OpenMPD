using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[NodeTint(180, 180, 70)]
[NodeWidth(300)]
[CreateNodeMenu(AEConsts.MENU_OPENMPD_STRING + "Primitive State", AEConsts.MENU_OPENMPD_OFFSET + 1)]
// This node describes what is referred to as a PrimitiveState, an association between a ParticlePrimitive and a set of Position, Ampliture, and Colour descriptors.
// Once a PrimitiveState is committed to OpenMPD, the content of that state will loop unless a second state has been committed to come after it.
// If any of the descriptor IDs are not set, the default descriptors will be used.
public class PrimitiveStateNode : DataNode {

	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public PrimitiveNode particlePrimitive;
	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public uint positionDescriptorID = 0;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public uint startingPositionSample = 0;
	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public uint amplitudeDescriptorID = 0;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public uint startingAmplitudeSample = 0;
	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public uint colourDescriptorID = 0;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public uint startingColourSample = 0;
	[Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public PrimitiveStateNode primtiveState;

	// Use this for initialization
	protected override void Init() {
		base.Init();
	}

	public override object GetValue(NodePort port)
	{
		return this;
	}

	public void Enqueue()
	{
		particlePrimitive = GetInputValue<PrimitiveNode>("particlePrimitive", null);
		if (particlePrimitive == null || particlePrimitive.primitiveID == 0 || !particlePrimitive.isEnabled) {
			Debug.LogError(this.name + ": Particle Primitive not attached or not ready");
			return;
		}

		OpenMPD_ContextManager cm = OpenMPD_ContextManager.Instance();
		// Load descriptorIDs, otherwise use default descriptor IDs
		positionDescriptorID = GetInputValue<uint>("positionDescriptorID", cm.GetDefaultPositionsDescriptor());
		amplitudeDescriptorID = GetInputValue<uint>("amplitudeDescriptorID", cm.GetDefaultAmplitudesDescriptor());
		colourDescriptorID = GetInputValue<uint>("colourDescriptorID", cm.GetDefaultColoursDescriptor());

		startingPositionSample = GetInputValue<uint>("startingPositionSample", 0);
		startingAmplitudeSample = GetInputValue<uint>("startingAmplitudeSample", 0);
		startingColourSample = GetInputValue<uint>("startingColourSample", 0);
		OpenMPD_ContextManager.Instance().UseDescriptors(particlePrimitive.primitiveID, positionDescriptorID, amplitudeDescriptorID, startingPositionSample, startingAmplitudeSample);
		OpenMPD_ContextManager.Instance().UseColoursDescriptor(particlePrimitive.primitiveID, colourDescriptorID, startingColourSample);
		OpenMPD_PresentationManager.Instance().RequestCommit();
	}
}