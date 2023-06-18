using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

// This class encompasses all the nodes that are addable to an AnimationGraph object
// The functions defined here allow for the modification of values to propagate through the graph and update the calculated fields within nodes.
// New nodes should implement the OnDirtyUpdate() function in order to define the behaviour when input values change.
// Dirty updates propagate through the graph based on lowest number of dirty inputs
// Code may need to be modified if there are cycles within the graph. Currently updates might stop propagating.
public abstract class AnimationGraphNode : Node
{
	[HideInInspector]
	private bool isDirty = false;
	[HideInInspector]
	private uint dirtyInputCount = 0;


	public override void OnCreateConnection(NodePort from, NodePort to)
	{
		base.OnCreateConnection(from, to);
		this.MarkDirty();
	}

	public override void OnRemoveConnection(NodePort port)
	{
		base.OnRemoveConnection(port);
		this.MarkDirty();
	}


		// Mark a node as dirty
		// Propagates dirtiness down the output tree before calling dirtyUpdate for all dirty nodes in order
	public void MarkDirty() {
		isDirty = true;
		//Debug.Log("Node " + name + ": makeDirty, Connections: " + dirtyInputCount);
		this.PropagateDirtyState();
		cleanUpDirt();
	}

	// Override to control which outputs propagate dirty state
	// Default behaviour is to propagate dirty state to all output nodes
	protected virtual void PropagateDirtyState()
	{
		PropagateDirtyState(this.Outputs);
	}

	// Override to perform calculations based on updated input values
	protected virtual void OnDirtyUpdate() {}
	private void cleanUpDirt()
	{
		OnDirtyUpdate();
		isDirty = false;
		PropagateCleanUp(Outputs);
	}

	private static void PropagateDirtyState(IEnumerable<NodePort> dirtyOutputPorts) { 
		foreach(NodePort dirtyOutputPort in dirtyOutputPorts)
		{
			if (dirtyOutputPort.IsConnected)
			{
				IEnumerable<NodePort> dirtyOutputConnections = dirtyOutputPort.GetConnections();
				foreach (NodePort dirtyInputPort in dirtyOutputConnections)
				{
					AnimationGraphNode dirtyConnectedNode = dirtyInputPort.node as AnimationGraphNode;
					if (dirtyConnectedNode != null)
					{
						dirtyConnectedNode.dirtyInputCount++;
						//Debug.Log("Node " + dirtyConnectedNode.name + ": newDirtyConnection, Connections: " + dirtyConnectedNode.dirtyInputCount);
						if (!dirtyConnectedNode.isDirty)
						{
							dirtyConnectedNode.isDirty = true;
							//Debug.Log("Node " + dirtyConnectedNode.name + ": isnowdirty, Connections: " + dirtyConnectedNode.dirtyInputCount);
							dirtyConnectedNode.PropagateDirtyState();
						}
					}
				}
			}
		}
	}
	private class AnimationGraphNodeDirtyComparer : IComparer<AnimationGraphNode>
	{
		public int Compare(AnimationGraphNode x, AnimationGraphNode y)
		{
			return x.dirtyInputCount.CompareTo(y.dirtyInputCount);
		}
	}

	private static void PropagateCleanUp(IEnumerable<NodePort> outputPorts)
	{
		List<AnimationGraphNode> dirtyConnections = new List<AnimationGraphNode>();
		foreach (NodePort outputPort in outputPorts)
		{
			if (outputPort.IsConnected)
			{
				
				IEnumerable<NodePort> outputConnections = outputPort.GetConnections();
				foreach (NodePort connectedInputPort in outputConnections)
				{
					AnimationGraphNode connectedNode = connectedInputPort.node as AnimationGraphNode;
					if (connectedNode != null && connectedNode.isDirty)
					{
						dirtyConnections.Add(connectedNode);
					}
				}
			}
		}

		while (dirtyConnections.Count > 0)
		{
			dirtyConnections.Sort(new AnimationGraphNodeDirtyComparer());
			
			AnimationGraphNode chosenNode = dirtyConnections[0]; // The node with the fewest dirty inputs
			chosenNode.dirtyInputCount--;
			// TODO: Check if this doesn't work for cyclical graphs
			if (chosenNode.dirtyInputCount == 0 && chosenNode.isDirty)
			{
				chosenNode.cleanUpDirt();
				dirtyConnections.Remove(chosenNode);
			}
			else
			{
				break;
			}
		}
	}
}