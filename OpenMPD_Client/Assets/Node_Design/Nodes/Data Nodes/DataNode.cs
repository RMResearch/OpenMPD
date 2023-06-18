using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[NodeTint(100, 70, 70)]
// This class exists to allow for a separation between "data nodes" which represent data or references to variables and
// "execution nodes" which would determine program flow and do the reading / writing of data.
// Execution nodes do not currently exist but they would provide a solution for real-time modification of graph nodes, eg. due to event-based callbacks.
public abstract class DataNode : AnimationGraphNode
{
}