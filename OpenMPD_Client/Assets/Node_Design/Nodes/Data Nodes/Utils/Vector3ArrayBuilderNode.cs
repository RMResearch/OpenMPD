using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_UTILITIES_STRING + "Vector 3 Array Builder", AEConsts.MENU_UTILITIES_OFFSET + 1)]
[NodeWidth(300)]
// This node was an example of how to combine an abstract number of Vector3 inputs to create an output array of type Vector3
// It can be used to manually define position lists.
public class Vector3ArrayBuilderNode : ArrayBuilderNode<Vector3> {
}