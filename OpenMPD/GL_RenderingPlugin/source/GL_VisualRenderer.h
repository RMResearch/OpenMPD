#pragma once
/**
Example low level rendering using OpenGL.
Additional bindings to also operate as a Unity plugin (https://docs.unity3d.com/Manual/NativePluginInterface.html)
For usage outside Unity (e.g. custom Opengl engine), the callback method returned by GetRenderEventFunc must be invoked with eventID=0
*/
#include "RenderAPI/PlatformBase.h"
#include "RenderAPI/RenderAPI.h"


extern "C" {
	// --------------------------------------------------------------------------
	// UnitySetInterfaces
	void UNITY_INTERFACE_EXPORT UnityPluginLoad(IUnityInterfaces* unityInterfaces);
	void UNITY_INTERFACE_EXPORT UnityPluginUnload();
	void UNITY_INTERFACE_EXPORT UnityApplicationStart();
	void UNITY_INTERFACE_EXPORT UnityPluginApplicationQuit();
	// --------------------------------------------------------------------------
	// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.
	void UNITY_INTERFACE_EXPORT CreateVisualRenderer(int renderWindowSizeInSamples=320, int numSamplesOffset=0);
	UnityRenderingEvent UNITY_INTERFACE_EXPORT GetRenderEventFunc();
	bool UNITY_INTERFACE_EXPORT VisualRenderedReady();
	long long UNITY_INTERFACE_EXPORT getOpenGLVisualRenderer();
	void UNITY_INTERFACE_EXPORT setPMatrix(float* P);
	void UNITY_INTERFACE_EXPORT setVMatrix(float* V);
	void UNITY_INTERFACE_EXPORT setPointSize(int pointSize);
	void UNITY_INTERFACE_EXPORT setRenderOffsetInSamples(int offsetInSamples);

};

