#include <GSPAT_RenderingEngine/PBD_Engine_Prerequisites.h>
#include <GSPAT_RenderingEngine/PBD_Engine_CWrapper.h>
#include <AmplitudeModulation/Audio/AmplitudeModulation.h>
#include <stdio.h>
#include <conio.h>
#include <Windows.h>

void print(const char* msg) { printf("%s\n", msg); }
void* client(void* arg);

unsigned char curFPS_Divider = 4;

int main() {
	//SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	do {
		do {
			PBDEngine_CWrapper_Initialize();
			PBDEngine_CWrapper_RegisterPrintFuncs(print, print, print);
			PBDEngine_CWrapper_SetupEngine(2000000, PBDEngine::GSPAT_SOLVER::V2);
			PBD_PrimitiveManager_Handler  pm = PBDEngine_CWrapper_StartEngine(curFPS_Divider, 32, 41, 40, true);
			PBDEngine_CWrapper_SetupPhaseOnly(false);
			client((void*)pm);
			PBDEngine_CWrapper_StopEngine();
			printf("Press any key to Restart the engine.\nPress SPACE BAR to Release current engine.");
		} while (_getch() != ' ');
		PBDEngine_CWrapper_Release();
		printf("Engine Released (all structures destroyed).\nPress any key to Restart a new instance.\nPress SPACE BAR to finish program.");
	} while (_getch() != ' ');
}
//Client thread data
const int MAX_PRIMITIVES = 16;
int numPrimitives = 1;
cl_uint primitives[MAX_PRIMITIVES];
cl_uint positions[MAX_PRIMITIVES];
cl_uint amplitudes[MAX_PRIMITIVES], modulatedAmplitudes[MAX_PRIMITIVES];

void declareContent(PBD_PrimitiveManager_Handler pm);
void destroyContent(PBD_PrimitiveManager_Handler pm);

void* client(void* arg) {
	PBD_PrimitiveManager_Handler pm = (PBD_PrimitiveManager_Handler)arg;
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
				PBDEngine_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
				for (int p = 0; p < numPrimitives; p++) {
					PBDEngine_CWrapper_updatePrimitive_Amplitudes(pm, primitives[p], modulatedAmplitudes[p], 0);
					PBDEngine_CWrapper_updatePrimitive_Amplitudes(pm, primitives[p], amplitudes[p], 0);
				}
				PBDEngine_CWrapper_commitUpdates(pm);
				PBDEngine_CWrapper_RegisterPrintFuncs(print, print, print);
				break;
			}
		//3. Update engine:
		if (moved)
			printf("(%f, %f, %f)\n", curMat[X_index], curMat[Y_index], curMat[Z_index]);
		PBDEngine_CWrapper_update_HighLevel(pm, primitives, numPrimitives, prevMat, curMat/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);
	}
	destroyContent(pm);
	return NULL;
}

void declareContent(PBD_PrimitiveManager_Handler pm) {
	float modulationIndex = 0.2f;		// from 0 to 1. you can get louder sound with a larger modulation index, but with less stable traps...
	float maxAmplitudeInPascal = 20000;	// the maximum amplitude in pascal

	AmplitudeModulation am;
	am.loadFile("Chirp100_5000.wav");					// load the audio file (accepts only .wav files)
	// if you don't want to do this resampling step every time (might take some time), you can save the resampled audio as a new file and use it for the next time
	am.convertSampleRate(40000.f / (float)curFPS_Divider);		// resample the audio at the frame rate of the engine
	//am.modulateAmplitudeDSB(modulationIndex, maxAmplitudeInPascal);// modulate the amplitude to create the audio
	am.modulateAmplitudeSSB(modulationIndex, maxAmplitudeInPascal);// modulate the amplitude to create the audio
	float* am_data;
	size_t sizeAM = am.fillBuffer(&am_data);					// fill the amplitude buffer

	for (int p = 0; p < numPrimitives; p++) {
		float p_data[] = { 0, 0, 0.12f, 1 };		// centre of the system
		float a_data[] = { maxAmplitudeInPascal };
		//Create descriptors
		positions[p] = PBDEngine_CWrapper_createPositionsDescriptor(pm, p_data, 1);
		amplitudes[p] = PBDEngine_CWrapper_createAmplitudesDescriptor(pm, a_data, 1);
		modulatedAmplitudes[p] = PBDEngine_CWrapper_createAmplitudesDescriptor(pm, am_data, sizeAM);
	}

	//Create Primitives
	for (int p = 0; p < numPrimitives; p++) {
		primitives[p] = PBDEngine_CWrapper_declarePrimitive(pm, positions[p], amplitudes[p]);
		PBDEngine_CWrapper_commitUpdates(pm);
		PBDEngine_CWrapper_setPrimitiveEnabled(pm, primitives[p], true);
		PBDEngine_CWrapper_commitUpdates(pm);
	}
	delete am_data;
}

void destroyContent(PBD_PrimitiveManager_Handler pm) {
	//Destroy primitives:
	for (int p = 0; p < numPrimitives; p++) {
		PBDEngine_CWrapper_releasePrimitive(pm, primitives[p]);
		PBDEngine_CWrapper_releasePositionsDescriptor(pm, positions[p]);
		PBDEngine_CWrapper_releaseAmplitudesDescriptor(pm, amplitudes[p]);
		PBDEngine_CWrapper_releaseAmplitudesDescriptor(pm, modulatedAmplitudes[p]);
	}
}
