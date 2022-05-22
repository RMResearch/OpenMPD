#include <OpenMPD_Prerequisites.h>
#include <OpenMPD_CWrapper.h>
#include <stdio.h>
#include <conio.h>
#include <GenerateTestPaths.h>

void print(const char* msg){ printf("%s\n", msg); }
void* client(void* arg);

unsigned char curFPS_Divider = 4;//10KHz 
int main() {

	do {
		do {
			OpenMPD_CWrapper_Initialize();
			OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
			OpenMPD_CWrapper_SetupEngine(2000000);
			OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine(curFPS_Divider, 32, 35, 32, true);
			OpenMPD_CWrapper_SetupPhaseOnly(true);
			client((void*)pm);
			OpenMPD_CWrapper_StopEngine();
			printf("Press any key to Restart the engine.\nPress SPACE BAR to Release current engine.");
		} while (_getch() != ' ');
		OpenMPD_CWrapper_Release();
		printf("Engine Released (all structures destroyed).\nPress any key to Restart a new instance.\nPress SPACE BAR to finish program.");
	} while (_getch() != ' ');
}
//Client thread data
float A1[] = { -0.05f, -0.03f, 0.1f,1 }, B1[] = { 0.05f, -0.03f, 0.1f, 1 };//0.2388f/2
float A2[] = { 0.05f, 0.03f, 0.1f,1 }, B2[] = { -0.05f, 0.03f, 0.1f, 1 };//0.2388f/2

float a0 = 10;		//Acceleration to test in m/s2 (assumed v0=0 for these tests).
float v0 = 0.05f;	//Return speed (particle returns to origin at this speed, no acceleration).
cl_uint primitive1, primitive2;
cl_uint posStart[2], posEnd[2], pathTest[2], pathReturn[2];
cl_uint amplitudes[2];

void declareContent(OpenMPD_Context_Handler pm);
void destroyContent(OpenMPD_Context_Handler pm);

void* client(void* arg) { 
	OpenMPD_Context_Handler pm= (OpenMPD_Context_Handler)arg;
	declareContent(pm);
	printf("Contents created. Press a key to finish.\n");
	bool running = true;
	cl_uint primitives[] = { primitive1, primitive2 };//Declared as global shared variables
	static const size_t X_index = 3, Y_index = 7, Z_index = 11;
	float matrices[]={1,0,0,0,
					  0,1,0,0,
					  0,0,1,0,
					  0,0,0,1,
					  1,0,0,0,
					  0,1,0,0,
					  0,0,1,0,
					  0,0,0,1};
	float *cur = &(matrices[0]), *prev = &(matrices[16]);
	while (running) {
		//1. React to user's input
		bool moved = false;
		if(_kbhit())
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
					moved = true;
					prev[X_index] = cur[X_index];
					cur[X_index] += 0.001f;
					break;

				case 'd':
					moved = true;
					prev[X_index] = cur[X_index];
					cur[X_index] -= 0.001f;
					break;
				case 'w':
					moved = true;
					prev[Y_index] = cur[Y_index];
					cur[Y_index] += 0.001f;
					break;
				case 's':
					moved = true;
					prev[Y_index] = cur[Y_index];
					cur[Y_index] -= 0.001f;
					break;
				case 'r':
					moved = true;
					prev[Z_index] = cur[Z_index];
					cur[Z_index] += 0.0005f;
					break;
				case 'f':
					moved = true;
					prev[Z_index] = cur[Z_index];
					cur[Z_index] -= 0.0005f;
					break;
				case ' ':
					printf("SPACE BAR pressed");
					running = false;
					break;
				//CHANGE STATE
				case '1':
					printf("Run test!\n");
					OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive1, pathTest[0], 0);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive1, posEnd[0], 0);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive2, pathTest[1], 0);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive2, posEnd[1], 0);
					OpenMPD_CWrapper_commitUpdates(pm);
					OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					break;
				case '2':
					printf("Return to origin\n");
					OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive1, pathReturn[0], 0);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive1, posStart[0], 0);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive2, pathReturn[1], 0);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive2, posStart[1], 0);					
					OpenMPD_CWrapper_commitUpdates(pm);
					OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					break;
				case '3':
					printf("Acceleration increased from %f to %f\n", a0, a0 + 10);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						a0 += 10;
						{//Particle 1
							float* testPath_data;
							size_t sizeTestPath = createLinearTest(A1, B1, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
							OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest[0]);
							pathTest[0] = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, sizeTestPath);
							delete testPath_data;
						}
						{//Particle 2
							float* testPath_data;
							size_t sizeTestPath = createLinearTest(A2, B2, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
							OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest[1]);
							pathTest[1] = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, sizeTestPath);
							delete testPath_data;
						}
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}								
					break;
				case '4':
					printf("Acceleration dereased from %f to %f\n", a0, a0 - 10);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						a0 -= 10;
						{//Particle 1
							float* testPath_data;
							size_t sizeTestPath = createLinearTest(A1, B1, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
							OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest[0]);
							pathTest[0] = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, sizeTestPath);
							delete testPath_data;
						}
						{//Particle 2
							float* testPath_data;
							size_t sizeTestPath = createLinearTest(A2, B2, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
							OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest[1]);
							pathTest[1] = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, sizeTestPath);
							delete testPath_data;
						}
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}				
					break;
			}
		//2. Update engine:
		if (moved) 
			printf("(%f, %f, %f)\n", cur[X_index], cur[Y_index], cur[Z_index]);
		float mStarts[32], mEnds[32];
		memcpy(&(mStarts[0]), prev, 16 * sizeof(float));
		memcpy(&(mStarts[16]), prev, 16 * sizeof(float));
		memcpy(&(mEnds[0]), cur, 16 * sizeof(float));
		memcpy(&(mEnds[16]), cur, 16 * sizeof(float));
		OpenMPD_CWrapper_update_HighLevel(pm,primitives, 2, mStarts, mEnds/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);						
	}
	destroyContent(pm);
	return NULL; 
}

void declareContent(OpenMPD_Context_Handler pm) {
	//Generating descriptors for particle 1: 
	{
		float* testPath_data, *returnPath_data;
		size_t sizeTestPath = createLinearTest(A1, B1, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
		size_t sizeReturnPath = createLinearTest(B1, A1, v0, 0, (1.0f*curFPS_Divider) / 40000, &returnPath_data);
		float a1_data[] = { 15000.0f };
		//Create descriptors
		posStart[0] = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, 1);
		posEnd[0] = OpenMPD_CWrapper_createPositionsDescriptor(pm, returnPath_data, 1);
		pathTest[0] = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, sizeTestPath);
		pathReturn[0] = OpenMPD_CWrapper_createPositionsDescriptor(pm, returnPath_data, sizeReturnPath);
		amplitudes[0] = OpenMPD_CWrapper_createAmplitudesDescriptor(pm, a1_data, 1);
		//Create Primitives
		primitive1 = OpenMPD_CWrapper_declarePrimitive(pm, posStart[0], amplitudes[0]);
		OpenMPD_CWrapper_commitUpdates(pm);//Do I need this intermediate commit?
		OpenMPD_CWrapper_setPrimitiveEnabled(pm, primitive1, true);
		OpenMPD_CWrapper_commitUpdates(pm);
		delete testPath_data; delete returnPath_data;
	}
	//Generating descriptors for particle 2: 
	{
		float* testPath_data, *returnPath_data;
		size_t sizeTestPath = createLinearTest(A2, B2, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
		size_t sizeReturnPath = createLinearTest(B2, A2, v0, 0, (1.0f*curFPS_Divider) / 40000, &returnPath_data);
		float a1_data[] = { 15000.0f };
		//Create descriptors
		posStart[1] = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, 1);
		posEnd[1] = OpenMPD_CWrapper_createPositionsDescriptor(pm, returnPath_data, 1);
		pathTest[1] = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, sizeTestPath);
		pathReturn[1] = OpenMPD_CWrapper_createPositionsDescriptor(pm, returnPath_data, sizeReturnPath);
		amplitudes[1] = OpenMPD_CWrapper_createAmplitudesDescriptor(pm, a1_data, 1);
		//Create Primitives
		primitive2 = OpenMPD_CWrapper_declarePrimitive(pm, posStart[1], amplitudes[1]);
		OpenMPD_CWrapper_commitUpdates(pm);//Do I need this intermediate commit?
		OpenMPD_CWrapper_setPrimitiveEnabled(pm, primitive2, true);
		OpenMPD_CWrapper_commitUpdates(pm);
		delete testPath_data; delete returnPath_data;
	}
}

void destroyContent(OpenMPD_Context_Handler pm){
	//Destroy primitives:
	OpenMPD_CWrapper_releasePrimitive(pm, primitive1);
	OpenMPD_CWrapper_releasePrimitive(pm, primitive2);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, posStart[0]);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, posEnd[0]);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest[0]);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathReturn[0]);
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes[0]);	
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, posStart[1]);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, posEnd[1]);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest[1]);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathReturn[1]);
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes[1]);	
}
