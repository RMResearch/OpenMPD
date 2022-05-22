// Example low level rendering Unity plugin

#include "GL_VisualRenderer.h"
#include <windows.h>
//#include <CL/opencl.h>
#include <assert.h>
#include <math.h>
#include <vector>
#	include "gl3w/gl3w.h"
#include <GL_Renderer\GL_Renderer.h>
// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.

static float g_Time;
static bool _createVisualRenderer = false;
#include <stdio.h>

void _print(char* msg) {
	/*FILE* f;
	while(fopen_s(&f,"logRender.txt","a")!=0)
		Sleep(1);
	fprintf_s(f,"%s\n", msg);
	fclose(f);*/
}




// --------------------------------------------------------------------------
// UnitySetInterfaces
static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;
static RenderAPI* s_CurrentAPI = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;
// ---------------------------------------------------------------------------
//OpenGL resources for interoperation:
HDC glDeviceContext;
HGLRC glContext;
HGLRC glContextData;
GL_Renderer* renderer = NULL;
float P[16] = {1.0f,	0,	0,	0,
				0,		1,	0,	0,
				0,		0,	1,	0, 
				0,		0,	0,	1};

float V[16] = {1.0f,	0,	0,	0,
				0,		1,	0,	0,
				0,		0,	1,	0, 
				0,		0,	0,	1};;

static void OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	// Create graphics API implementation upon initialization
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		_print("OnGraphicsDeviceEvent:   eventType == kUnityGfxDeviceEventInitialize");
		assert(s_CurrentAPI == NULL);
		s_DeviceType = s_Graphics->GetRenderer();
		s_CurrentAPI = CreateRenderAPI(s_DeviceType);
	}

	// Let the implementation process the device related events
	if (s_CurrentAPI)
	{
		_print("OnGraphicsDeviceEvent:  Other ");
		s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
	}

	// Cleanup graphics API implementation upon shutdown
	if (eventType == kUnityGfxDeviceEventShutdown)
	{
		_print("OnGraphicsDeviceEvent:   eventType == kUnityGfxDeviceEventShutdown");
		delete s_CurrentAPI;
		s_CurrentAPI = NULL;
		s_DeviceType = kUnityGfxRendererNull;
	}
}

void UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	_print("Plugin load");
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
	// Run OnGraphicsDeviceEvent(initialize) manually on plugin load
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

void UnityPluginApplicationQuit() {
	_print("Application quitting.");
	if (renderer) {
		//delete renderer;
		renderer = NULL;
	}
}

void UnityPluginUnload()
{
	_print("Plugin unload.");
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
	if (renderer) {
		//delete renderer;
		renderer = NULL;
	}
}

void UnityApplicationStart()
{
	_print("Application starting.");
	if (renderer) {
		delete renderer;
		renderer = NULL;
	}		
}

void CreateVisualRenderer(int renderWindowSizeInSamples, int numSamplesOffset) {
		_print("RenderingPlugin:  Creating shared context (OpenGL-OpenCL) from renderer's thread (GL context current)!\n");
		glDeviceContext=wglGetCurrentDC();
		glContext=wglGetCurrentContext();
		glContextData = wglCreateContext(glDeviceContext);
		wglShareLists(glContext, glContextData);
		renderer = new GL_Renderer(glDeviceContext, glContextData, renderWindowSizeInSamples, numSamplesOffset, _print);		
}

// --------------------------------------------------------------------------
// OnRenderEvent
// This will be called for GL.IssuePluginEvent script calls; eventID will
// be the integer passed to IssuePluginEvent. In this example, we just ignore
// that value.
static void OnRenderEvent(int eventID)
{	char consoleLine[128];
	sprintf_s(consoleLine, "Received Event: %d", eventID);	_print(consoleLine);

	if (eventID == 1) {

		// Unknown / unsupported graphics device type? Do nothing
		if (s_CurrentAPI == NULL)
			return;

		if (!s_CurrentAPI->contextInitialized()) {
			_print("Context not intialized");
			return;
		}
	}
	
	if (renderer == NULL) 
		CreateVisualRenderer(330);		
	else 
		renderer->render(P, V);	
}


bool VisualRenderedReady() {
	return (renderer !=NULL);
}
long long getOpenGLVisualRenderer() { return (long long)renderer; }

void UNITY_INTERFACE_EXPORT setPMatrix(float * _P)
{
	memcpy(P, _P, 16 * sizeof(float));
}

void UNITY_INTERFACE_EXPORT setVMatrix(float * _V)
{
	memcpy(V, _V, 16 * sizeof(float));
}

void UNITY_INTERFACE_EXPORT setPointSize(int pointSize)
{
	if (renderer)
		renderer->setPointSize(pointSize);
}

void UNITY_INTERFACE_EXPORT setRenderOffsetInSamples(int offsetInSamples)
{
	if (renderer)
		renderer->setRenderOffsetInSamples(offsetInSamples);
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.

UnityRenderingEvent GetRenderEventFunc()
{
	return OnRenderEvent;
}
