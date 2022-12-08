#include <OpenMPD_Prerequisites.h>
#include <OpenMPD_CWrapper.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <vector>
#include <string>
#include <deque>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#define _USE_MATH_DEFINES

void print(const char* msg){ printf("%s\n", msg); }
void* client(OpenMPD_Context_Handler pm, char* file);

unsigned char curFPS_Divider = 4;
int main(int argc, char **argv) {
	//0. Check arguments
	/*if (argc != 2) {
		printf("Incorrect input parameters:\n\t %s <filename.csv>\n ", argv[0]);
		return 0;
	}*/
	do {
		do {
			OpenMPD_CWrapper_Initialize();
			OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
			OpenMPD_CWrapper_SetupEngine(20000000, OpenMPD::GSPAT_SOLVER::V2);
			OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine_TopBottom(curFPS_Divider , 32, 41, 40, false);// top, bottom
			OpenMPD_CWrapper_SetupPhaseOnly(true);
			OpenMPD_CWrapper_SetupHardwareSync(true);
			client(pm, "OptiTrap_fish.csv");// OptiTrap_cardioid.csv  OptiTrap_fish.csv  OptiTrap_squircle.csv  OptiTrap_circle.csv
			OpenMPD_CWrapper_StopEngine();
			printf("Press any key to Restart the engine.\nPress SPACE BAR to Release current engine.");
		} while (_getch() != ' ');
		OpenMPD_CWrapper_Release();
		printf("Engine Released (all structures destroyed).\nPress any key to Restart a new instance.\nPress SPACE BAR to finish program.");
	} while (_getch() != ' ');
}

void declareContentFromCSV(OpenMPD_Context_Handler pm, char * filename, cl_uint& primitive, cl_uint& path, cl_uint& initialPos, cl_uint& amplitudes, cl_uint& am);
void destroyContent(OpenMPD_Context_Handler pm, cl_uint& primitive, cl_uint& path, cl_uint& initialPos, cl_uint& amplitudes, cl_uint& AM);

void* client(OpenMPD_Context_Handler pm, char* file) { 
	cl_uint primitive;
	cl_uint path, initialPos ;
	cl_uint amplitudes, AM;
	declareContentFromCSV(pm, file, primitive, path, initialPos, amplitudes, AM);
	printf("Application started. Place a bead in starting position.\n Then press keys to control application:\n");	
	printf("\t - WASD to move particle\n");
	printf("\t - Q/E to reduce/increase UPS\n");
	printf("\t - Numbers to select path to render [1=initial; 2=POV path]\n");
	printf("\t - SPACEBAR to stop execution.\n");
	bool running = true;
	cl_uint primitives[] = { primitive };//Declared as global shared variables
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
		bool updateObjectMatrix = false;
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
		//MOVE:
		case 'a':
			updateObjectMatrix = true;
			prev[X_index] = cur[X_index];
			cur[X_index] += 0.001f;
			prev[Y_index] = cur[Y_index];
			break;

		case 'd':
			updateObjectMatrix = true;
			prev[X_index] = cur[X_index];
			cur[X_index] -= 0.001f;
			prev[Y_index] = cur[Y_index];
			break;
		case 'w':
			updateObjectMatrix = true;
			prev[Y_index] = cur[Y_index];
			cur[Y_index] += 0.001f;
			prev[X_index] = cur[X_index];
			break;
		case 's':
			updateObjectMatrix = true;
			prev[Y_index] = cur[Y_index];
			cur[Y_index] -= 0.001f;
			prev[X_index] = cur[X_index];
			break;
		case ' ':
			printf("SPACE BAR pressed <<<--- \n");
			running = false;
			break;
		case '2':
			printf("State: Animating <<<--- \n");
			OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive, path, 0);
			OpenMPD_CWrapper_commitUpdates(pm);
			break;
		case '1':
			printf("State: Initial Position <<<--- \n"); 
			OpenMPD_CWrapper_updatePrimitive_Positions(pm,primitive, initialPos, 0);
			OpenMPD_CWrapper_commitUpdates(pm);
			break;

		case '3':
			printf("State: AM is On <<<--- \n");
			OpenMPD_CWrapper_updatePrimitive_Amplitudes(pm, primitive, AM, 0);
			OpenMPD_CWrapper_commitUpdates(pm);
			break;
		case '4':
			printf("State: AM is Off<<<--- \n");
			OpenMPD_CWrapper_updatePrimitive_Amplitudes(pm, primitive, amplitudes, 0);
			OpenMPD_CWrapper_commitUpdates(pm);
			break;
		}

		//Update engine:
		if (updateObjectMatrix) {
			printf("(%f, %f, %f)\n", cur[X_index], cur[Y_index], cur[Z_index]);
			OpenMPD_CWrapper_update_HighLevel(pm,primitives, 1, prev, cur/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);
		}
		//OpenMPD_CWrapper_commitUpdates(pm);
	}
	destroyContent(pm, primitive, path, initialPos, amplitudes, AM);
	return NULL; 
}

std::vector<std::string> split(std::string& input, char delimiter)
{
	std::istringstream stream(input);
	std::string field;
	std::vector<std::string> result;
	while (getline(stream, field, delimiter)) {
		result.push_back(field);
	}
	return result;
}

void declareContentFromCSV(OpenMPD_Context_Handler pm, char * filename, cl_uint& primitive, cl_uint& path, cl_uint& initialPos, cl_uint& amplitudes, cl_uint& AM) {
	//0. Reading the file:
	std::vector<float> values;
	std::ifstream ifs(filename); //	<----------------------------CHANGE FILENAME HERE
	std::string line;
	while (getline(ifs, line)) {
		std::vector<std::string> strvec = split(line, ',');
		for (int i = 0; i < strvec.size(); i++) {
			values.push_back(stof(strvec.at(i)));
		}
	}
	//1. copy data to our path descriptors:
	float *path_Data = new float[values.size()]; 
	float initialPos_Data[4];
	float amplitudes_Data[] = { 15000.0f};

	//B. Audio at 200Hz (used as "audio" for fixed position).
	static const int  min = 12500, range = 2500, FPS = 10000, freq = 200;
	float* amp_data = new float[FPS / freq];
	for (int s = 0; s < FPS / freq; s++)
		amp_data[s] = min + range * cosf(2 * CL_M_PI * (1.0f * s) / (FPS / freq));
	int sampleNum = FPS / freq;
	//float amp_data[FPS / freq];// = new float[FPS / freq];
	//for (int s = 0; s < FPS / freq; s++)
	//	amp_data[s] = min + range * cosf(2 * CL_M_PI * (1.0f * s) / (FPS / freq));

	memcpy(path_Data, values.data(), values.size() * sizeof(float));
	memcpy(initialPos_Data, values.data(), 4 * sizeof(float));
	//2. Create the descriptors
	path=OpenMPD_CWrapper_createPositionsDescriptor(pm, path_Data, values.size()/4);
	initialPos = OpenMPD_CWrapper_createPositionsDescriptor(pm, initialPos_Data, 1);
	amplitudes = OpenMPD_CWrapper_createAmplitudesDescriptor(pm, amplitudes_Data, 1);
	AM         = OpenMPD_CWrapper_createAmplitudesDescriptor(pm, amp_data, sampleNum);
	//3. Create the Primitive (it will use the initialPos and the fixed amplitudes.
	primitive = OpenMPD_CWrapper_declarePrimitive(pm, initialPos, amplitudes);
	OpenMPD_CWrapper_commitUpdates(pm);
	OpenMPD_CWrapper_setPrimitiveEnabled(pm,primitive,true);
	OpenMPD_CWrapper_commitUpdates(pm);
	delete path_Data;	
	delete amp_data;
}

void destroyContent(OpenMPD_Context_Handler pm, cl_uint& primitive, cl_uint& path, cl_uint& initialPos, cl_uint& amplitudes, cl_uint& AM){
	//Destroy primitives:
	OpenMPD_CWrapper_releasePrimitive(pm, primitive);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, path);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, initialPos);
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes); 
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, AM);
}

