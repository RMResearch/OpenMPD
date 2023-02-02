#include "BeadDetector\BD_Wrapper.h"
#include <conio.h> //for _kbhit() 
#include <stdio.h> //for printf

void print(const char* msg) {
	printf("%s\n", msg);
}

void main(void) {
	BeadDetector_Initialize();
	BeadDetector_RegisterPrintFuncs(print, print, print);
	float p1[] = {-0.084f, -0.084f, 0.03f},
		  p2[] = { 0.084f, -0.084f, 0.03f},
		  p3[] = { 0.084f,  0.084f, 0.03f},
		  p4[] = {-0.084f,  0.084f, 0.03f};
	BeadDetector_Instance_Handler bd = BeadDetector_createInstance(1, p1, p2, p3, p4,5000,100,1,0.4f,0.001f, 0.005f, true);
	BeadDetector_calibrateDetector(bd);
	bool end = false;
	while (!_kbhit()) {
		int numParticlesDetected = BeadDetector_detectBeads(bd);
		if (numParticlesDetected == 0)
			continue;
		float* beadPosition=new float [3*numParticlesDetected];
		BeadDetector_getCurrentBeadPositions(bd, beadPosition);
		printf("Detected [%f, %f, %f]\n", beadPosition[0], beadPosition[1], beadPosition[2]);
		delete beadPosition;
	}
	BeadDetector_destroyInstance(bd);
	BeadDetector_Release();
}