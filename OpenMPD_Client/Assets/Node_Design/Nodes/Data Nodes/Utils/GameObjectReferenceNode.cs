using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_UTILITIES_STRING + "GameObject Reference", AEConsts.MENU_UTILITIES_OFFSET + 0)]
// This node is used to provide a link to a GameObject that exists in the current scene
// The active state, position, rotation, and scale are sampled whenever the port values are read.
public class GameObjectReferenceNode : UtilityNode {
    [Output(ShowBackingValue.Always, ConnectionType.Multiple, TypeConstraint.Inherited)] public GameObject sceneObject;
    [Output(ShowBackingValue.Always, ConnectionType.Multiple, TypeConstraint.Inherited)] public bool isActive = true;
    [Output(ShowBackingValue.Always, ConnectionType.Multiple, TypeConstraint.Inherited)] public Vector3 scenePosition;
    [Output(ShowBackingValue.Always, ConnectionType.Multiple, TypeConstraint.Inherited)] public Vector3 rotation;
    [Output(ShowBackingValue.Always, ConnectionType.Multiple, TypeConstraint.Inherited)] public Vector3 scale;

    public override object GetValue(NodePort port)
    {
        if (sceneObject != null)
        {
            isActive = sceneObject.activeInHierarchy;
            scenePosition = sceneObject.transform.position;
            rotation = sceneObject.transform.rotation.eulerAngles;
            scale = sceneObject.transform.lossyScale;
        }
        switch (port.fieldName)
        {
            case "isActive":
                return isActive;
            case "sceneObject":
                return sceneObject;
            case "scenePosition":
                return scenePosition;
            case "rotation":
                return rotation;
            case "scale":
                return scale;
        }
        return null;
    }

    protected override void OnDirtyUpdate()
    {
        if (sceneObject != null)
        {
            isActive = sceneObject.activeInHierarchy;
            scenePosition = sceneObject.transform.position;
            rotation = sceneObject.transform.rotation.eulerAngles;
            scale = sceneObject.transform.lossyScale;
        }
    }
}