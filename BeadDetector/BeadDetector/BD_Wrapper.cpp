#include "BD_Wrapper.h"
#include "BeadDetector.h"

static Detector* BDInstance = NULL;

bool BeadDetector_Initialize()
{
	return true;
}

bool BeadDetector_Release()
{
	if (BDInstance)
		delete BDInstance;

	return true;
}

void BeadDetector_RegisterPrintFuncs(void(*p_Message)(const char*), void(*p_Warning)(const char*), void(*p_Error)(const char*))
{
	Detector::RegisterPrintFuncs(p_Message, p_Warning, p_Error);
}

BEAD_DETECTOR_EXPORT BeadDetector_Instance_Handler BeadDetector_createInstance(int webcamID, float p1_World[3], float p2_World[3], float p3_World[3], float p4_World[3], int pixelsPerMeter, int threshold, int erodeDilate, float sphericity, float minRadiusInMeters, float maxRadiusInMeters, bool visualize)
{
	if(BDInstance == NULL)
		BDInstance = new Detector();
	BDInstance->initInstance(webcamID, p1_World, p2_World, p3_World, p4_World, pixelsPerMeter, threshold, erodeDilate, sphericity, minRadiusInMeters, maxRadiusInMeters, visualize);
	return (BeadDetector_Instance_Handler)BDInstance;
}

BEAD_DETECTOR_EXPORT void BeadDetector_calibrateDetector(BeadDetector_Instance_Handler instance) {
	((Detector*)instance)->calibrateDetector();
}

BEAD_DETECTOR_EXPORT int  BeadDetector_detectBeads(BeadDetector_Instance_Handler instance) {
	return ((Detector*)instance)->detectBeads();
}

BEAD_DETECTOR_EXPORT void BeadDetector_getCurrentBeadPositions(BeadDetector_Instance_Handler instance, float *pointArray) {
	((Detector*)instance)->getCurrentBeadPositions(pointArray);
}

BEAD_DETECTOR_EXPORT void BeadDetector_destroyInstance(BeadDetector_Instance_Handler instance) {
	Detector* target = (Detector*)instance;
	if (target == BDInstance) {
		delete BDInstance;
		BDInstance = NULL;
	}
}