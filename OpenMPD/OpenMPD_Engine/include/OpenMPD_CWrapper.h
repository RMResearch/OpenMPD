#ifndef _OpenMPD_ENGINE_CWRAPPER
#define _OpenMPD_ENGINE_CWRAPPER
#include <OpenMPD_Prerequisites.h>

#include <assert.h>
#include <math.h>
#include <vector>

extern "C" {
	static const int GSPAT_V2 = OpenMPD::V2;
	static const int GSPAT_V3 = OpenMPD::V3;
	static const int GSPAT_V4 = OpenMPD::V4;
	//General methods for DLL wrapper (i.e. general order of invokation):
	_OPEN_MPD_ENGINE_Export bool OpenMPD_CWrapper_Initialize();
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_RegisterPrintFuncs(void(*p_Message)(const char*), void(*p_Warning)(const char*), void(*p_Error)(const char*));
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_SetupEngine( size_t memorySizeInBytes, int gspat_version, OpenMPD_Pointer_Handler visualRenderer=0);
	_OPEN_MPD_ENGINE_Export OpenMPD_Context_Handler  OpenMPD_CWrapper_StartEngineSingleBoard(cl_uchar FPS_Divider, cl_uint numParallelGeometries, cl_uint boardID, float* matToWorld, bool forceSync = true);
	_OPEN_MPD_ENGINE_Export OpenMPD_Context_Handler  OpenMPD_CWrapper_StartEngine(cl_uchar FPS_Divider, cl_uint numParallelGeometries, cl_uint topBoardID, cl_uint bottomBoardID, bool forceSync=true);
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_SetupFPS_Divider(unsigned char FPSDivider);
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_SetupPhaseOnly(bool phaseOnly);
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_SetupHardwareSync(bool useHardwareSync);
	_OPEN_MPD_ENGINE_Export bool OpenMPD_CWrapper_StopEngine();
	_OPEN_MPD_ENGINE_Export bool OpenMPD_CWrapper_Release();
	_OPEN_MPD_ENGINE_Export int  OpenMPD_CWrapper_GetCurrentPosIndex();

	//Primitive Manager methods:
	//1. Defining low-level descriptors
	_OPEN_MPD_ENGINE_Export cl_uint OpenMPD_CWrapper_createPositionsDescriptor(OpenMPD_Context_Handler pm,float* positions, int numPosSamples);
	_OPEN_MPD_ENGINE_Export cl_uint OpenMPD_CWrapper_createAmplitudesDescriptor(OpenMPD_Context_Handler pm,float* amplitudes, int numAmpSamples);
	_OPEN_MPD_ENGINE_Export cl_uint OpenMPD_CWrapper_createColoursDescriptor(OpenMPD_Context_Handler pm,float* colours, int numColSamples);
	//2. Defining primitives (based on descriptors)
	_OPEN_MPD_ENGINE_Export cl_uint OpenMPD_CWrapper_declarePrimitive(OpenMPD_Context_Handler pm,cl_uint posDescriptor, cl_uint ampDescriptor, cl_uint firstPosIndex=0, cl_uint firstAmpIndex=0) ;

	//3. Runtime
	_OPEN_MPD_ENGINE_Export bool OpenMPD_CWrapper_setPrimitiveEnabled(OpenMPD_Context_Handler pm,cl_uint p, bool enabled) ;
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_update_HighLevel(OpenMPD_Context_Handler pm,cl_uint* primitives, cl_uint num_primitives, float* mStarts, float* mEnds/*,  GSPAT::MatrixAlignment ma*/) ;
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_updatePrimitive_Positions(OpenMPD_Context_Handler pm,cl_uint primitiveID, cl_uint nextPosDescriptor, cl_uint nextFirstPosIndex=0) ;
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_updatePrimitive_Amplitudes(OpenMPD_Context_Handler pm,cl_uint primitiveID, cl_uint nextAmpDescriptor, cl_uint nextFirstAmpIndex=0) ;
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_updatePrimitive_Colours(OpenMPD_Context_Handler pm,cl_uint primitiveID, cl_uint nextColDescriptor, cl_uint nextFirstColIndex=0) ;
	_OPEN_MPD_ENGINE_Export void OpenMPD_CWrapper_commitUpdates(OpenMPD_Context_Handler pm);
	//4. Releasing resources: 
	_OPEN_MPD_ENGINE_Export bool OpenMPD_CWrapper_releasePositionsDescriptor(OpenMPD_Context_Handler pm,cl_uint pd) ;
	_OPEN_MPD_ENGINE_Export bool OpenMPD_CWrapper_releaseAmplitudesDescriptor(OpenMPD_Context_Handler pm,cl_uint ad) ;
	_OPEN_MPD_ENGINE_Export bool OpenMPD_CWrapper_releaseColoursDescriptor(OpenMPD_Context_Handler pm,cl_uint cd) ;
	_OPEN_MPD_ENGINE_Export bool OpenMPD_CWrapper_releasePrimitive(OpenMPD_Context_Handler pm, cl_uint p) ;
};
#endif

