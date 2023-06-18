using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

//[CreateAssetMenu(fileName = "New Animation Graph", menuName = "Animation Designer/Animation Graph")]
[System.Serializable]
// This AnimationGraph class is the OpenMPD specific adaptation of the XNode graph.
// It stores lists of StateCollection nodes and GameObjectReference nodes for display in the editor
// The list of PrimitiveNodes is used by the AnimationSceneGraph MonoBehaviour to apply unity callbacks such as Update()
public class AnimationGraph : NodeGraph
{

    [HideInInspector]
    public List<StateCollectionNode> states = new List<StateCollectionNode>();
    [HideInInspector]
    public List<PrimitiveNode> primitives = new List<PrimitiveNode>();
    [HideInInspector]
    public List<GameObjectReferenceNode> gameObjectReferences = new List<GameObjectReferenceNode>();

    AnimationGraph()
    {
        XNodeEditor.NodeEditor.onUpdateNode = OnUpdateCallback;
    }

    public static void OnUpdateCallback(Node changedNode)
    {
        NodeGraph g = changedNode.graph;
        //Debug.Log("Node Changed: " + changedNode.name);
        AnimationGraphNode changedAGNode = changedNode as DataNode;
        if (g is AnimationGraph && changedAGNode != null)
        {
            changedAGNode.MarkDirty();
        }
    }

    public override Node AddNode(Type type)
    {
        Node newNode = base.AddNode(type);
        if (type == typeof(StateCollectionNode))
        {
            newNode.name = FindFirstAvailableName(states, "State Collection ");
            states.Add((StateCollectionNode)newNode);
        }
        if (type == typeof(PrimitiveNode))
        {
            newNode.name = FindFirstAvailableName(primitives, "Primitive ");
            primitives.Add((PrimitiveNode)newNode);
        }
        if (type == typeof(GameObjectReferenceNode))
        {
            newNode.name = FindFirstAvailableName(gameObjectReferences, "GameObject Reference ");
            gameObjectReferences.Add((GameObjectReferenceNode)newNode);
        }
        return newNode;
    }

    private string FindFirstAvailableName<T>(List<T> namedNodes, string nameTemplate) where T : Node
    {
        HashSet<string> usedNodeNames = new HashSet<string>();
        for (int i = 0; i < namedNodes.Count; i++)
        {
            if (namedNodes[i] == null) // Corrupt data workaround
            {
                continue;
            }
            usedNodeNames.Add(namedNodes[i].name);
        }
        for (int i = 0; i < namedNodes.Count + 1; i++)
        {
            string newName = nameTemplate + (i + 1).ToString();
            if (!usedNodeNames.Contains(newName))
            {
                return newName;
            }
        }
        return "";
    }

    public override XNode.NodeGraph Copy()
    {
        AnimationGraph copiedGraph = (AnimationGraph)base.Copy();

        // Update new graph's state list pointers to aim at new graph's nodes
        for (int i = 0; i < states.Count; i++)
        {
            if (states[i] != null)
            {
                int index = nodes.IndexOf(states[i]);
                if (index >= 0) copiedGraph.states[i] = (StateCollectionNode)copiedGraph.nodes[index];
            }
        }
        for (int i = 0; i < primitives.Count; i++)
        {
            if (primitives[i] != null)
            {
                int index = nodes.IndexOf(primitives[i]);
                if (index >= 0) copiedGraph.primitives[i] = (PrimitiveNode)copiedGraph.nodes[index];
            }
        }
        for (int i = 0; i < gameObjectReferences.Count; i++)
        {
            if (gameObjectReferences[i] != null)
            {
                int index = nodes.IndexOf(gameObjectReferences[i]);
                if (index >= 0) copiedGraph.gameObjectReferences[i] = (GameObjectReferenceNode)copiedGraph.nodes[index];
            }
        }

        copiedGraph.name = copiedGraph.name.Replace("(Clone)", "");
        foreach (Node n in copiedGraph.nodes)
        {
            if (n != null)
            {
                n.name = n.name.Replace("(Clone)", "");
            }
        }
        return copiedGraph;
    }

    public override void RemoveNode(Node node)
    {
        StateCollectionNode thisState = node as StateCollectionNode;
        if (thisState != null)
        {
            if (states.Contains(thisState))
            {
                states.Remove(thisState);
            }
        }
        PrimitiveNode thisPrimitive = node as PrimitiveNode;
        if (thisPrimitive != null)
        {
            if (primitives.Contains(thisPrimitive))
            {
                primitives.Remove(thisPrimitive);
            }
        }
        GameObjectReferenceNode thisNode = node as GameObjectReferenceNode;
        if (thisNode != null)
        {
            if (gameObjectReferences.Contains(thisNode))
            {
                gameObjectReferences.Remove(thisNode);
            }
        }
        base.RemoveNode(node);
    }

    public void EnterState(StateCollectionNode chosenState)
    {
        foreach (StateCollectionNode stateNode in states)
        {
            if (stateNode != chosenState)
            {
                stateNode.isActiveState = false;
                stateNode.MarkDirty();
            }
        }
        chosenState.EnterState();
    }

    public void EnterState(String stateName)
    {
        foreach (StateCollectionNode stateNode in states)
        {
            if (stateNode.name.Equals(stateName))
            {
                EnterState(stateNode);
                break;
            }
        }
    }
}