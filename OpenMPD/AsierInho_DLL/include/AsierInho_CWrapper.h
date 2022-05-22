/**
	Simple C Wrapper for Board controller.
	The methods are a direct transcription to those used by the C++ interface
	For a description of what each of them does, please refer to AsierInho.h
*/
#ifndef _ASIERINHO_CWRAPPER
#define _ASIERINHO_CWRAPPER
#include <AsierInho_Prerequisites.h>

extern "C" {
		_AsierInho_Export bool AsierInho_CWrapper_Initialize();
		_AsierInho_Export bool AsierInho_CWrapper_Release();
		_AsierInho_Export void AsierInho_CWrapper_RegisterPrintFuncs(void(*p_Message)(const char*), void(*p_Warning)(const char*), void(*p_Error)(const char*));
		_AsierInho_Export AsierInho_Handler AsierInho_CWrapper_createHandler();
		_AsierInho_Export void AsierInho_CWrapper_destroyHandler(AsierInho_Handler h);
		//_AsierInho_Export void AsierInho_CWrapper_readAdjustments(AsierInho_Handler h, int* transducerIds, int* phaseAdjust);
		_AsierInho_Export void AsierInho_CWrapper_readParameters(AsierInho_Handler h, float* transducerPositions, int* transducerIds, int* phaseAdjust, float* amplitudeAdjust, int* numDiscreteLevels);
		_AsierInho_Export void AsierInho_CWrapper_modeReset(AsierInho_Handler h, int mode = AsierInho::NORMAL);
		_AsierInho_Export void AsierInho_CWrapper_updateMessage(AsierInho_Handler h, unsigned char* messages);
		_AsierInho_Export void AsierInho_CWrapper_turnTransducersOn(AsierInho_Handler h);
		_AsierInho_Export void AsierInho_CWrapper_turnTransducersOff(AsierInho_Handler h);
		_AsierInho_Export bool AsierInho_CWrapper_connect(AsierInho_Handler h, AsierInho::BoardType boardType, int bottomBoardID, int topBoardID);
		_AsierInho_Export void AsierInho_CWrapper_disconnect(AsierInho_Handler h);
		_AsierInho_Export float AsierInho_CWrapper_boardHeight(AsierInho_Handler h);
		_AsierInho_Export void AsierInho_CWrapper_updateFrame(AsierInho_Handler h, AsierInho::vec3 position, AsierInho::vec3 colour = AsierInho::vec3(0, 0, 0), float amplitude = 1.0f, float phase = 0.0f, bool twinTrap = true);
		_AsierInho_Export void AsierInho_CWrapper_updateMultipleFrames(AsierInho_Handler h, int numFramesPerUpdate, AsierInho::vec3 *positions, AsierInho::vec3 *colours = nullptr, float *amplitudes = nullptr, float *phases = nullptr, bool *twinFlags = nullptr);
		_AsierInho_Export void AsierInho_CWrapper_discretizePhases(AsierInho_Handler h, float* phases, unsigned char* discretePhases);
		_AsierInho_Export unsigned char AsierInho_CWrapper_discretizePhase(AsierInho_Handler h, float phase);
		_AsierInho_Export void AsierInho_CWrapper_discretizeAmplitudes(AsierInho_Handler h, float* amplitudes, unsigned char* discreteAmplitudes);
		_AsierInho_Export unsigned char AsierInho_CWrapper_discretizeAmplitude(AsierInho_Handler h, float amplitude);
		_AsierInho_Export void AsierInho_CWrapper_correctPhasesShift(AsierInho_Handler h, unsigned char* discretePhases, unsigned char* discreteAmplitudes);
		_AsierInho_Export void AsierInho_CWrapper_updateDiscretePhases(AsierInho_Handler h, unsigned char* discretePhases);
		_AsierInho_Export void AsierInho_CWrapper_updateDiscretePhasesAndAmplitudes(AsierInho_Handler h, unsigned char* discretePhases, unsigned char* discreteAmplitudes);

};
#endif