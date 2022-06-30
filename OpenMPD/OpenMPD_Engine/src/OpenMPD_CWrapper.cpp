#include <OpenMPD_CWrapper.h>
#include <OpenMPD.h>
#include <src/OpenMPD_Context.h>

//General methods for DLL wrapper:
 bool OpenMPD_CWrapper_Initialize() {
	return OpenMPD::Initialize();
}

 void OpenMPD_CWrapper_RegisterPrintFuncs(void(*p_Message)(const char*), void(*p_Warning)(const char*), void(*p_Error)(const char*)) {
	OpenMPD::RegisterPrintFuncs(p_Message, p_Warning, p_Error);
}
 
void OpenMPD_CWrapper_SetupEngine(size_t memorySizeInBytes, int gspat_version, OpenMPD_Pointer_Handler visualRenderer) {
	OpenMPD::SetupEngine(memorySizeInBytes, (OpenMPD::GSPAT_SOLVER)gspat_version, (OpenMPD::IVisualRenderer*)visualRenderer);
}

OpenMPD_Context_Handler  OpenMPD_CWrapper_StartEngine_TopBottom(cl_uchar FPS_Divider, cl_uint numParallelGeometries, cl_uint topBoardID, cl_uint bottomBoardID, bool forceSync) {
	OpenMPD::IPrimitiveUpdater* result = OpenMPD::StartEngine_TopBottom(FPS_Divider, numParallelGeometries, topBoardID, bottomBoardID, forceSync);
	return (OpenMPD_Context_Handler)result;
};

OpenMPD_Context_Handler  OpenMPD_CWrapper_StartEngine(cl_uchar FPS_Divider, cl_uint numParallelGeometries, cl_uint numBoards, cl_uint* boardIDs, float* boardLocations4x4, bool forceSync) {
	OpenMPD::IPrimitiveUpdater* result = OpenMPD::StartEngine(FPS_Divider, numParallelGeometries, numBoards, boardIDs, boardLocations4x4, forceSync);
	return (OpenMPD_Context_Handler)result;
};

 void OpenMPD_CWrapper_SetupFPS_Divider(unsigned char FPSDivider)
{
	return OpenMPD::setupFPS_Divider(FPSDivider);
}

 void OpenMPD_CWrapper_SetupPhaseOnly(bool phaseOnly)
{
	return OpenMPD::setupPhaseOnly(phaseOnly);
}

 void OpenMPD_CWrapper_SetupHardwareSync(bool useHardwareSync)
{
	return OpenMPD::setupHardwareSync(useHardwareSync);
}

 bool OpenMPD_CWrapper_StopEngine() {
	return OpenMPD::StopEngine();
}

 bool OpenMPD_CWrapper_Release() {
	return OpenMPD::Release();
}

 int OpenMPD_CWrapper_GetCurrentPosIndex() {
	return OpenMPD::GetCurrentPosIndex();
}
	
//Primitive Manager methods:
//1. Defining low-level descriptors
 cl_uint OpenMPD_CWrapper_createPositionsDescriptor(OpenMPD_Context_Handler pm, float* positions, int numPosSamples) {
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->createPositionsDescriptor(positions, numPosSamples);
}
 cl_uint OpenMPD_CWrapper_createAmplitudesDescriptor(OpenMPD_Context_Handler pm,float* amplitudes, int numAmpSamples){
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->createAmplitudesDescriptor(amplitudes, numAmpSamples);
}
 cl_uint OpenMPD_CWrapper_createColoursDescriptor(OpenMPD_Context_Handler pm,float* colours, int numColSamples){
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->createColoursDescriptor(colours, numColSamples);
}
//2. Defining primitives (based on descriptors)
 cl_uint OpenMPD_CWrapper_declarePrimitive(OpenMPD_Context_Handler pm,cl_uint posDescriptor, cl_uint ampDescriptor, cl_uint firstPosIndex, cl_uint firstAmpIndex) {
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->declarePrimitive(posDescriptor, ampDescriptor,firstPosIndex, firstAmpIndex);
}

//3. Runtime
 bool OpenMPD_CWrapper_setPrimitiveEnabled(OpenMPD_Context_Handler pm,cl_uint p, bool enabled){
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->setPrimitiveEnabled(p, enabled);
}
 void OpenMPD_CWrapper_update_HighLevel(OpenMPD_Context_Handler pm,cl_uint* primitives, cl_uint num_primitives, float* mStarts, float* mEnds/*,  GSPAT::MatrixAlignment ma*/){
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	manager->update_HighLevel(primitives, num_primitives, mStarts, mEnds);
}
 void OpenMPD_CWrapper_updatePrimitive_Positions(OpenMPD_Context_Handler pm,cl_uint primitiveID, cl_uint nextPosDescriptor, cl_uint nextFirstPosIndex){
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	manager->updatePrimitive_Positions(primitiveID, nextPosDescriptor,nextFirstPosIndex);
}
 void OpenMPD_CWrapper_updatePrimitive_Amplitudes(OpenMPD_Context_Handler pm,cl_uint primitiveID, cl_uint nextAmpDescriptor, cl_uint nextFirstAmpIndex){
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	manager->updatePrimitive_Amplitudes(primitiveID, nextAmpDescriptor, nextFirstAmpIndex);
}
 void OpenMPD_CWrapper_updatePrimitive_Colours(OpenMPD_Context_Handler pm,cl_uint primitiveID, cl_uint nextColDescriptor, cl_uint nextFirstColIndex){
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	manager->updatePrimitive_Colours(primitiveID, nextColDescriptor, nextFirstColIndex);
}
 void OpenMPD_CWrapper_commitUpdates(OpenMPD_Context_Handler pm){
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	manager->commitUpdates();
}

//4. Releasing resources: 
  bool OpenMPD_CWrapper_releasePositionsDescriptor(OpenMPD_Context_Handler pm,cl_uint pd) {
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->releasePositionsDescriptor(pd);
}
  bool OpenMPD_CWrapper_releaseAmplitudesDescriptor(OpenMPD_Context_Handler pm, cl_uint ad) {
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->releaseAmplitudesDescriptor(ad);
}
  bool OpenMPD_CWrapper_releaseColoursDescriptor(OpenMPD_Context_Handler pm, cl_uint cd) {
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->releaseColoursDescriptor(cd);
}
  bool OpenMPD_CWrapper_releasePrimitive(OpenMPD_Context_Handler pm,cl_uint p) {
	OpenMPD::IPrimitiveUpdater* manager = (OpenMPD::IPrimitiveUpdater*)pm;
	return manager->releasePrimitive(p);
}