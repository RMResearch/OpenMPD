using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[NodeTint(180, 70, 180)]
[CreateNodeMenu("Path Sampler", AEConsts.MENU_OPENMPD_SPEED_CONTROLLERS_OFFSET + 10)]
// This node combines a SpeedControllerNode with a PathNode in order to generate a list of positions.
public class PathSamplerNode : DataNode {

	float[] samplePositions;

	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public PathNode path;
	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public SpeedControllerNode sampleGenerator;
	[Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public Vector3[] positionList = new Vector3[0];


	protected override void OnDirtyUpdate()
	{
		GeneratePositionList();
	}

	public override object GetValue(NodePort port)
	{
		if (positionList.Length == 0)
		{
			GeneratePositionList();
		}
		return positionList;
	}//GetInputValue<uint>("colourDescriptorID", cm.GetDefaultColoursDescriptor());

	public void GeneratePositionList()
	{
		sampleGenerator = GetInputValue<SpeedControllerNode>("sampleGenerator", null);

		float[] samplePercentages;
		if (sampleGenerator == null)
		{
			samplePercentages = new float[1] { 0 };
		}
		else
		{
			samplePercentages = sampleGenerator.GenerateSamplePercentages();
		}

		path = GetInputValue<PathNode>("path", null);

		positionList = new Vector3[samplePercentages.Length];
		for (int i = 0; i < samplePercentages.Length; i++)
		{
			Vector3 thisPos;
			if (path == null)
			{
				thisPos = new Vector3(0, 0, 0);
			}
			else
			{
				thisPos = path.getPositionAtPercent(samplePercentages[i]);
			}
			positionList[i] = thisPos;
		}

	}
}