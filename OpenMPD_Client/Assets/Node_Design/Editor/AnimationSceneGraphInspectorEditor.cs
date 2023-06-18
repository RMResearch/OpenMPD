using System;
using UnityEditor;
using UnityEngine;
using XNodeEditor;
using XNode;

[CustomEditor(typeof(AnimationSceneGraph), true)]
[CanEditMultipleObjects]
// Script for describing how the inspector editor for AnimationSceneGraph MonoBehaviours should work
// Provides UI for new / load / save functionality
public class AnimationSceneGraphInspectorEditor : Editor
{
    private AnimationSceneGraph animationSceneGraph;
    private bool removeSafely;
    private Type graphType;
    private bool layoutRefreshed = false;
    private Editor _editor;

    public override void OnInspectorGUI()
    {
        if (!layoutRefreshed && Event.current.type != EventType.Layout)
            return;

        if (Event.current.type == EventType.Layout)
        {
            layoutRefreshed = true;
        }

        if (animationSceneGraph.graph == null)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label("Graph:");
            GUILayout.FlexibleSpace();
            GUILayout.Label("No Graph.");
            GUILayout.EndHorizontal();

            GUILayout.BeginHorizontal();
            if (GUILayout.Button("New Graph", GUILayout.Height(40)))
            {
                if (graphType == null)
                {
                    Type[] graphTypes = NodeEditorReflection.GetDerivedTypes(typeof(AnimationGraph));
                    if (graphTypes.Length == 1)
                    {
                        CreateGraph(graphTypes[0]);
                    }
                    else
                    {
                        GenericMenu menu = new GenericMenu();
                        for (int i = 0; i < graphTypes.Length; i++)
                        {
                            Type graphType = graphTypes[i];
                            menu.AddItem(new GUIContent(graphType.Name), false, () => CreateGraph(graphType));
                        }
                        menu.ShowAsContext();
                    }
                }
                else
                {
                    CreateGraph(graphType);
                }
                layoutRefreshed = false;
            }
            if (GUILayout.Button("Load Graph", GUILayout.Height(40)))
            {
                EditorGUIUtility.ShowObjectPicker<AnimationGraph>(null, false, "", 0);
            }
            GUILayout.EndHorizontal();
            if (Event.current.commandName == "ObjectSelectorClosed")
            {
                UnityEngine.Object chosenObject = EditorGUIUtility.GetObjectPickerObject();
                AnimationGraph chosenGraph = chosenObject as AnimationGraph;
                if (chosenGraph != null)
                {
                    CreateSceneGraphFromAsset(chosenGraph);
                    layoutRefreshed = false;
                }
            }
        }
        else
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label("Graph:");
            //GUILayout.FlexibleSpace();
            animationSceneGraph.graph.name = GUILayout.TextField(animationSceneGraph.graph.name);
            //GUILayout.Label(animationSceneGraph.graph.name);
            GUILayout.EndHorizontal();

            //if (GUILayout.Button("Open Graph", GUILayout.Height(40)))
            //{
            //    NodeEditorWindow.Open(animationSceneGraph.graph);
            //}
            if (removeSafely)
            {
                GUILayout.BeginHorizontal();
                GUILayout.Label("Really remove graph?");
                GUI.color = new Color(1, 0.8f, 0.8f);
                if (GUILayout.Button("Remove"))
                {
                    removeSafely = false;
                    Undo.RecordObject(animationSceneGraph, "Removed graph");
                    animationSceneGraph.graph = null;
                }
                GUI.color = Color.white;
                if (GUILayout.Button("Cancel"))
                {
                    removeSafely = false;
                }
                GUILayout.EndHorizontal();
            }
            else
            {
                GUILayout.BeginHorizontal();
                if (GUILayout.Button("Save to file"))
                {
                    string path = EditorUtility.SaveFilePanelInProject("Save animation graph to asset", animationSceneGraph.graph.name, "asset", "Hello");
                    if (path.Length != 0)
                    {
                        XNode.NodeGraph existingGraph = AssetDatabase.LoadAssetAtPath<XNode.NodeGraph>(path);
                        if (existingGraph != null) AssetDatabase.DeleteAsset(path);
                        AnimationGraph copiedGraph = (AnimationGraph)animationSceneGraph.graph.Copy();
                        AssetDatabase.CreateAsset(copiedGraph, path);
                        AssetDatabase.SetMainObject(copiedGraph, path);
                        foreach(Node n in copiedGraph.nodes)
                        {
                            if (n != null)
                            {
                                AssetDatabase.AddObjectToAsset(n, path);
                            }
                        }
                        EditorUtility.SetDirty(copiedGraph);
                        AssetDatabase.SaveAssets();
                    }
                }
                GUI.color = new Color(1, 0.8f, 0.8f);
                if (GUILayout.Button("Remove graph"))
                {
                    removeSafely = true;
                }
                GUI.color = Color.white;
                GUILayout.EndHorizontal();
            }
        }
        //DrawDefaultInspector();
        if (animationSceneGraph.graph != null)
        {
            CreateCachedEditor(animationSceneGraph.graph, null, ref _editor);
            _editor.OnInspectorGUI();
        }
        //animationSceneGraph.graph

        serializedObject.ApplyModifiedProperties();
    }

    private void OnEnable()
    {
        animationSceneGraph = target as AnimationSceneGraph;
        Type sceneGraphType = animationSceneGraph.GetType();
        if (sceneGraphType == typeof(SceneGraph))
        {
            graphType = null;
        }
        else
        {
            Type baseType = sceneGraphType.BaseType;
            if (baseType.IsGenericType)
            {
                graphType = sceneGraphType = baseType.GetGenericArguments()[0];
            }
        }
    }

    public void CreateGraph(Type type)
    {
        Undo.RecordObject(animationSceneGraph, "Create graph");
        animationSceneGraph.graph = ScriptableObject.CreateInstance(type) as AnimationGraph;
        animationSceneGraph.graph.name = animationSceneGraph.name + "-graph";
    }

    public void CreateSceneGraphFromAsset(AnimationGraph asset)
    {
        Undo.RecordObject(animationSceneGraph, "Create graph");
        animationSceneGraph.graph = (AnimationGraph)asset.Copy();
        //animationSceneGraph.graph.name = animationSceneGraph.name + "-graph";
    }
}