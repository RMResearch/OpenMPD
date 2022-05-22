#include "GenerateTestPaths.h"
#include <stdio.h>

float A[] = { 0, 0, 0.16f,1 }, B[] = { 0.0f, 0, 0.06f, 1 };//0.2388f/2
float a0[] = { 10,20,40,60,80,100,120,140,160,180,200,250,300,350,400,450,500, 550,600,650,700,750,800,850,900,950,1000,1050,1100,1150,1200,1250,1300,1350,1400,1450,1500 };		//Acceleration to test in m/s2 (assumed v0=0 for these tests).
int numTests=37;
int curTargetFPS = 40000;
float v0 = 0.05f;


void main(void) {
	char filename[100];
	for(int test=0;test<numTests;test++){
		float* testPath_data;
		size_t sizeTestPath		= createLinearTest(A, B, 0, a0[test], 1.0f/curTargetFPS, &testPath_data);
		sprintf(filename, "verticalTestA=%d-FPS=%d.csv", (int)a0[test],curTargetFPS);
		FILE* f;
		fopen_s(&f, filename, "w");
		for (int s = 0; s < sizeTestPath; s++)
			fprintf(f, "%.8f,%.8f,%.8f,%.8f\n", testPath_data[4 * s + 0], testPath_data[4 * s + 1], testPath_data[4 * s + 2], testPath_data[4 * s + 3]);
		delete testPath_data;
		fclose(f);
	}
}