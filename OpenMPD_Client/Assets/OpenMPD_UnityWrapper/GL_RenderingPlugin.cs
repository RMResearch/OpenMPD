using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;


public class GL_RenderingPlugin : MonoBehaviour
{
	[DllImport("GL_RenderingPlugin")]
	private static extern IntPtr GetRenderEventFunc();

	[DllImport("GL_RenderingPlugin")]
	private static extern void UnityApplicationStart();

	[DllImport("GL_RenderingPlugin")]
	private static extern void UnityPluginApplicationQuit();

	[DllImport("GL_RenderingPlugin")]
	public static extern bool VisualRenderedReady();

	[DllImport("GL_RenderingPlugin")]
	public static extern  long getOpenGLVisualRenderer();
	[DllImport("GL_RenderingPlugin")]
	public static extern void setPMatrix(float[]P);
	[DllImport("GL_RenderingPlugin")]
	public static extern void setVMatrix(float[]V);

	IEnumerator Start()
	{
		UnityApplicationStart();
		yield return StartCoroutine("CallPluginAtEndOfFrames");
	}
	private void Update()
	{
		if (OpenMPD_PresentationManager.Instance() == null)
			return;

		//0. Configure the camera parameters (this can be done here for static cameras)
		Matrix4x4 fromLevitatorToWorld = OpenMPD_PresentationManager.Instance().GetLevitatorNode().localToWorldMatrix;
		Matrix4x4 matP, matV;//Unity uses Row-major order
		matP = GL.GetGPUProjectionMatrix(Camera.allCameras[0].projectionMatrix, false);
		matV = GL.GetGPUProjectionMatrix(Camera.allCameras[0].worldToCameraMatrix* fromLevitatorToWorld, false);
		float[] P = new float[]{    matP[0],  matP[1], matP[2],  matP[3],
									matP[4],  matP[5], matP[6],  matP[7],
									matP[8],  matP[9], matP[10], matP[11],
									matP[12], matP[13], matP[14], matP[15]};
		float[] V = new float[]{    matV[0],  matV[1], matV[2],  matV[3],
									matV[4],  matV[5], matV[6],  matV[7],
									matV[8],  matV[9], matV[10], matV[11],
									matV[12], matV[13], matV[14], matV[15]};
		setPMatrix(P);
		setVMatrix(V);

	}
	private void OnApplicationQuit()
	{
		OpenMPD_Wrapper.PrintWarning("UseRenderingPlugin(Unity)::OnApplicationQuit()");
		UnityPluginApplicationQuit();
	}


	private IEnumerator CallPluginAtEndOfFrames()
	{
		while (true) {
			// Wait until all frame rendering is done
			yield return new WaitForEndOfFrame();

			// Issue a plugin event with arbitrary integer identifier.
			// The plugin can distinguish between different
			// things it needs to do based on this ID.
			// For our simple plugin, it does not matter which ID we pass here.
			GL.IssuePluginEvent(GetRenderEventFunc(), 1);
			
		}
	}
}
