using UnityEditor;
using UnityEngine;

[CustomEditor(typeof(AnimationGraphNode), true)]
[CanEditMultipleObjects]
// Used to hide the data / member variables of Graph Nodes by default
public class AnimationNodeInspectorEditor : Editor
{
    public bool showContent;
    public override void OnInspectorGUI()
    {
        serializedObject.Update();

        //if (GUILayout.Button("Edit graph", GUILayout.Height(40))) {
        //    XNodeEditor.NodeEditorWindow.Open(serializedObject.targetObject as XNode.NodeGraph);
        //}

        //GUILayout.Label("States", "BoldLabel");
        //AnimationGraph graph = serializedObject.targetObject as AnimationGraph;
        //if (graph != null) {
        //    for (int i = 0; i < graph.states.Count; i++)
        //    {
        //        if (GUILayout.Button(graph.states[i].name.ToString(), GUILayout.Height(25)))
        //        {
        //            Debug.Log(graph.states[i].name.ToString());
        //            graph.states[i].EnterState();
        //        }
        //    }
        //}

        //GUILayout.Space(EditorGUIUtility.singleLineHeight);
        //GUILayout.Label("Raw data", "BoldLabel");

        showContent = EditorGUI.Foldout(GUILayoutUtility.GetRect(EditorGUIUtility.currentViewWidth, EditorGUIUtility.singleLineHeight), showContent, "Raw Data");
        if (showContent)
        {
            DrawDefaultInspector();
        }

        serializedObject.ApplyModifiedProperties();
    }
}