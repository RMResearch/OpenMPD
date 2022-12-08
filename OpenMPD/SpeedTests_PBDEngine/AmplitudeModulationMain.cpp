#include <OpenMPD_Prerequisites.h>
#include <OpenMPD_CWrapper.h>
#include <../SpeedTests_PBDEngine/Audio/AmplitudeModulation.h>
//#include <AmplitudeModulation/Audio/AmplitudeModulation.h>
#include <stdio.h>
#include <conio.h>
#include <Windows.h>

// prototypes
void print(const char* msg) { printf("%s\n", msg); }
void* client(void* arg);
void writeArrayToCSV(std::string fileName, float* data, int dataSize);

// variables
unsigned char curFPS_Divider = 4;
cl_uint geometries = 32;
cl_uint topBoard = 40;
cl_uint bottomBoard = 41;
bool foceSync = true;
bool phaseOnly = false;
bool HW_Sync = true;
// it must be a .wav file. this file should be stored on the LIBS_HOME/bin/x64
std::string fileName = "Chirp100_5000"; 
int numPrimitives = 1;

int main() {
	do {
		do {
			OpenMPD_CWrapper_Initialize();
			OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
			OpenMPD_CWrapper_SetupEngine(2000000, OpenMPD::GSPAT_SOLVER::IBP);
			OpenMPD_Context_Handler  pm = OpenMPD_CWrapper_StartEngine_TopBottom(curFPS_Divider, geometries, topBoard, bottomBoard, foceSync);
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
const int MAX_PRIMITIVES = 16;
cl_uint primitives[MAX_PRIMITIVES];
cl_uint positions[MAX_PRIMITIVES];
cl_uint amplitudes[MAX_PRIMITIVES], modulatedAmplitudes[MAX_PRIMITIVES];
void declareContent(OpenMPD_Context_Handler pm);
void destroyContent(OpenMPD_Context_Handler pm);

void* client(void* arg) {
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
		//1. React to user's input
		bool moved = false;
		if (_kbhit())
			switch (_getch()) {
				//MOVE
			case 'a':
				moved = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + X_index] = curMat[16 * p + X_index];
					curMat[16 * p + X_index] += 0.001f;
				}
				break;

			case 'd':
				moved = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + X_index] = curMat[16 * p + X_index];
					curMat[16 * p + X_index] -= 0.001f;
				}
				break;
			case 'w':
				moved = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + Y_index] = curMat[16 * p + Y_index];
					curMat[16 * p + Y_index] += 0.001f;
				}
				break;
			case 's':
				moved = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + Y_index] = curMat[16 * p + Y_index];
					curMat[16 * p + Y_index] -= 0.001f;
				}
				break;
			case 'r':
				moved = true;
				for (int p = 0; p < numPrimitives; p++) {
					prevMat[16 * p + Z_index] = curMat[16 * p + Z_index];
					curMat[16 * p + Z_index] += 0.0005f;
				}
				break;
			case 'f':
				moved = true;
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
				printf("Start amplitude modulation!\n");
				OpenMPD_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
				for (int p = 0; p < numPrimitives; p++) {
					OpenMPD_CWrapper_updatePrimitive_Amplitudes(pm, primitives[p], modulatedAmplitudes[0], 0);
					OpenMPD_CWrapper_updatePrimitive_Amplitudes(pm, primitives[p], amplitudes[0], 0);
				}

				OpenMPD_CWrapper_commitUpdates(pm);
				OpenMPD_CWrapper_RegisterPrintFuncs(print, print, print);
				break;
			}
		//3. Update engine:
		if (moved)
			printf("(%f, %f, %f)\n", curMat[X_index], curMat[Y_index], curMat[Z_index]);
		OpenMPD_CWrapper_update_HighLevel(pm, primitives, numPrimitives, prevMat, curMat/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);
	}
	destroyContent(pm);
	return NULL;
}

void declareContent(OpenMPD_Context_Handler pm) {
	// generate amplitud descriptors
	float modulationIndex = 0.2f;		// from 0 to 1. you can get louder sound with a larger modulation index, but with less stable traps...
	float maxAmplitudeInPascal = 20000;	// the maximum amplitude in pascal

	AmplitudeModulation am;	
	am.loadFile(fileName+".wav");					// load the audio file (accepts only .wav files)
	// if you don't want to do this resampling step every time (might take some time), you can save the resampled audio as a new file and use it for the next time
	am.convertSampleRate(40000.f / (float)curFPS_Divider);		// resample the audio at the frame rate of the engine
	am.modulateAmplitudeDSB(modulationIndex, maxAmplitudeInPascal);// modulate the amplitude to create the audio
	//am.generateMultiFrequencyAudio()
	//am.modulateAmplitudeSSB(modulationIndex, maxAmplitudeInPascal);// modulate the amplitude to create the audio
	float* am_data;
	size_t sizeAM = am.fillBuffer(&am_data);					// fill the amplitude buffer
	//writeArrayToCSV(fileName, am_data, sizeAM);
	float a_data[] = { maxAmplitudeInPascal };
	float p_data0[] = {  0.00f, 0, 0.12f, 1 };		// centre of the system
	float p_data1[] = { -0.02f, 0, 0.12f, 1 };		// centre of the system
	float p_data2[] = {  0.02f, 0, 0.12f, 1 };		// centre of the system
	
	for (int p = 0; p < numPrimitives; p++) {
		float p_data[] = { -0.02 +((float)p*0.02f), 0, 0.12f, 1 };		// centre of the system
		float a_data[] = { maxAmplitudeInPascal };
		//Create descriptors
		positions[p] = OpenMPD_CWrapper_createPositionsDescriptor(pm, p_data, 1);
		amplitudes[p] = OpenMPD_CWrapper_createAmplitudesDescriptor(pm, a_data, 1);
		modulatedAmplitudes[p] = OpenMPD_CWrapper_createAmplitudesDescriptor(pm, am_data, sizeAM);
	}

	//Create Primitives
	for (int p = 0; p < numPrimitives; p++) {
		primitives[p] = OpenMPD_CWrapper_declarePrimitive(pm, positions[p], amplitudes[p]);
		OpenMPD_CWrapper_commitUpdates(pm);
		OpenMPD_CWrapper_setPrimitiveEnabled(pm, primitives[p], true);
 		OpenMPD_CWrapper_commitUpdates(pm);
	}
	delete am_data;
}

void writeArrayToCSV(std::string fileName, float* data, int dataSize) {
 	std::ofstream out(fileName + ".csv");

	for (int i = 0; i < dataSize; i++) {
		float val = *(data+i);
			out << val << ',';
		out << '\n';
	}
	out.close();
}

void destroyContent(OpenMPD_Context_Handler pm) {
	//Destroy primitives:
	for (int p = 0; p < numPrimitives; p++) {
		OpenMPD_CWrapper_releasePrimitive(pm, primitives[p]);
		OpenMPD_CWrapper_releasePositionsDescriptor(pm, positions[p]);
		OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes[p]);
		OpenMPD_CWrapper_releaseAmplitudesDescriptor(pm, modulatedAmplitudes[p]);
	}
}
