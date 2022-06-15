// #include <OpenMPD_Engine/include/OpenMPD_Prerequisites.h>
//#include <OpenMPD_Engine/include/OpenMPD_CWrapper.h>
#include <OpenMPD/OpenMPD_Prerequisites.h>
#include <OpenMPD/OpenMPD_CWrapper.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <vector>
#include <../Helper/TimeFunctions.h>
#include <sstream>
#include <Windows.h>
//#include "MicroTimer.h"
#define _USE_MATH_DEFINES

void print(const char* msg) { printf("%s\n", msg); }
void* client(void* arg);
float* createSampledArc(float origin[3], float p0[3], float angleInRads, int numSamples);

unsigned char curFPS_Divider = 4;
cl_uint geometries = 32;
//cl_uint topBoard = 40;
//cl_uint bottomBoard = 41;
bool foceSync = true;
bool phaseOnly = true;
bool HW_Sync = true;
int numPrimitives = 1;
int numOfBoards = 2;

#define activeBoards 4
#if activeBoards == 2
int boardIDs[] = { 57, 56 };
// 2  4  6  8  10  12
//32 64 96 128 160 192
float matBoardToWorld[32] = {
	/*2 bottom --------*/
	1, 0, 0, 0,  //0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*2 top*/
	-1, 0, 0, 0, //0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
};

#elif activeBoards == 4
#// bottom, top
int boardIDs[] = { 56, 57, 49, 48 };
// 2  4  6  8  10  12
//32 64 96 128 160 192
float matBoardToWorld[64] = {
	/*2 bottom --------*/
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*2 top*/
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*4 bottom --------*/
	1, 0, 0, -0.16f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*4 top*/
	-1, 0, 0, -0.16f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
};

#elif activeBoards == 6	
int boardIDs[] = { 1, 2, 3, 4, 5, 6 };
// 2  4  6  8  10  12
//32 64 96 128 160 192
float matBoardToWorld[96] = {
	/*2 bottom --------*/
	1, 0, 0, -0.24f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*2 top*/
	-1, 0, 0, -0.24f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*4 bottom --------*/
	1, 0, 0, -0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*4 top*/
	-1, 0, 0, -0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*6 bottom --------*/
	1, 0, 0, 0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*6 top*/
	-1, 0, 0, 0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
};

#elif activeBoards == 8
int boardIDs[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
// 2  4  6  8  10  12
//32 64 96 128 160 192
float matBoardToWorld[128] = {
	/*2 bottom --------*/
	1, 0, 0, -0.24f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*2 top*/
	-1, 0, 0, -0.24f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*4 bottom --------*/
	1, 0, 0, -0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*4 top*/
	-1, 0, 0, -0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*6 bottom --------*/
	1, 0, 0, 0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*6 top*/
	-1, 0, 0, 0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*8 bottom --------*/
	1, 0, 0, 0.24f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*8 top*/
	-1, 0, 0, 0.24f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
};

#elif activeBoards == 10
int boardIDs[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
// 2  4  6  8  10  12
//32 64 96 128 160 192
float matBoardToWorld[160] = {
	/*2 bottom --------*/
	1, 0, 0, -0.32f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*2 top*/
	-1, 0, 0, -0.32f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*4 bottom --------*/
	1, 0, 0, -0.24f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*4 top*/
	-1, 0, 0, -0.24f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*6 bottom --------*/
	1, 0, 0, -0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*6 top*/
	-1, 0, 0, -0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*8 bottom --------*/
	1, 0, 0, 0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*8 top*/
	-1, 0, 0, 0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*10 bottom --------*/
	1, 0, 0, 0.24f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*10 top*/
	-1, 0, 0, 0.24f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
};

#elif activeBoards == 12
int boardIDs[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
// 2  4  6  8  10  12
//32 64 96 128 160 192
float matBoardToWorld[192] = {
	/*2 bottom --------*/
	1, 0, 0, -0.32f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*2 top*/
	-1, 0, 0, -0.32f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*4 bottom --------*/
	1, 0, 0, -0.24f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*4 top*/
	-1, 0, 0, -0.24f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*6 bottom --------*/
	1, 0, 0, -0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*6 top*/
	-1, 0, 0, -0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*8 bottom --------*/
	1, 0, 0, 0.08f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*8 top*/
	-1, 0, 0, 0.08f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*10 bottom --------*/
	1, 0, 0, 0.24f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*10 top*/
	-1, 0, 0, 0.24f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
	/*12 bottom --------*/
	1, 0, 0, 0.32f,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	/*12 top*/
	-1, 0, 0, 0.32f,
	0, 1, 0, 0,
	0, 0,-1, 0.2388f,
	0, 0, 0, 1,
};
#endif

int main() {
	numOfBoards = (int)activeBoards;
	do {
		do {
			OpenMPD_CWrapper_Initialize();
			OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
			OpenMPD_CWrapper_SetupEngine(4000000, OpenMPD::GSPAT_SOLVER::V2);
			OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngineMultiSetup(curFPS_Divider, geometries, numOfBoards, boardIDs, matBoardToWorld, foceSync);
			//OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine(curFPS_Divider , geometries, topBoard, bottomBoard, foceSync);
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
//cl_uint pri1, pri2;
cl_uint circlePath;
//cl_uint pos1, pos2, circle1, circle2;
//cl_uint amp1, amp2;
const int numSamplesCircle = 10000;// 5 revolutions per second, if we are running at 10KHz (divider 4).
const int MAX_PRIMITIVES = 16;
cl_uint primitives[MAX_PRIMITIVES];
cl_uint positions[MAX_PRIMITIVES];
cl_uint amplitudes[MAX_PRIMITIVES];

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
		printf("High-level FPS %f\n", 1000 * numFramesSinceLastSecond / milisElapsed);
		last = cur;
		numFramesSinceLastSecond = 0;
	}
}

void* client(void* arg) {
	// counter
	static int framesCounter = 0;
	static struct timeval prevTime, curTime;
	static double totalTime = 0;
	static bool firstTime = true;
	static int reportPeriod = 1;

	OpenMPD_Context_Handler pm = (OpenMPD_Context_Handler)arg;
	declareContent(pm);
	printf("Contents created. Press a key to finish.\n");

	bool running = true;
	static const size_t X_index = 3, Y_index = 7, Z_index = 11;

	float* prevMat = new float[MAX_PRIMITIVES * 16];
	float* curMat = new float[MAX_PRIMITIVES * 16];
	//Each primitive's matrix is rotated around Z axis 
	for (int p = 0; p < numPrimitives; p++) {
		float matrix[] = { 1, 0, 0, 0,
						   0, 1, 0, 0,
						   0, 0, 1, 0,
						   0, 0, 0, 1 };
		memcpy(&(curMat[16 * p]), matrix, 16 * sizeof(float));
		memcpy(&(prevMat[16 * p]), matrix, 16 * sizeof(float));
	}

	while (running) {
		bool commit = false;
		//LEFT_RIGHT
		//scanf("%c", &c);
#pragma region 1 seconds report
		if (firstTime) {
			gettimeofday(&prevTime, 0x0);
			firstTime = false;
		}
		else {
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

		/*DWORD curTime = MicroTimer::uGetTime();
		static int count = 0;
		count++;
		static DWORD prevTime = curTime;
		if (curTime - prevTime >= 1000000.f) {
			printf("Count = %d\n", count);
			prevTime = curTime;
			count = 0;
		}*/
#pragma endregion
		static bool circle = false;
		static bool sound = false;
		if (_kbhit())
			switch (_getch()) {
				//UPDATE DIVIDER:
			case 'e':
				curFPS_Divider++;
				OpenMPD_CWrapper_SetupFPS_Divider(curFPS_Divider);
				break;
			case 'q':
				curFPS_Divider--;
				OpenMPD_CWrapper_SetupFPS_Divider(curFPS_Divider);
				break;
				//MOVE
			case 'a':
				commit = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + X_index] = curMat[16 * p + X_index];
					curMat[16 * p + X_index] += 0.001f;
				}
				break;

			case 'd':
				commit = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + X_index] = curMat[16 * p + X_index];
					curMat[16 * p + X_index] -= 0.001f;
				}
				break;
			case 'w':
				commit = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + Y_index] = curMat[16 * p + Y_index];
					curMat[16 * p + Y_index] += 0.001f;
				}
				break;
			case 's':
				commit = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + Y_index] = curMat[16 * p + Y_index];
					curMat[16 * p + Y_index] -= 0.001f;
				}
				break;
			case 'r':
				commit = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + Z_index] = curMat[16 * p + Z_index];
					curMat[16 * p + Z_index] += 0.0005f;
				}
				break;
			case 'f':
				commit = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + Z_index] = curMat[16 * p + Z_index];
					curMat[16 * p + Z_index] -= 0.0005f;
				}
				break;
			case ' ':
				printf("SPACE BAR pressed");
				running = false;
				break;
				//CHANGE STATE
			case '1':
				circle = !circle;
				if (circle) {
					printf("\n     Start rotation! ---------------> \n");
					OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
					for (int p = 0; p < numPrimitives; p++) {
						if (p == 0)
							OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitives[p], circlePath, 0);
						else
							OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitives[p], circlePath, numSamplesCircle / numPrimitives);
					}
				}
				else {
					printf("\n     Stop rotation! ---------------->\n");
					OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
					for (int p = 0; p < numPrimitives; p++) {
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitives[p], positions[p], 0);
					}
				}
				OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
				OpenMPD_CWrapper_commitUpdates(pm);
				break;				
			}
		//Update engine:
		if (commit)
			printf("(%f, %f, %f)\n", curMat[X_index], curMat[Y_index], curMat[Z_index]);
		float mStarts[32], mEnds[32];
		memcpy(&(mStarts[0]), prevMat, 16 * sizeof(float));
		memcpy(&(mStarts[16]), prevMat, 16 * sizeof(float));
		memcpy(&(mEnds[0]), curMat, 16 * sizeof(float));
		memcpy(&(mEnds[16]), curMat, 16 * sizeof(float));
		OpenMPD_CWrapper_update_HighLevel(pm, primitives, numPrimitives, mStarts, mEnds/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);
		countFrames();
	}
	destroyContent(pm);
	return NULL;
}

void declareContent(OpenMPD_Context_Handler pm) {
	// generate amplitud descriptors
	float maxAmplitudeInPascal = 20000;	// the maximum amplitude in pascal
	//POSITION DESCRIPTORS:
	float radius = 0.03f;
	float height = 0.12f; // 0.12f

	float origin[] = { -0.08f, 0, 0.12f };
	float p1_data[] = { radius , 0 , 0.12f, 1, };


	for (int p = 0; p < numPrimitives; p++) {
		float p_data[] = { radius + ((float)p * radius), 0 , height , 1 };		// centre of the system
		//memcpy(p1_data, &p_data, sizeof(float)*4);
		float a_data[] = { maxAmplitudeInPascal };
		//Create descriptors
		positions[p] = OpenMPD_CWrapper_createPositionsDescriptor(pm, p_data, 1);
		amplitudes[p] = OpenMPD_CWrapper_createAmplitudesDescriptor(pm, a_data, 1);
	}

	float* circle_data1 = createSampledArc(origin, p1_data, 2 * 3.14159265f, numSamplesCircle);
	circlePath = OpenMPD_CWrapper_createPositionsDescriptor(pm, circle_data1, numSamplesCircle);

	//Create Primitives
	for (int p = 0; p < numPrimitives; p++) {
		primitives[p] = OpenMPD_CWrapper_declarePrimitive(pm, positions[p], amplitudes[p]);
		OpenMPD_CWrapper_commitUpdates(pm);
		OpenMPD_CWrapper_setPrimitiveEnabled(pm, primitives[p], true);
		OpenMPD_CWrapper_commitUpdates(pm);
	}

}

void destroyContent(OpenMPD_Context_Handler pm) {
	//Desable primitives:
	for (int p = 0; p < numPrimitives; p++) {
		OpenMPD_CWrapper_setPrimitiveEnabled(pm, primitives[p], false);
	}
	//Destroy primitives:
	for (int p = 0; p < numPrimitives; p++) {
		OpenMPD_CWrapper_releasePrimitive(pm, primitives[p]);
		OpenMPD_CWrapper_releasePositionsDescriptor(pm, positions[p]);
		OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes[p]);
	}
	OpenMPD_CWrapper_commitUpdates(pm);
	//If you are using "force sync", we need to give the engine a chance to apply all this
	// Calling update_HighLevel allows engine to react to changes 
	OpenMPD_CWrapper_update_HighLevel(pm, NULL, 0, NULL, NULL/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);
	Sleep(100);
}

float* createSampledArc(float origin[3], float p0[3], float angleInRads, int numSamples) {
	//static float buffer[4 * 8];
	float aux[numSamplesCircle * 4];
	float* buffer = new float[numSamples * 4];
	float radius[] = { p0[0] - origin[0], p0[1] - origin[1], p0[2] - origin[2] };
	float curP[4];
	//Fill in all the samples:
	for (int s = 0; s < numSamples; s++) {
		float curAngle = (s * angleInRads) / numSamples;
		curP[0] = cosf(curAngle) * radius[0] - sinf(curAngle) * radius[1] + origin[0];
		curP[1] = sinf(curAngle) * radius[0] + cosf(curAngle) * radius[1] + origin[1];
		curP[2] = 0.12f + origin[2];
		curP[3] = 1;
		memcpy(&(buffer[4 * s]), curP, 4 * sizeof(float));
		memcpy(&(aux[4 * s]), curP, 4 * sizeof(float));
	}
	return buffer;
}









