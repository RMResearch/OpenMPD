 #include <OpenMPD_Engine/include/OpenMPD_Prerequisites.h>
#include <OpenMPD_Engine/include/OpenMPD_CWrapper.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <vector>
#include <../Helper/TimeFunctions.h>
#include <sstream>
#include <Windows.h>
//#include "MicroTimer.h"
#define _USE_MATH_DEFINES

void print(const char* msg) { printf("%s\n", msg);}

unsigned char curFPS_Divider = 4;
cl_uint geometries = 32;
cl_uint numPrimitives=2	; 
cl_uint numBoards=16;
cl_uint boardIDs[] = { 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8};
float matBoardToWorld[256] = {/*bottom-left*/
								1, 0, 0, -0.08f,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top-left*/
								-1, 0, 0, -0.08f,
								0, 1, 0, 0,
								0, 0,-1, 0.2388f,
								0, 0, 0, 1,	
								/*bottom-right*/
								1, 0, 0, 0.08f,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top-right*/
								-1, 0, 0, 0.08f,
								0, 1, 0, 0,
								0, 0,-1, 0.2388f,
								0, 0, 0, 1,	
								/*bottom-left*/
								1, 0, 0, -0.16f,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top-left*/
								-1, 0, 0, -0.16f,
								0, 1, 0, 0,
								0, 0,-1, 0.2388f,
								0, 0, 0, 1,	
								/*bottom-right*/
								1, 0, 0, 0.16f,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top-right*/
								-1, 0, 0, 0.16f,
								0, 1, 0, 0,
								0, 0,-1, 0.2388f,
								0, 0, 0, 1,	
								/*bottom-left*/
								1, 0, 0, -0.08f,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top-left*/
								-1, 0, 0, -0.08f,
								0, 1, 0, 0,
								0, 0,-1, 0.2388f,
								0, 0, 0, 1,	
								/*bottom-right*/
								1, 0, 0, 0.08f,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top-right*/
								-1, 0, 0, 0.08f,
								0, 1, 0, 0,
								0, 0,-1, 0.2388f,
								0, 0, 0, 1,	
								/*bottom-left*/
								1, 0, 0, -0.16f,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top-left*/
								-1, 0, 0, -0.16f,
								0, 1, 0, 0,
								0, 0,-1, 0.2388f,
								0, 0, 0, 1,	
								/*bottom-right*/
								1, 0, 0, 0.16f,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top-right*/
								-1, 0, 0, 0.16f,
								0, 1, 0, 0,
								0, 0,-1, 0.2388f,
								0, 0, 0, 1,	
	};
bool foceSync = true;
bool phaseOnly = false;
bool HW_Sync = true;
void* client(void* arg);
int main() {

	do {
		do {
			OpenMPD_CWrapper_Initialize();
			OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print); 
			OpenMPD_CWrapper_SetupEngine(2000000, OpenMPD::GSPAT_SOLVER::V2);
			OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine(curFPS_Divider , geometries, numBoards, boardIDs, matBoardToWorld, foceSync);
			OpenMPD_CWrapper_SetupPhaseOnly(phaseOnly); 
			OpenMPD_CWrapper_SetupHardwareSync(HW_Sync);
			client((void*)pm);
			OpenMPD_CWrapper_StopEngine();
			printf("Press any key to Restart the engine.\nPress SPACE BAR to Release current engine.");
		} while (_getch() != ' ');
		OpenMPD_CWrapper_Release();
		printf("Engine Released (all structures destroyed).\nPress any key to Restart a new instance.\nPress SPACE BAR to finish program.");
	} while (_getch() != ' ');
}
//Client thread data
cl_uint* primitives;
cl_uint pos1;
cl_uint amp1;
float* matrices;

void declareContent(OpenMPD_Context_Handler pm);
void destroyContent(OpenMPD_Context_Handler pm);

void countFrames() {
	static bool first = true;
	static int numFramesSinceLastSecond;
	static struct timeval last, cur;
	static float lastSecond;
	if (first) {
		first = false;
		numFramesSinceLastSecond = 0;
		gettimeofday(&last, NULL);
	}
	numFramesSinceLastSecond++;
	gettimeofday(&cur, NULL);	
	float milisElapsed = computeTimeElapsedInMilis(last, cur);
	if (milisElapsed >= 1000) {
		printf("High-level FPS %f\n", 1000*numFramesSinceLastSecond/milisElapsed);
		last = cur;
		numFramesSinceLastSecond=0;
	}
}

void* client(void* arg) { 
	// counter
	static int framesCounter = 0;
	static struct timeval prevTime, curTime;
	static double totalTime = 0;
	static bool firstTime = true;
	static int reportPeriod = 1;

	OpenMPD_Context_Handler pm= (OpenMPD_Context_Handler)arg;
	declareContent(pm);
	bool running = true;	

	while (!_kbhit()) {
		bool commit = false;
		//LEFT_RIGHT
		//scanf("%c", &c);
		#pragma region 1 seconds report
		if (firstTime) {
  			gettimeofday(&prevTime, 0x0);
			firstTime = false;
		} 
		else{
			gettimeofday(&curTime, 0x0);
			totalTime += computeTimeElapsed(prevTime, curTime);
			//totalTime += computeTimeElapsed(prevTime, curTime);			
			prevTime = curTime;
		}
		
		if (totalTime <= reportPeriod) {
			framesCounter++;
		}
		else {
			static char logPerformance[512];
			sprintf_s(logPerformance, "Frames counter (last %f seconds): Actual UPS: %f, Target UPS: %f\n", totalTime, framesCounter, 10000);
			framesCounter = 0;
			totalTime = 0;
		}
		OpenMPD_CWrapper_update_HighLevel(pm,primitives, numPrimitives, matrices, matrices/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);						
		countFrames();	
				
	}
	destroyContent(pm);
	return NULL; 
}

void declareContent(OpenMPD_Context_Handler pm) {

	//A. Fixed position (used with "audio" amplitude descriptors)
	float p1_data[] = { 0, 0 , 0.12f, 1, };
	float a1_data[] = { 15000.0f, 15000.0f, 15000.0f, 15000.0f };
	
	pos1=OpenMPD_CWrapper_createPositionsDescriptor(pm, p1_data, 1);
	amp1=OpenMPD_CWrapper_createAmplitudesDescriptor(pm,a1_data, 4);
		
	//Create Primitives
	primitives = new cl_uint[numPrimitives];
	matrices=new float[16*numPrimitives];
	float matrix[] = { 1.0f,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
	for (int p = 0; p < numPrimitives; p++) {
		primitives[p] = OpenMPD_CWrapper_declarePrimitive(pm, pos1, amp1);
		OpenMPD_CWrapper_setPrimitiveEnabled(pm,primitives[p],true);
		memcpy(&(matrices[16 * p]), matrix, 16 * sizeof(float));
	}
	OpenMPD_CWrapper_commitUpdates(pm);	
}

void destroyContent(OpenMPD_Context_Handler pm){
	for (int p = 0; p < numPrimitives; p++) {
		OpenMPD_CWrapper_setPrimitiveEnabled(pm, primitives[p], false);
	}
	OpenMPD_CWrapper_commitUpdates(pm);
	//Destroy primitives:
	for (int p = 0; p < numPrimitives; p++) {
		OpenMPD_CWrapper_releasePrimitive(pm, primitives[p]);
	}
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pos1);
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amp1);
	OpenMPD_CWrapper_commitUpdates(pm);
	//If you are using "force sync", we need to give the engine a chance to apply all this
	// Calling update_HighLevel allows engine to react to changes 
	OpenMPD_CWrapper_update_HighLevel(pm, NULL, 0, NULL, NULL/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);
	Sleep(100);
}
