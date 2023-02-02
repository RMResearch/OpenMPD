#ifndef _BEADDETECTOR_WRAPPER
#define _BEADDETECTOR_WRAPPER

#define BEAD_DETECTOR_EXPORT __declspec(dllexport)
#define BeadDetector_Instance_Handler long long

extern "C" {
	BEAD_DETECTOR_EXPORT bool BeadDetector_Initialize(); // return true
	BEAD_DETECTOR_EXPORT bool BeadDetector_Release(); //return true
	BEAD_DETECTOR_EXPORT void BeadDetector_RegisterPrintFuncs(void(*p_Message)(const char*), void(*p_Warning)(const char*), void(*p_Error)(const char*));
	
	// Base Functions - These were for testing the wrapper. 
	BEAD_DETECTOR_EXPORT BeadDetector_Instance_Handler BeadDetector_createInstance(int deviceID, float p1_World[3], float p2_World[3], float p3_World[3], float p4_World[3], int pixelsPerMeter = 5000, int threshold = 100, int erodeDilate = 2, float sphericity = 0.4f, float minRadiusInMeters = 0.001f,  float maxRadiusInMeters = 0.005f, bool visualize=false);
	BEAD_DETECTOR_EXPORT void BeadDetector_calibrateDetector(BeadDetector_Instance_Handler instance);
	BEAD_DETECTOR_EXPORT int BeadDetector_detectBeads(BeadDetector_Instance_Handler instance);
	BEAD_DETECTOR_EXPORT void BeadDetector_getCurrentBeadPositions(BeadDetector_Instance_Handler instance, float *pointArray);
	BEAD_DETECTOR_EXPORT void BeadDetector_destroyInstance(BeadDetector_Instance_Handler instance);
}
#endif