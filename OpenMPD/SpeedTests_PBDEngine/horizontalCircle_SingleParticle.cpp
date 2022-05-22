#include <OpenMPD_Prerequisites.h>
#include <OpenMPD_CWrapper.h>
#include <stdio.h>
#include <conio.h>
#include <GenerateTestPaths.h>
#include <Windows.h>

void print(const char* msg){ printf("%s\n", msg); }
void* client(void* arg);

unsigned char curFPS_Divider = 4;

int main() {
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	do {
		do {
			OpenMPD_CWrapper_Initialize();
			OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
			OpenMPD_CWrapper_SetupEngine(2000000);
			OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine(curFPS_Divider , 32, 4, 2, true);
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
float O[] = { 0, 0, 0.1f,1 }, radius = 0.03f;//0.2388f/2
float a0 = 10;		//Acceleration to test in m/s2.

cl_uint primitive1;
cl_uint initialPos, pathAccelerate, pathCircle, pathDecelerate;
cl_uint amplitudes;

void declareContent(OpenMPD_Context_Handler pm);
void destroyContent(OpenMPD_Context_Handler pm);

void* client(void* arg) { 
	OpenMPD_Context_Handler pm= (OpenMPD_Context_Handler)arg;
	declareContent(pm);
	printf("Contents created. Press a key to finish.\n");
	bool running = true;
	bool returning = false;
	cl_uint primitives[] = { primitive1 };//Declared as global shared variables
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
		//2. React to user's input
		bool moved = false;
		if(_kbhit())
			switch (_getch()) {
				//UPDATE DIVIDER:
				case 'e':
					curFPS_Divider++;
					OpenMPD_CWrapper_SetupFPS_Divider(curFPS_Divider);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						//Release previous descriptors (no effect untill commit)
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathAccelerate);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathCircle);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathDecelerate);
						//Create new descriptors
						float* accelPath_data, *circlePath_data, *decelPath_data;
						size_t sizeAccelPath, sizeCirclePath, sizeDecelPath;
						createCircleTest(O, radius, a0, (1.0f*curFPS_Divider) / 40000, &accelPath_data, &sizeAccelPath, &circlePath_data, &sizeCirclePath, &decelPath_data, &sizeDecelPath);
						pathAccelerate = OpenMPD_CWrapper_createPositionsDescriptor(pm, accelPath_data, sizeAccelPath);
						pathCircle = OpenMPD_CWrapper_createPositionsDescriptor(pm, circlePath_data, sizeCirclePath);
						pathDecelerate = OpenMPD_CWrapper_createPositionsDescriptor(pm, decelPath_data, sizeDecelPath);
						delete accelPath_data; delete circlePath_data; delete decelPath_data;
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}
					break;
				case 'q':
					curFPS_Divider--;
					OpenMPD_CWrapper_SetupFPS_Divider(curFPS_Divider);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						//Release previous descriptors (no effect untill commit)
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathAccelerate);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathCircle);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathDecelerate);
						//Create new descriptors
						float* accelPath_data, *circlePath_data, *decelPath_data;
						size_t sizeAccelPath, sizeCirclePath, sizeDecelPath;
						createCircleTest(O, radius, a0, (1.0f*curFPS_Divider) / 40000, &accelPath_data, &sizeAccelPath, &circlePath_data, &sizeCirclePath, &decelPath_data, &sizeDecelPath);
						pathAccelerate = OpenMPD_CWrapper_createPositionsDescriptor(pm, accelPath_data, sizeAccelPath);
						pathCircle = OpenMPD_CWrapper_createPositionsDescriptor(pm, circlePath_data, sizeCirclePath);
						pathDecelerate = OpenMPD_CWrapper_createPositionsDescriptor(pm, decelPath_data, sizeDecelPath);
						delete accelPath_data; delete circlePath_data; delete decelPath_data;
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}
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
					if (!returning)
					{
						printf("Run test!\n");
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive1, pathAccelerate, 0);
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive1, pathCircle, 0);//Enqueued after pathAccelerate. While no other descriptors are added, it will be looped.
						OpenMPD_CWrapper_commitUpdates(pm);
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
						returning = true;
					}
					else {
						printf("Return to origin\n");
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive1, pathDecelerate, 0);
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive1, initialPos, 0);//Enqueued after deceleration. Looped afterwards.
						OpenMPD_CWrapper_commitUpdates(pm);
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
						returning = false;
					}
					break;
				case '3':
					printf("Acceleration increased from %f to %f\n", a0, a0 + 10);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						a0 += 10;
						//Release previous descriptors (no effect untill commit)
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathAccelerate);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathCircle);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathDecelerate);
						//Create new descriptors
						float* accelPath_data, *circlePath_data, *decelPath_data;
						size_t sizeAccelPath, sizeCirclePath, sizeDecelPath;
						createCircleTest(O, radius, a0, (1.0f*curFPS_Divider) / 40000, &accelPath_data, &sizeAccelPath, &circlePath_data, &sizeCirclePath, &decelPath_data, &sizeDecelPath);
						pathAccelerate =	OpenMPD_CWrapper_createPositionsDescriptor(pm,accelPath_data, sizeAccelPath);
						pathCircle =	OpenMPD_CWrapper_createPositionsDescriptor(pm,circlePath_data, sizeCirclePath);
						pathDecelerate= OpenMPD_CWrapper_createPositionsDescriptor(pm,decelPath_data, sizeDecelPath );
						delete accelPath_data; delete circlePath_data; delete decelPath_data;
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}								
					break;
				case '4':
					printf("Acceleration dereased from %f to %f\n", a0, a0 - 10);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						a0 -= 10;
						//Release previous descriptors (no effect untill commit)
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathAccelerate);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathCircle);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathDecelerate);
						//Create new descriptors
						float* accelPath_data, *circlePath_data, *decelPath_data;
						size_t sizeAccelPath, sizeCirclePath, sizeDecelPath;
						createCircleTest(O, radius, a0, (1.0f*curFPS_Divider) / 40000, &accelPath_data, &sizeAccelPath, &circlePath_data, &sizeCirclePath, &decelPath_data, &sizeDecelPath);
						pathAccelerate =	OpenMPD_CWrapper_createPositionsDescriptor(pm,accelPath_data, sizeAccelPath);
						pathCircle =	OpenMPD_CWrapper_createPositionsDescriptor(pm,circlePath_data, sizeCirclePath);
						pathDecelerate= OpenMPD_CWrapper_createPositionsDescriptor(pm,decelPath_data, sizeDecelPath );
						delete accelPath_data; delete circlePath_data; delete decelPath_data;
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}				
					break;
			}
		//3. Update engine:
		if (moved) 
			printf("(%f, %f, %f)\n", cur[X_index], cur[Y_index], cur[Z_index]);
		float mStarts[16], mEnds[16];
		memcpy(&(mStarts[0]), prev, 16 * sizeof(float));
		memcpy(&(mEnds[0]), cur, 16 * sizeof(float));
		OpenMPD_CWrapper_update_HighLevel(pm,primitives, 1, mStarts, mEnds/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);						
	}
	destroyContent(pm);
	return NULL; 
}

void declareContent(OpenMPD_Context_Handler pm) {
	float* accelPath_data, *circlePath_data, *decelPath_data;
	size_t sizeAccelPath, sizeCirclePath, sizeDecelPath;
	createCircleTest(O, radius, a0, (1.0f*curFPS_Divider) / 40000, &accelPath_data, &sizeAccelPath, &circlePath_data, &sizeCirclePath, &decelPath_data, &sizeDecelPath);
	float a1_data[] = { 15000.0f};
	//Create descriptors
	initialPos =	OpenMPD_CWrapper_createPositionsDescriptor(pm, accelPath_data, 1);
	pathAccelerate =	OpenMPD_CWrapper_createPositionsDescriptor(pm,accelPath_data, sizeAccelPath);
	pathCircle =	OpenMPD_CWrapper_createPositionsDescriptor(pm,circlePath_data, sizeCirclePath);
	pathDecelerate= OpenMPD_CWrapper_createPositionsDescriptor(pm,decelPath_data, sizeDecelPath );
	amplitudes=OpenMPD_CWrapper_createAmplitudesDescriptor(pm,a1_data, 1);
	//Create Primitives
	primitive1 = OpenMPD_CWrapper_declarePrimitive(pm,initialPos, amplitudes);
	OpenMPD_CWrapper_commitUpdates(pm);//Do I need this intermediate commit?
	OpenMPD_CWrapper_setPrimitiveEnabled(pm,primitive1,true);
	OpenMPD_CWrapper_commitUpdates(pm);
	delete accelPath_data;
	delete circlePath_data;
	delete decelPath_data;
}

void destroyContent(OpenMPD_Context_Handler pm){
	//Destroy primitives:initialPos, pathAccelerate, pathCircle, pathDecelerate
	OpenMPD_CWrapper_releasePrimitive(pm, primitive1);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, initialPos);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathAccelerate);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathCircle);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathDecelerate);
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes);	
}
