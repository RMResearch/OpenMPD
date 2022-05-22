#include "AmplitudeModulation.h"

// read and write from/to an audio file
bool AmplitudeModulation::loadFile(std::string fileName) {
	bool success = audioFile.load(fileName);
	if (!success)
		std::cout << "file could not open" << std::endl;
	return success;
}
bool AmplitudeModulation::saveFile(std::string fileName) {
	bool success = audioFile.save(fileName);
	if (!success)
		std::cout << "file could not open" << std::endl;
	return success;
}
// generate a test audio signal using single/multiple frequency
void AmplitudeModulation::generateSingleFrequencyAudio(float amplitude, float frequency, int numSamples, int numChannels, int sampleRate, int bitDepth) {
	AudioFile<float>::AudioBuffer buffer;
	buffer.resize(numChannels);
	for (int i = 0; i < numChannels; i++)
		buffer[i].resize(numSamples);

	for (int k = 0; k < numChannels; k++)
		for (int j = 0; j < numSamples; j++)
			buffer[k][j] = amplitude * sin(2.0 * M_PI * ((float)j / sampleRate) * frequency);

	audioFile.setAudioBuffer(buffer);
	audioFile.setSampleRate(sampleRate);
	audioFile.setBitDepth(bitDepth);

}
void AmplitudeModulation::generateMultiFrequencyAudio(std::vector<float> amplitudes, std::vector<float> frequencies, int numSamples, int numChannels, int sampleRate, int bitDepth) {
	AudioFile<float>::AudioBuffer buffer;
	buffer.resize(numChannels);
	for (int i = 0; i < numChannels; i++)
		buffer[i].resize(numSamples);

	int numWaves = amplitudes.size();
	for (int k = 0; k < numChannels; k++) {
		for (int j = 0; j < numSamples; j++) {
			float sample = 0.0f;
			for (int i = 0; i < numWaves; i++)
				sample += amplitudes[i] * sin(2.0 * M_PI * ((float)j / sampleRate) * frequencies[i]);

			buffer[k][j] = sample;
		}
	}
	audioFile.setAudioBuffer(buffer);
	audioFile.setSampleRate(sampleRate);
	audioFile.setBitDepth(bitDepth);

}

// convert the sampling rate of the audio signal
void AmplitudeModulation::convertSampleRate(int newSampleRate) {
	int currentSampleRate = audioFile.getSampleRate();
	int currentNumSamples = audioFile.getNumSamplesPerChannel();
	int newNumChannels = audioFile.getNumChannels();
	int newNumSamples = currentNumSamples * ((float)newSampleRate / currentSampleRate);

	AudioFile<float>::AudioBuffer buffer;
	buffer.resize(newNumChannels);
	for (int j = 0; j < newNumChannels; j++)
		buffer[j].resize(newNumSamples);

	for (int j = 0; j < newNumChannels; j++) {
		for (int i = 0; i < newNumSamples; i++) {
			float t = (float)i / newSampleRate;
			float index = t * currentSampleRate;
			int intIndex = (int)index;
			float decIndex = index - intIndex;
			buffer[j][i] = (1 - decIndex) * audioFile.samples[j][intIndex] + decIndex * audioFile.samples[j][intIndex + 1];
		}
	}

	audioFile.setAudioBuffer(buffer);
	audioFile.setSampleRate(newSampleRate);
}
// normalize the ampliutde so that the maximum amplitude is going to be 'amplitudeMax'
void AmplitudeModulation::normalizeAudio(float amplitudeMax) {
	float absMax = 0.0;
	for (int j = 0; j < audioFile.getNumChannels(); j++) {
		for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++) {
			float absSignal = abs(audioFile.samples[j][i]);
			if (absMax < absSignal) absMax = absSignal;
		}
	}
	float normValue = (absMax != 0 ? amplitudeMax / absMax : 0);
	for (int j = 0; j < audioFile.getNumChannels(); j++)
		for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++)
			audioFile.samples[j][i] *= normValue;
}
// modulate the amplutude using double sideband modulation (modulation index m is from 0 to 1)
void AmplitudeModulation::modulateAmplitudeDSB(float m, float amplitudeInPascal) {
	int numChannels = audioFile.getNumChannels();
	int numSamples = audioFile.getNumSamplesPerChannel();

	normalizeAudio(1.f); // the audio signal should be from -1 to +1
	modulatedSignal.resize(numChannels);
	for (int j = 0; j < numChannels; j++) {
		modulatedSignal[j].resize(audioFile.getNumSamplesPerChannel());
		for (int i = 0; i < numSamples; i++) {
			modulatedSignal[j][i] = amplitudeInPascal * (1.f + m * audioFile.samples[j][i]) / (1.f + m);
		}
	}
}
void AmplitudeModulation::modulateAmplitudeSSB(float m, float amplitudeInPascal) {
	int numChannels = audioFile.getNumChannels();
	int numSamples = audioFile.getNumSamplesPerChannel();

	normalizeAudio(1.f); // the audio signal should be from -1 to +1
	modulatedSignal.resize(numChannels);

	calculateHilbert();

	for (int j = 0; j < numChannels; j++) {
		modulatedSignal[j].resize(audioFile.getNumSamplesPerChannel());
		for (int i = 0; i < numSamples; i++) {
			float a = m * hilbert[j][i];
			float b = 1.0 + m * audioFile.samples[j][i];
			modulatedSignal[j][i] = sqrt(a * a + b * b);
			// modulatedPhase[j][i] = atan2(b, a); // not in use...
		}
	}

	// normalize
	float amax = 0.0f;
	for (int j = 0; j < audioFile.getNumChannels(); j++)
		for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++)
			amax = fmax(amax, modulatedSignal[j][i]);
	for (int j = 0; j < audioFile.getNumChannels(); j++)
		for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++)
			modulatedSignal[j][i] *= (amplitudeInPascal / amax);

}

// fill an amplitude buffer
size_t AmplitudeModulation::fillBuffer(float** buffer, int channel) {
	int numSamples = audioFile.getNumSamplesPerChannel();

	*buffer = new float[numSamples];
	for (size_t i = 0; i < numSamples; i++) {
		(*buffer)[i] = modulatedSignal[channel][i];
	}
	return numSamples;
}

 //calculate the Hilbert function of the audio input (this function needs fftw library)
void AmplitudeModulation::calculateHilbert() {
	hilbert.resize(audioFile.getNumChannels());
	for (int i = 0; i < audioFile.getNumChannels(); i++)
		hilbert[i].resize(audioFile.getNumSamplesPerChannel());

	int N = audioFile.getNumSamplesPerChannel();
	fftw_complex *y;
	y = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);

	int windowLength = 200;
	for (int j = 0; j < audioFile.getNumChannels(); j++) {
		for (int i = 0; i < N; i++) {
			float w = 1.0;
			if (i < windowLength) w = 0.5 - 0.5 * cos(i * M_PI / windowLength);
			if (i >= N - windowLength) w = 0.5 - 0.5 * cos((i - N + 2 * windowLength) * M_PI / windowLength);
			y[i][0] = audioFile.samples[j][i] * w;
			y[i][1] = 0;
		}

		fftw_plan plan = fftw_plan_dft_1d(N, y, y, FFTW_FORWARD, FFTW_ESTIMATE);
		fftw_execute(plan);
		fftw_destroy_plan(plan);
		int hN = N >> 1;
		int numRem = hN;

		for (int i = 1; i < hN; i++) {
			y[i][0] *= 2;
			y[i][1] *= 2;
		}

		if (N % 2 == 0)
			numRem--;
		else if (N > 1) {
			y[hN][0] *= 2;
			y[hN][1] *= 2;
		}

		memset(&y[hN + 1][0], 0, numRem * sizeof(fftw_complex));
		plan = fftw_plan_dft_1d(N, y, y, FFTW_BACKWARD, FFTW_ESTIMATE);
		fftw_execute(plan);
		fftw_destroy_plan(plan);
		fftw_cleanup();

		for (int i = 0; i < N; i++) {
			hilbert[j][i] = y[i][1] / N;
		}
	}
	fftw_free(y);
}
