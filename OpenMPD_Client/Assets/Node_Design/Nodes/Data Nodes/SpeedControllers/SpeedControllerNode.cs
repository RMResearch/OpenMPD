using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

// Previous approach used delegates for output, not sure which approach is cleaner.
// public delegate float[] SamplingFunction(float[] Path);


[NodeTint(70, 180, 180)]
// This class generates a list of floats which is then fed into a PathNode to
// calculate a list of positions, defining the sequential positions of the primitive
// and thus the trajectory and speed at which the primitive moves
public abstract class SpeedControllerNode : DataNode {

	[Output] public SpeedControllerNode SpeedControl;

	// Default SpeedControllerNodes only have one output.
	// If another output is added, this should be overidden.
	// TODO: add output count assertion check to catch errors.
	public override object GetValue(NodePort port)
	{
		return this;
	}

	// Return a distribution of sample percentages (0.0 - 1.0) representing (0% - 100%)
	// e.g. uniform distribution between 0 and 1 results in constant speed
	// e.g. distribution that decreases in density (more smaller values than larger values)
	// will result in acceleration
	public abstract float[] GenerateSamplePercentages();
}