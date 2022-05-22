using UnityEngine;
using UnityEditor;
using System.Runtime.InteropServices;
using System;

public class OpenMPD_Wrapper : MonoBehaviour
{
	
	static string Data = "";
	public static void InitializeString()
    {
		Data = "";
    }

	public static string GetString()
    {
		return Data;
    }

	public static void SetLabelFlag(string label, bool isIni)
    {
        if (isIni)
			Data = label + "\n";
		else
			Data += label + "\n";
	}
	public static void PrintMessage(string s)
	{
		//Debug.Log("New Message");
		Debug.Log("New Message: " + s);
		//Data += s + DateTime.Now.Hour.ToString() + "," + DateTime.Now.Minute.ToString() + "," + DateTime.Now.Second.ToString() + "," + DateTime.Now.Millisecond.ToString() + "\n"; 
	}

	public static void PrintWarning(string s)
	{
		Debug.LogWarning("Warning: " + (s));
	}

	public static void PrintError(string s)
	{
		// Debug.LogError(PluginString(s));
		Debug.LogError("Error: " + (s));
	}

	//General methods for Engine (setup, configuration, execution...):
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern bool OpenMPD_CWrapper_Initialize();

	public delegate void PrintFunc(string s);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_RegisterPrintFuncs(PrintFunc printMessage, PrintFunc printWarning, PrintFunc printError);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_SetupEngine(long memorySizeInBytes, int version, long gl_renderer=0);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern long OpenMPD_CWrapper_StartEngine(byte FPS_Divider, uint numParallelGeometries, uint topBoardID, uint bottomBoardID, bool forceSync);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_SetupFPS_Divider(byte FPS_Divider);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_SetupPhaseOnly(bool phaseOnly);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_SetupHardwareSync(bool isHWSync);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	
	public static extern bool OpenMPD_CWrapper_StopEngine();
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern bool OpenMPD_CWrapper_Release();
	//Primitive Manager methods:
	//1. Defining low-level descriptors
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern uint OpenMPD_CWrapper_createPositionsDescriptor(long primitiveManagerHandler, float[] positions, int numPosSamples);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern uint OpenMPD_CWrapper_createAmplitudesDescriptor(long primitiveManagerHandler, float[] amplitudes, int numPosSamples);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern uint OpenMPD_CWrapper_createColoursDescriptor(long primitiveManagerHandler, float[] colours, int numColSamples);

	//2. Defining primitives (content based on descriptors, updateable by setting its transformation matrix)
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern uint OpenMPD_CWrapper_declarePrimitive(long primitiveManagerHandler, uint posDescriptor, uint ampDescriptor, uint firstPosIndex = 0, uint firstAmpIndex = 0);

	//3. Runtime
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern bool OpenMPD_CWrapper_setPrimitiveEnabled(long primitiveManagerHandler, uint p, bool enabled);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_update_HighLevel(long primitiveManagerHandler, uint[] primitives, uint num_primitives, float[] mStarts, float[] mEnds);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_updatePrimitive_Positions(long primitiveManagerHandler, uint primitiveID, uint nextPosDescriptor, uint nextFirstPosIndex);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_updatePrimitive_Amplitudes(long primitiveManagerHandler, uint primitiveID, uint nextAmpDescriptor, uint nextFirstAmpIndex);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_updatePrimitive_Colours(long primitiveManagerHandler, uint primitiveID, uint nextColDescriptor, uint nextFirstColIndex);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern void OpenMPD_CWrapper_commitUpdates(long primitiveManagerHandler);

	//4. Releasing resources: 
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern bool OpenMPD_CWrapper_releasePositionsDescriptor(long primitiveManagerHandler, uint pd);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern bool OpenMPD_CWrapper_releaseAmplitudesDescriptor(long primitiveManagerHandler, uint ad);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern bool OpenMPD_CWrapper_releaseColoursDescriptor(long primitiveManagerHandler, uint cd);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern bool OpenMPD_CWrapper_releasePrimitive(long primitiveManagerHandler, uint p);
	[DllImport(".\\Assets\\Plugins\\OpenMPD.dll")]
	public static extern int OpenMPD_CWrapper_GetCurrentPosIndex();

}