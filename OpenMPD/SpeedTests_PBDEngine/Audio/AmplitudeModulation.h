#pragma once
#include "AudioFile.h"
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <fstream>
//#include <../fftw335_64/fftw3.h>
//#include <fftw3.h>

class AmplitudeModulation {
public:
	AmplitudeModulation() { ; }
	~AmplitudeModulation() { ; }

	// read and write from/to an audio file
	bool loadFile(std::string fileName);
	bool saveFile(std::string fileName);
	// generate a test audio signal using single/multiple frequency
	void generateSingleFrequencyAudio(float amplitude, float frequency, int numSamples, int numChannels = 1, int sampleRate = 40000, int bitDepth = 16);
	void generateMultiFrequencyAudio(std::vector<float> amplitudes, std::vector<float> frequencies, int numSamples, int numChannels = 1, int sampleRate = 40000, int bitDepth = 16);

	// convert the sampling rate of the audio signal
	void convertSampleRate(int newSampleRate);
	// normalize the ampliutde so that the maximum amplitude is going to be 'amplitudeMax'
	void normalizeAudio(float amplitudeMax = 1.0);
	// modulate the amplutude (modulation index m is from 0 to 1)
	void modulateAmplitudeDSB(float m, float amplitudeInPascal); // using double sideband modulation 
	//void modulateAmplitudeSSB(float m, float amplitudeInPascal); // using single sidebanc modulation (this function needs fftw library)

	// fill an amplitude buffer
	size_t fillBuffer(float** buffer, int channel = 0);

	// print summary of the audio file
	void printSummary() { audioFile.printSummary(); };
protected:
	AudioFile<float> audioFile;
	AudioFile<float>::AudioBuffer modulatedSignal;	// modulated signal

	//// in case you want to use Single Sideband modulation (you need fftw library)
	//AudioFile<float>::AudioBuffer modulatedPhase;	// not in use...
	//AudioFile<float>::AudioBuffer hilbert;			// hilbert function of the audio file
	//void calculateHilbert();						// this function needs fftw library
};
