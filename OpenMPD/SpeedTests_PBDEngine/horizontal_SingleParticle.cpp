#include <OpenMPD_Prerequisites.h>
#include <OpenMPD_CWrapper.h>
#include <fstream>
#include <string>
#include <stdio.h>
#include <conio.h>
#include <GenerateTestPaths.h>
#include <Windows.h>

//#include <iostream>

void print(const char* msg){ printf("%s\n", msg); }
void* client(void* arg);

unsigned char curFPS_Divider = 4;
cl_uint geometries = 32;
int boardIDs[] = { 1, 2,3,4 };
float matBoardToWorld[64] = {   /*bottom-left*/
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
};
bool foceSync = true;
bool HW_Sync = true;

int main() {
	//SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	do {
		do {
			OpenMPD_CWrapper_Initialize();
			OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
			OpenMPD_CWrapper_SetupEngine(2000000, OpenMPD::GSPAT_SOLVER::V2);
			//OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine_TopBottom(curFPS_Divider, geometries, topBoard, bottomBoard, foceSync);
			OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine(curFPS_Divider, geometries, 4, (cl_uint*)boardIDs, matBoardToWorld, foceSync);
			OpenMPD_CWrapper_SetupPhaseOnly(true);
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
float A[] = { -0.05f, 0, 0.1f,1 }, B[] = { 0.05f, 0, 0.1f, 1 };//0.2388f/2
float a0 = 10;		//Acceleration to test in m/s2 (assumed v0=0 for these tests).
float v0 = 0.05f;	//Return speed (particle returns to origin at this speed, no acceleration).
cl_uint primitive1;
cl_uint posStart, posEnd, pathTest, pathReturn;
cl_uint amplitudes;

void declareContent(OpenMPD_Context_Handler pm);
void destroyContent(OpenMPD_Context_Handler pm);
bool writeDataToFile(float* fileDataIn, std::string filePath, size_t size);

void* client(void* arg) { 
	OpenMPD_Context_Handler pm= (OpenMPD_Context_Handler)arg;
	declareContent(pm);
	printf("Contents created. Press a key to finish.\n");
	bool running = true;
	bool returning = false;
	bool statusChanged = false;

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
		//1. React to user's input
		bool moved = false;
		if(_kbhit())
			switch (_getch()) {
				//UPDATE DIVIDER:
				case 'e':
					curFPS_Divider++;
					OpenMPD_CWrapper_SetupFPS_Divider(curFPS_Divider);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						float* testPath_data;
						size_t sizeTestPath = createLinearTest(A, B, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest);
						pathTest = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, sizeTestPath);
						delete testPath_data;
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}
					break;
				case 'q':
					curFPS_Divider--;
					OpenMPD_CWrapper_SetupFPS_Divider(curFPS_Divider);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						float* testPath_data;
						size_t sizeTestPath = createLinearTest(A, B, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest);
						pathTest = OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, sizeTestPath);
						delete testPath_data;
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
					if (!returning) {
						printf("Run test!\n");
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive1, pathTest, 0);
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive1, posEnd, 0);
						OpenMPD_CWrapper_commitUpdates(pm);
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
						returning = true;
					}
					else {
						printf("Return to origin\n");
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive1, pathReturn, 0);
						OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive1, posStart, 0);
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
						float* testPath_data;
						size_t sizeTestPath = createLinearTest(A, B, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest);
						pathTest =	OpenMPD_CWrapper_createPositionsDescriptor(pm,testPath_data, sizeTestPath);

						/*if (statusChanged) {
							writeDataToFile(testPath_data, "C:\\Roberto\\GitRepos\\0.libraries\\bin\\x64\\ST_H_Acc-" + std::to_string(a0) + ".csv", sizeTestPath);
							statusChanged = false;
						}*/

						delete testPath_data;
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}								
					break;
				case '4':
					printf("Acceleration dereased from %f to %f\n", a0, a0 - 10);
					{
						OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
						a0 =(a0-10>0? a0-10:10);
						float* testPath_data;
						size_t sizeTestPath = createLinearTest(A, B, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
						OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest);
						pathTest =	OpenMPD_CWrapper_createPositionsDescriptor(pm,testPath_data, sizeTestPath);

						/*if (statusChanged) {
							writeDataToFile(testPath_data, "C:\\Roberto\\GitRepos\\0.libraries\\bin\\x64\\ST_H_Acc-" + std::to_string(a0) + ".csv", sizeTestPath);
							statusChanged = false;
						}*/

						delete testPath_data;
						OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
					}				
					break;
				case '5':
					statusChanged = true;
					break;
			}
		//2. Update engine:
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
	float* testPath_data, *returnPath_data;
	size_t sizeTestPath		= createLinearTest(A, B, 0, a0, (1.0f*curFPS_Divider) / 40000, &testPath_data);
	size_t sizeReturnPath	= createLinearTest(B, A, v0, 0, (1.0f*curFPS_Divider) / 40000, &returnPath_data);
	float a1_data[] = { 15000.0f};
	//Create descriptors
	posStart =	OpenMPD_CWrapper_createPositionsDescriptor(pm, testPath_data, 1);
	posEnd =	OpenMPD_CWrapper_createPositionsDescriptor(pm,returnPath_data, 1);
	pathTest =	OpenMPD_CWrapper_createPositionsDescriptor(pm,testPath_data, sizeTestPath);
	pathReturn= OpenMPD_CWrapper_createPositionsDescriptor(pm,returnPath_data, sizeReturnPath );
	amplitudes=OpenMPD_CWrapper_createAmplitudesDescriptor(pm,a1_data, 1);
	//Create Primitives
	primitive1 = OpenMPD_CWrapper_declarePrimitive(pm,posStart, amplitudes);
	OpenMPD_CWrapper_commitUpdates(pm);//Do I need this intermediate commit?
	OpenMPD_CWrapper_setPrimitiveEnabled(pm,primitive1,true);
	OpenMPD_CWrapper_commitUpdates(pm);
	delete testPath_data; delete returnPath_data;
}

void destroyContent(OpenMPD_Context_Handler pm){
	//Destroy primitives:
	OpenMPD_CWrapper_releasePrimitive(pm, primitive1);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, posStart);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, posEnd);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathTest);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, pathReturn);
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes);	
}

bool writeDataToFile(float* fileDataIn, std::string filePath, size_t size)//std::vector<uint8_t>& fileData,
{
	//C:\Roberto\GitRepos\0.libraries\bin\x64
	std::ofstream outputFile(filePath, std::ios::binary);

	std::vector<uint8_t> fileData;
	fileData.resize(size);
	std::memcpy(fileData.data(), &fileDataIn, size);//sizeof(fileDataIn)

	if (outputFile.is_open())
	{
		for(auto e: fileData)


		for (int i = 0; i < fileData.size(); i++)
		{
			char value = (char)fileData[i];
			outputFile.write(&value, sizeof(char));
		}

		outputFile.close();

		return true;
	}

	return false;
}
