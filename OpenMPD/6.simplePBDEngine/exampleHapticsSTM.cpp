/**
This example provides a simple test to generate different tactile patterns.
We will use multi-point spatio-temporal modulation, which we implement with several primitives (one per point),
each of them drawing a circle shape of 3cm (as a position descriptor) and using maximum amplitudes (phase Only = true).
The use will be able to select a number of points and base frequency, and the demo will setup the engine and rendeer the haptics.
The user can stop the rendering (SPACE), select new parameters (points, frequency) and rendering will re-start.
*/

#include <OpenMPD_Prerequisites.h>
#include <OpenMPD_CWrapper.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <vector>
#include <../Helper/TimeFunctions.h>
#include <sstream>
#include <Windows.h>
//#include "MicroTimer.h"
#define _USE_MATH_DEFINES

void print(const char* msg){ printf("%s\n", msg); }
void* client(void* arg, int curNumPoints, int frequency);
float* createSampledArc(float origin[3], float p0[3], float angleInRads, cl_uint numSamples);
int curFPS_Divider = 4;
int curFreq = 20;
int curNumPoints = 1;

int main() {

		OpenMPD_CWrapper_Initialize();
		//OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
		OpenMPD_CWrapper_SetupEngine(2000000, OpenMPD::GSPAT_SOLVER::V2);
		float matToWorld[] = { 1,0,0,0,  0,1,0,0,   0,0,1,0,   0,0,0,1 };
		OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine_SingleBoard(curFPS_Divider, 32, 6, matToWorld, false);
		OpenMPD_CWrapper_SetupPhaseOnly(true);
		do {
			
			printf("Press any key to change haptic parameters.\nPress SPACE BAR to Finish the application.\n");
			client((void*)pm, curNumPoints, curFreq);
			printf("Input number of points to use [1-32] (Current: %d)", curNumPoints);
			scanf("%d", &curNumPoints);
			printf("Input frequency [50-250] (Current: %d)", curFreq);
			scanf("%d", &curFreq);
			
			
		} while (_getch() != ' ');
		OpenMPD_CWrapper_StopEngine();
		OpenMPD_CWrapper_Release();
		
}
void declareContent(OpenMPD_Context_Handler pm, int curNumPoints, int frequency);
void destroyContent(OpenMPD_Context_Handler pm, int curNumPoints);

void* client(void* arg, int curNumPoints, int frequency) { 
	OpenMPD_Context_Handler pm= (OpenMPD_Context_Handler)arg;
	declareContent(pm, curNumPoints, frequency);
	while (! _kbhit()) {						
		Sleep(1000);
	}
	destroyContent(pm, curNumPoints);
	return NULL; 
}

cl_uint* primitives;
cl_uint posDescriptor;
cl_uint ampDescriptor;

void declareContent(OpenMPD_Context_Handler pm, int curNumPoints, int frequency) {
	float origin[] = { 0,0,0.12f }, startPoint1[] = { 0.025f,0,0.12f };
	int numSamplesCircle = (int) 40000 / (curFPS_Divider * frequency);
	float* circle_data = createSampledArc(origin, startPoint1, (float)(2 * CL_M_PI), numSamplesCircle);
	float amp_data = 1;
	
	//Create descriptors
	posDescriptor=OpenMPD_CWrapper_createPositionsDescriptor(pm, circle_data, numSamplesCircle);
	ampDescriptor=OpenMPD_CWrapper_createAmplitudesDescriptor(pm,&amp_data, 1);
	//Create Primitives
	primitives = new cl_uint[curNumPoints];
	float* primitiveMatrices = new float[16 * curNumPoints];
	for (int p = 0; p < curNumPoints; p++) {
		primitives[p] = OpenMPD_CWrapper_declarePrimitive(pm, posDescriptor, ampDescriptor);
		OpenMPD_CWrapper_setPrimitiveEnabled(pm, primitives[p], true);
		//ALL PRIMITIVES ARE EQUAL, BUT THEY ARE ROTATED ALONG THE VERTICAL AXIS (i.e., they start from a different angle in the circle). 
		float rotation = (float) (2 * p*CL_M_PI / curNumPoints);
		float mat[] = { cosf(rotation), sinf(rotation), 0, 0,
						-sinf(rotation), cosf(rotation), 0, 0,
						0,0,1,0,
						0,0,0,1 };
		memcpy(&(primitiveMatrices[16 * p]), mat, 16 * sizeof(float));
	}
	
	OpenMPD_CWrapper_commitUpdates(pm);
	OpenMPD_CWrapper_update_HighLevel(pm, primitives, curNumPoints, primitiveMatrices, primitiveMatrices/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);
}

void destroyContent(OpenMPD_Context_Handler pm, int curNumPoints){
	//Disable all primitives.
	for (int p = 0; p < curNumPoints; p++) {
		OpenMPD_CWrapper_setPrimitiveEnabled(pm, primitives[p], false);
	}
	OpenMPD_CWrapper_commitUpdates(pm);
	//Destroy all primitives:
	for (int p = 0; p < curNumPoints; p++)
		OpenMPD_CWrapper_releasePrimitive(pm, primitives[p]);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, posDescriptor);
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, ampDescriptor);	
}

float* createSampledArc(float origin[3], float p0[3], float angleInRads, cl_uint numSamples) {
	//static float buffer[4 * 8];
	float* buffer = new float[numSamples*4];
	float radius[] = { p0[0] - origin[0], p0[1] - origin[1], p0[2] - origin[2]};
	float curP[4];
	//Fill in all the samples:
	for (int s = 0; s < numSamples; s++) {
		float curAngle = (s*angleInRads) / numSamples;
		curP[0] = cosf(curAngle)*radius[0] - sinf(curAngle)*radius[1] + origin[0];
		curP[1] = sinf(curAngle)*radius[0] + cosf(curAngle)*radius[1] + origin[1];
		curP[2] = origin[2];
		curP[3] = 1;
		memcpy(&(buffer[4 * s]), curP, 4 * sizeof(float));
	}
	return buffer;
}
