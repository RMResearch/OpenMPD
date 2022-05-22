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
#include <Windows.h>
#define _USE_MATH_DEFINES

void print(const char* msg){ printf("%s\n", msg); }
void* client(OpenMPD_Context_Handler pm, char* accel_file, char* cyclic_file);

//Example commnad line parameters: 
//     <yourApp.exe> 4 2 10000 circle_accel_constraint_1.0_mm.csv epicycloid_periodic_constraint_1.0_mm.csv  

unsigned char curFPS_Divider = 4;

int main(int argc, char **argv) {
	//0. Check arguments
	if (argc != 6) {
		printf("Incorrect input parameters:\n\t %s <TopBoardID> <BotBoardID> <Framerate> <accel_filename.csv> <shape_filename.csv>\n ", argv[0]);
		return 0;
	}
	int botBoardID=atoi(argv[1]), topBoardID=atoi(argv[2]);	
	curFPS_Divider= atoi(argv[3]);	
	do {
		do {
			OpenMPD_CWrapper_Initialize();
			OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
			OpenMPD_CWrapper_SetupEngine(2000000);
			bool forceSync = true;
			OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine(curFPS_Divider , 32, topBoardID , botBoardID, forceSync);//boards: 8&7
			client(pm, (char*) argv[4],(char*) argv[5]);
			OpenMPD_CWrapper_StopEngine();
			printf("Press any key to Restart the engine.\nPress SPACE BAR to Release current engine.");
		} while (_getch() != ' ');
		OpenMPD_CWrapper_Release();
		printf("Engine Released (all structures destroyed).\nPress any key to Restart a new instance.\nPress SPACE BAR to finish program.");
	} while (_getch() != ' ');
}

void declareContentFromCSV(OpenMPD_Context_Handler pm, char * filename, cl_uint& path, cl_uint& initialPos, cl_uint& amplitudes);
void destroyContent(OpenMPD_Context_Handler pm,  cl_uint& path, cl_uint& initialPos, cl_uint& amplitudes);

void* client(OpenMPD_Context_Handler pm, char* accel_file, char* cyclic_file) { 
	//0. Define variables to contain our content
	//Paths that our content can use (initial pos, acceleration path and cycli path) NOTE: I think accel_initPos==cyclic_initPos, but I am not sure (I will not use cyclic_initPos)
	cl_uint accel_initPos, accel_path, cyclic_initPos, cyclic_path ;
	//Amplitudes could vary along paths (e.g. to create autio), but I do not think we will use this
	cl_uint accel_amplitudes, cyclic_amplitudes;
	//Our content is controlled via a "Primitive", which we can use to determine the paths/amplitudes to use
	//at each point and to move it as a whole (using a homogeneous transformation matrix)
	cl_uint primitive;
	
	//1. Read configuration (positions) from files and configure the primitive
	declareContentFromCSV(pm, accel_file, accel_path, accel_initPos, accel_amplitudes);
	declareContentFromCSV(pm, cyclic_file, cyclic_path, cyclic_initPos, cyclic_amplitudes);
	primitive = OpenMPD_CWrapper_declarePrimitive(pm,accel_initPos, accel_amplitudes);
	OpenMPD_CWrapper_commitUpdates(pm);
	OpenMPD_CWrapper_setPrimitiveEnabled(pm,primitive,true);
	OpenMPD_CWrapper_commitUpdates(pm);
	
	//2. Run the application
	{
		printf("Application started. Place a bead in starting position.\n Then press keys to control application:\n");
		printf("\t - WASD to move primitive\n");
		printf("\t - Numbers to select path to render [1=initial (or stop); 2=Accel+Cyclic path]\n");
		printf("\t - SPACEBAR to stop execution.\n");
		bool running = true;
		cl_uint primitives[] = { primitive };//Declared as global shared variables
		static const size_t X_index = 3, Y_index = 7, Z_index = 11;
		float matrices[] = { 1,0,0,0,
						  0,1,0,0,
						  0,0,1,0,
						  0,0,0,1,
						  1,0,0,0,
						  0,1,0,0,
						  0,0,1,0,
						  0,0,0,1 };
		float *cur = &(matrices[0]), *prev = &(matrices[16]);
		while (running) {
			if (_kbhit()) {
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

				case 'a':
					cur[X_index] += 0.001f;
					break;

				case 'd':
					cur[X_index] -= 0.001f;
					break;
				case 'w':
					cur[Y_index] += 0.001f;
					break;
				case 's':
					cur[Y_index] -= 0.001f;
					break;
				case ' ':
					printf("SPACE BAR pressed");
					running = false;
					break;
				case '2':
					//Load the two paths (we leave a little pause, so that it starts rendering the accel_path, before declaring the new one)
					OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive, accel_path, 0);
					OpenMPD_CWrapper_commitUpdates(pm);
					Sleep(50);
					OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive, cyclic_path, 0);
					OpenMPD_CWrapper_commitUpdates(pm);
					break;
				case '1':
					//We load the "initial" position path... we should probably add a deceleration path to do this properly...
					OpenMPD_CWrapper_updatePrimitive_Positions(pm, primitive, accel_initPos, 0);
					OpenMPD_CWrapper_commitUpdates(pm);
					break;
				}
				printf("(%f, %f, %f)\n", cur[X_index], cur[Y_index], cur[Z_index]);
				memcpy(prev, cur, 16 * sizeof(float));
			}
			//Update engine: Updates the general position of the primitive (via its matrix)
			OpenMPD_CWrapper_update_HighLevel(pm, primitives, 1, prev, cur/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);				
		}
	}

	//3. Destroy contents
	OpenMPD_CWrapper_releasePrimitive(pm, primitive);
	destroyContent(pm, accel_path, accel_initPos, accel_amplitudes);
	destroyContent(pm, cyclic_path, cyclic_initPos, cyclic_amplitudes);
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

void declareContentFromCSV(OpenMPD_Context_Handler pm, char * filename,  cl_uint& path, cl_uint& initialPos, cl_uint& amplitudes) {
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
	float amplitudes_Data[1] = { 15000.0f };//Needs to be a multiple of 4 in length.
	memcpy(path_Data, values.data(), values.size() * sizeof(float));
	memcpy(initialPos_Data, values.data(), 4 * sizeof(float));
	//2. Create the descriptors
	path=OpenMPD_CWrapper_createPositionsDescriptor(pm, path_Data, values.size()/4);
	initialPos=OpenMPD_CWrapper_createPositionsDescriptor(pm,initialPos_Data, 1);
	amplitudes=OpenMPD_CWrapper_createAmplitudesDescriptor(pm,amplitudes_Data, 1);
	
	delete path_Data;	
}

void destroyContent(OpenMPD_Context_Handler pm, cl_uint& path, cl_uint& initialPos, cl_uint& amplitudes){
	//Destroy primitives:
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, path);
	OpenMPD_CWrapper_releasePositionsDescriptor(pm, initialPos);
	OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes);
}

