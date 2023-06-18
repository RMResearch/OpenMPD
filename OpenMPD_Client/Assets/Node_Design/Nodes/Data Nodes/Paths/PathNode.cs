using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

// Previous attempt used delegates for output, not sure which approach is cleaner.
// public delegate float PathSamplingFunction(float percentage);

[NodeTint(70, 180, 70)]
// This class describes a data node which provides the means of sampling a path.
// All PathNodes provide the getPositionAtPercent(float p) function which should
// return a position based on progress along a path described with p (0.0 - 1.0)
// representing (0% - 100%)
public abstract class PathNode : DataNode {

	// Output a reference to this object so that the
	// getPositionAtPercent function can be called.
	[Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public PathNode Path;

	// Default PathNodes only have one output.
	// If another output is added, this should be overidden.
	// TODO: add output count assertion check to catch errors.
	public override object GetValue(NodePort port)
	{
		return this;
	}

	// Return a position based on progress along a path described with p (0.0 - 1.0)
	// representing (0% - 100%)
	public abstract Vector3 getPositionAtPercent(float p);
}