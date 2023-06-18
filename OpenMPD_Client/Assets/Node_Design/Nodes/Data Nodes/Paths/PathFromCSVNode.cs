using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[NodeTint(180, 70, 180)]
[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATHS_STRING + "Path From CSV", AEConsts.MENU_OPENMPD_PATHS_OFFSET + (int)PathTypes.CSVPath)]
// This node can be used to load a path list from CSV file
public class PathFromCSVNode : DataNode {

	private Vector3[] loadedPositions;
	public String filePath = "";
	private String lastFilePath = "";

	[Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public Vector3[] positionList = new Vector3[0];

	// If filepath changes
	protected override void OnDirtyUpdate()
	{
		if(filePath != null && filePath != ""){
			if(filePath != lastFilePath || (positionList.Length == 0)){
				// if file exists
					// try read CSV
					TryReadPositionList();
				//
				lastFilePath = filePath;
			}
		}
	}

	public override object GetValue(NodePort port)
	{
		if (positionList.Length == 0)
		{
			TryReadPositionList();
		}
		return positionList;
	}

	public void TryReadPositionList()
	{
		if (System.IO.File.Exists(System.IO.Directory.GetCurrentDirectory() + "\\Assets\\" + filePath + ".csv"))
		{
			try
			{
				WriteData rw = new WriteData();
				List<Vector3> positionsFromFile = new List<Vector3>();
				rw.readFromFileVec3(System.IO.Directory.GetCurrentDirectory() + "\\Assets\\" + filePath, ref positionsFromFile);
				positionList = positionsFromFile.ToArray();
			} catch (Exception e)
			{
				Debug.LogError(e.ToString() + ": " + e.Message);
			}
		}
		else
		{
			Debug.LogError("File not found: " + System.IO.Directory.GetCurrentDirectory() + "\\Assets\\" + filePath + ".csv");
			positionList = new Vector3[0];
		}

	}
}