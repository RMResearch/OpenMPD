using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// This class ties the AnimationGraph to a MonoBehaviour script, attaching it to an object in the scene.
// The editor for this class provides an interface for naming and loading AnimationGraphs from asset files
// This class connects the Unity Monobehaviour updates (Start, Update, OnApplicationQuit) to the PrimitiveNodes within the graph.
public class AnimationSceneGraph : MonoBehaviour // Similar to XNode.SceneGraph<AnimationGraph>
{
    public AnimationGraph graph;

    public void Start()
    {
        foreach (PrimitiveNode pn in this.graph.primitives)
        {
            pn.OnStart();
        }
    }

    public void Update()
    {
        if (this.graph != null)
        {
            foreach (PrimitiveNode pn in this.graph.primitives)
            {
                pn.OnUpdate();
            }
        }
    }

    public void OnApplicationQuit()
    {
        foreach (PrimitiveNode pn in this.graph.primitives)
        {
            pn.OnStop();
        }
    }
}
