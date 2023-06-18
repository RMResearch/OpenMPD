using System.Collections;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

[CustomPropertyDrawer(typeof(XNode.NodeGraph))]
public class AGDrawer : PropertyDrawer
{
    private Editor editor = null;
    public override void OnGUI(Rect position, SerializedProperty prop, GUIContent label)
    {
        EditorGUI.PropertyField(position, prop, label, true);

        if (prop.objectReferenceValue != null)
        {
            prop.isExpanded = EditorGUI.Foldout(position, prop.isExpanded, GUIContent.none);
        }

        if (prop.objectReferenceValue != null && prop.isExpanded)
        {
            EditorGUI.indentLevel++;

            if (!editor)
            {
                Editor.CreateCachedEditor(prop.objectReferenceValue, null, ref editor);
            }
            editor.OnInspectorGUI();
            EditorGUI.indentLevel--;
        }
    }
}
