using UnityEditor;
using UnityEngine;

[CustomEditor(typeof(AnimationGraph), true)]
[CanEditMultipleObjects]

// Custom editor for the AnimationGraph object.
// Draws a list of States with buttons to enter each of the states represented by StateCollectionNodes
// Also draws a list of GameObjectReferences so objects can be easily dragged and dropped to assign them.
public class AnimationGraphEditor : Editor {

    public bool showContent = false;
    AnimationGraphEditor()
    {
        
    }
    public override void OnInspectorGUI() {
        serializedObject.Update();

        if (GUILayout.Button("Edit graph", GUILayout.Height(40))) {
            XNodeEditor.NodeEditorWindow.Open(serializedObject.targetObject as XNode.NodeGraph);
        }

        GUILayout.Label("States", "BoldLabel");
        AnimationGraph graph = serializedObject.targetObject as AnimationGraph;
        if (graph != null) {
            for (int i = 0; i < graph.states.Count; i++)
            {
                if (GUILayout.Button(graph.states[i].name.ToString(), GUILayout.Height(25)))
                {
                    Debug.Log("Entering State: " + graph.states[i].name.ToString());
                    graph.EnterState(graph.states[i]);
                }
            }
        }

        GUILayout.Label("GameObject References", "BoldLabel");
        if (graph != null)
        {
            for (int i = 0; i < graph.gameObjectReferences.Count; i++)
            {
                graph.gameObjectReferences[i].sceneObject = (GameObject) EditorGUILayout.ObjectField(graph.gameObjectReferences[i].name.ToString(), graph.gameObjectReferences[i].sceneObject, typeof(GameObject));
                //EditorGUI.PropertyField(GUILayoutUtility.GetRect(EditorGUIUtility.currentViewWidth, EditorGUIUtility.singleLineHeight), );//, false, graph.gameObjectReferences[i].name.ToString());
                //EditorGUI.(graph.gameObjectReferences[i].goob2);
                //if (GUILayout.(graph.gameObjectReferences[i].name.ToString(), GUILayout.Height(25)))
                //{
                //    Debug.Log(graph.gameObjectReferences[i].name.ToString());
                //    graph.gameObjectReferences[i].EnterState();
                //}
            }
        }

        GUILayout.Space(EditorGUIUtility.singleLineHeight);
        showContent = EditorGUI.Foldout(GUILayoutUtility.GetRect(EditorGUIUtility.currentViewWidth, EditorGUIUtility.singleLineHeight), showContent, "Raw Data");
        if (showContent)
        {
            DrawDefaultInspector();
        }

        serializedObject.ApplyModifiedProperties();
    }

    public static void OnUpdateNodeCallback(XNode.Node changedNode)
    {
        DataNode changedDataNode = changedNode as DataNode;
        if(changedDataNode != null)
        {
            changedDataNode.MarkDirty();
        }
    }
}