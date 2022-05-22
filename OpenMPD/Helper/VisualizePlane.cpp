#include "VisualizePlane.h"
#include <Helper\HelperMethods.h>

#include "CImg\CImg.h"
#include "stdio.h"
using namespace cimg_library;

void VisualizePlane::visualize(float A[3], float B[3], float C[3], int imageSize[2], float* transducers_re_im, int boardSize[2], float trans_pitch) {
	CImg<unsigned char> img = CImg<unsigned char>(imageSize[0], imageSize[1], 1, 3);
	float* amplitudes = new float[imageSize[0] * imageSize[1]];
	float avg_Amp = 0, min_Amp = 1000000, max_Amp = 0;;
	//1.Initialize iterator vectors.
	float step_AB[3], step_AC[3];
	step_AB[0] = (B[0] - A[0]) / imageSize[0]; //Step vector in direction AB
	step_AB[1] = (B[1] - A[1]) / imageSize[0];
	step_AB[2] = (B[2] - A[2]) / imageSize[0];
	step_AC[0] = (C[0] - A[0]) / imageSize[1]; //Step vector in direction AC
	step_AC[1] = (C[1] - A[1]) / imageSize[1];
	step_AC[2] = (C[2] - A[2]) / imageSize[1];
	//2. Step through the pixels: 
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			//2.1. Compute 3D position of pixel (px,py)
			float pixelCoords[3];
			pixelCoords[0] = A[0] + px*step_AB[0] + py*step_AC[0];
			pixelCoords[1] = A[1] + px*step_AB[1] + py*step_AC[1];
			pixelCoords[2] = A[2] + px*step_AB[2] + py*step_AC[2];
			float pixelField[3] = { 0,0,0.0f };
			//2.2. Add contribution from each transducer: 
			for (int tx = 0; tx < boardSize[0]; tx++)
				for (int ty = 0; ty < boardSize[1]; ty++) {
					//a. Compute position of the transducer
					int t_index[2]; t_index[0] = tx; t_index[1] = ty;
					float t_pos[3];
					//computeTransducerPos(t_index, boardSize, 0.0105f, t_pos);
					computeTransducerPos_SideBySide(t_index, 0.0105f, t_pos);
					//b. Compute distance and amplitude: 
					float distance, amplitude;
					computeAmplitudeAndDistance(t_pos, pixelCoords, &amplitude, &distance);
					//c. Compute complex field: It will be the product of the transducer state and its propagated field
					//   Propagated:
					float phaseDelay = K()*distance;
					float Re_propagated = amplitude*cosf(phaseDelay);
					float Im_propagated = amplitude*sinf(phaseDelay);
					//   Transducer state: the buffer is stored as {{Re1, Im1}, {Re2, Im2}...}
					float Re_q = transducers_re_im[t_index[1] * boardSize[0] * 2 + t_index[0] * 2];
					float Im_q = transducers_re_im[t_index[1] * boardSize[0] * 2 + t_index[0] * 2 + 1];
					//d. Add to the pixel field (this is still NOT A COLOUR).
					pixelField[0] += Re_q*Re_propagated - Im_q*Im_propagated;
					pixelField[1] += Im_q*Re_propagated + Re_q*Im_propagated;
				}
			//2.3. Map field to pixel colour: 
			float amp_R = sqrt(pixelField[0] * pixelField[0] + pixelField[1] * pixelField[1]);
			amplitudes[py*imageSize[0] + px] = amp_R;
			//Some stats:
			avg_Amp += amp_R ;
			min_Amp = (amp_R < min_Amp ? amp_R : min_Amp);
			max_Amp = (amp_R > max_Amp ? amp_R : max_Amp);
		}

	}
	float ampToColour = 3 * 256 / max_Amp;
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			
			unsigned int amp_R = (unsigned int)(ampToColour*amplitudes[py*imageSize[0] + px]);
			unsigned int R_aux = 0, G_aux = 0, B_aux = 0;
			if (amp_R < 256) R_aux = amp_R;
			else if (amp_R < 512) { R_aux = 255; G_aux = (amp_R - 256); }
			else { R_aux = 255; G_aux = 255; B_aux = amp_R - 512; }
			unsigned char R = (unsigned char)(R_aux);
			unsigned char G = (unsigned char)(G_aux);
			unsigned char B = (unsigned char)(B_aux);
			img(px, py, 0) = R; img(px, py, 1) = G; img(px, py, 2) = B;
		}
	}
		
	static CImgDisplay disp(img, "Playing with images...", false);
	disp.assign(img);
	delete amplitudes;
	printf("\nAVG Amp= %f;\n MIN Amp=%f;\n MAX amp=%f;\n", avg_Amp/ (imageSize[0] * imageSize[1]), min_Amp, max_Amp);
}

void VisualizePlane::visualize(float A[3], float B[3], float C[3], int imageSize[2], float* transducers_re_im, int numTransducers, float* positions) {
	CImg<unsigned char> img = CImg<unsigned char>(imageSize[0], imageSize[1], 1, 3);
	float* amplitudes = new float[imageSize[0] * imageSize[1]];
	float avg_Amp = 0, min_Amp = 1000000, max_Amp = 0;;
	//1.Initialize iterator vectors.
	float step_AB[3], step_AC[3];
	step_AB[0] = (B[0] - A[0]) / imageSize[0]; //Step vector in direction AB
	step_AB[1] = (B[1] - A[1]) / imageSize[0];
	step_AB[2] = (B[2] - A[2]) / imageSize[0];
	step_AC[0] = (C[0] - A[0]) / imageSize[1]; //Step vector in direction AC
	step_AC[1] = (C[1] - A[1]) / imageSize[1];
	step_AC[2] = (C[2] - A[2]) / imageSize[1];
	//2. Step through the pixels: 
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			//2.1. Compute 3D position of pixel (px,py)
			float pixelCoords[3];
			pixelCoords[0] = A[0] + px*step_AB[0] + py*step_AC[0];
			pixelCoords[1] = A[1] + px*step_AB[1] + py*step_AC[1];
			pixelCoords[2] = A[2] + px*step_AB[2] + py*step_AC[2];
			//float pixelField[3] = { 0,0,0.0f };
			//2.2. Add contribution from each transducer: 
			lapack_complex_float pixelField_complex=propagateFieldToPoint(pixelCoords, (lapack_complex_float*)transducers_re_im, positions, numTransducers, 8.02f);
			float* pixelField = (float*)(&pixelField_complex);
			//for (int t = 0; t < numTransducers; t++){
			//		//a. Compute position of the transducer
			//		float* t_pos=&(positions[3*t]);
			//		//b. Compute distance and amplitude: 
			//		float distance, amplitude;
			//		computeAmplitudeAndDistance(t_pos, pixelCoords, &amplitude, &distance);
			//		//c. Compute complex field: It will be the product of the transducer state and its propagated field
			//		//   Propagated:
			//		float phaseDelay = K()*distance;
			//		float Re_propagated = amplitude*cosf(phaseDelay);
			//		float Im_propagated = amplitude*sinf(phaseDelay);
			//		//   Transducer state: the buffer is stored as {{Re1, Im1}, {Re2, Im2}...}
			//		float Re_q = transducers_re_im[t* 2];
			//		float Im_q = transducers_re_im[t* 2 + 1];
			//		//d. Add to the pixel field (this is still NOT A COLOUR).
			//		pixelField[0] += Re_q*Re_propagated - Im_q*Im_propagated;
			//		pixelField[1] += Im_q*Re_propagated + Re_q*Im_propagated;
			//	}
			//2.3. Map field to pixel colour: 
			float amp_R = sqrt(pixelField[0] * pixelField[0] + pixelField[1] * pixelField[1]);
			amplitudes[py*imageSize[0] + px] = amp_R;
			//Some stats:
			avg_Amp += amp_R ;
			min_Amp = (amp_R < min_Amp ? amp_R : min_Amp);
			max_Amp = (amp_R > max_Amp ? amp_R : max_Amp);
		}

	}
	float ampToColour = 3 * 256 / max_Amp;
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			
			unsigned int amp_R = (unsigned int)(ampToColour*amplitudes[py*imageSize[0] + px]);
			unsigned int R_aux = 0, G_aux = 0, B_aux = 0;
			if (amp_R < 256) R_aux = amp_R;
			else if (amp_R < 512) { R_aux = 255; G_aux = (amp_R - 256); }
			else { R_aux = 255; G_aux = 255; B_aux = amp_R - 512; }
			unsigned char R = (unsigned char)(R_aux);
			unsigned char G = (unsigned char)(G_aux);
			unsigned char B = (unsigned char)(B_aux);
			img(px, py, 0) = R; img(px, py, 1) = G; img(px, py, 2) = B;
		}
	}
		
	static CImgDisplay disp(img, "Playing with images...", false);
	disp.assign(img);
	delete amplitudes;
	printf("\nAVG Amp= %f;\n MIN Amp=%f;\n MAX amp=%f;\n", avg_Amp/ (imageSize[0] * imageSize[1]), min_Amp, max_Amp);
}

void VisualizePlane::visualizeFromPhases(float A[3], float B[3], float C[3], int imageSize[2], float* transducers_phases, int numTransducers, float* positions) {
	CImg<unsigned char> img = CImg<unsigned char>(imageSize[0], imageSize[1], 1, 3);
	float* amplitudes = new float[imageSize[0] * imageSize[1]];
	float avg_Amp = 0, min_Amp = 1000000, max_Amp = 0;;
	//1.Initialize iterator vectors.
	float step_AB[3], step_AC[3];
	step_AB[0] = (B[0] - A[0]) / imageSize[0]; //Step vector in direction AB
	step_AB[1] = (B[1] - A[1]) / imageSize[0];
	step_AB[2] = (B[2] - A[2]) / imageSize[0];
	step_AC[0] = (C[0] - A[0]) / imageSize[1]; //Step vector in direction AC
	step_AC[1] = (C[1] - A[1]) / imageSize[1];
	step_AC[2] = (C[2] - A[2]) / imageSize[1];
	//2. Step through the pixels: 
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			//2.1. Compute 3D position of pixel (px,py)
			float pixelCoords[3];
			pixelCoords[0] = A[0] + px*step_AB[0] + py*step_AC[0];
			pixelCoords[1] = A[1] + px*step_AB[1] + py*step_AC[1];
			pixelCoords[2] = A[2] + px*step_AB[2] + py*step_AC[2];
			float pixelField[3] = { 0,0,0.0f };
			//2.2. Add contribution from each transducer: 
			for (int t = 0; t < numTransducers; t++){
					//a. Compute position of the transducer
					float* t_pos=&(positions[3*t]);
					//b. Compute distance and amplitude: 
					float distance, amplitude;
					computeAmplitudeAndDistance(t_pos, pixelCoords, &amplitude, &distance);
					//c. Compute complex field: It will be the product of the transducer state and its propagated field
					//   Propagated:
					float phaseDelay = K()*distance;
					float Re_propagated = amplitude*cosf(phaseDelay);
					float Im_propagated = amplitude*sinf(phaseDelay);
					//   Transducer state: the buffer is stored as {{Re1, Im1}, {Re2, Im2}...}
					float Re_q = cosf(transducers_phases[t]);
					float Im_q = sinf(transducers_phases[t]);
					//d. Add to the pixel field (this is still NOT A COLOUR).
					pixelField[0] += Re_q*Re_propagated - Im_q*Im_propagated;
					pixelField[1] += Im_q*Re_propagated + Re_q*Im_propagated;
				}
			//2.3. Map field to pixel colour: 
			float amp_R = sqrt(pixelField[0] * pixelField[0] + pixelField[1] * pixelField[1]);
			amplitudes[py*imageSize[0] + px] = amp_R;
			//Some stats:
			avg_Amp += amp_R ;
			min_Amp = (amp_R < min_Amp ? amp_R : min_Amp);
			max_Amp = (amp_R > max_Amp ? amp_R : max_Amp);
		}

	}
	float ampToColour = 3 * 256 / max_Amp;
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			
			unsigned int amp_R = (unsigned int)(ampToColour*amplitudes[py*imageSize[0] + px]);
			unsigned int R_aux = 0, G_aux = 0, B_aux = 0;
			if (amp_R < 256) R_aux = amp_R;
			else if (amp_R < 512) { R_aux = 255; G_aux = (amp_R - 256); }
			else { R_aux = 255; G_aux = 255; B_aux = amp_R - 512; }
			unsigned char R = (unsigned char)(R_aux);
			unsigned char G = (unsigned char)(G_aux);
			unsigned char B = (unsigned char)(B_aux);
			img(px, py, 0) = R; img(px, py, 1) = G; img(px, py, 2) = B;
		}
	}
		
	static CImgDisplay disp(img, "Playing with images...", false);
	disp.assign(img);
	delete amplitudes;
	printf("\nAVG Amp= %f;\n MIN Amp=%f;\n MAX amp=%f;\n", avg_Amp/ (imageSize[0] * imageSize[1]), min_Amp, max_Amp);
}



/*
void VisualizePlane::visualizeAndSave(float A[3], float B[3], float C[3], int imageSize[2], float * transducers_re_im, int boardSize[2], 
	float trans_pitch, char label[11], bool saveImages, bool isFloorLevel)
{
	CImg<unsigned char> img = CImg<unsigned char>(imageSize[0], imageSize[1], 1, 3);
	float* amplitudes = new float[imageSize[0] * imageSize[1]];
	float avg_Amp = 0, min_Amp = 1000000, max_Amp = 0;;
	//1.Initialize iterator vectors.
	float step_AB[3], step_AC[3];
	step_AB[0] = (B[0] - A[0]) / imageSize[0]; //Step vector in direction AB
	step_AB[1] = (B[1] - A[1]) / imageSize[0];
	step_AB[2] = (B[2] - A[2]) / imageSize[0];
	step_AC[0] = (C[0] - A[0]) / imageSize[1]; //Step vector in direction AC
	step_AC[1] = (C[1] - A[1]) / imageSize[1];
	step_AC[2] = (C[2] - A[2]) / imageSize[1];
	//2. Step through the pixels: 
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			//2.1. Compute 3D position of pixel (px,py)
			float pixelCoords[3];
			pixelCoords[0] = A[0] + px * step_AB[0] + py * step_AC[0];
			pixelCoords[1] = A[1] + px * step_AB[1] + py * step_AC[1];
			pixelCoords[2] = A[2] + px * step_AB[2] + py * step_AC[2];
			float pixelField[3] = { 0,0,0.0f };
			//2.2. Add contribution from each transducer: 
			for (int tx = 0; tx < boardSize[0]; tx++)
				for (int ty = 0; ty < boardSize[1]; ty++) {
					//a. Compute position of the transducer
					int t_index[2]; t_index[0] = tx; t_index[1] = ty;
					float t_pos[3];
					//computeTransducerPos(t_index, boardSize, 0.0105f, t_pos);
					computeTransducerPos_SideBySide(t_index, 0.0105f, t_pos);
					//b. Compute distance and amplitude: 
					float distance, amplitude;
					computeAmplitudeAndDistance(t_pos, pixelCoords, &amplitude, &distance);
					//c. Compute complex field: It will be the product of the transducer state and its propagated field
					//   Propagated:
					float phaseDelay = K()*distance;
					float Re_propagated = amplitude * cosf(phaseDelay);
					float Im_propagated = amplitude * sinf(phaseDelay);
					//   Transducer state: the buffer is stored as {{Re1, Im1}, {Re2, Im2}...}
					float Re_q = transducers_re_im[t_index[1] * boardSize[0] * 2 + t_index[0] * 2];
					float Im_q = transducers_re_im[t_index[1] * boardSize[0] * 2 + t_index[0] * 2 + 1];
					//d. Add to the pixel field (this is still NOT A COLOUR).
					pixelField[0] += Re_q * Re_propagated - Im_q * Im_propagated;
					pixelField[1] += Im_q * Re_propagated + Re_q * Im_propagated;
				}
			//2.3. Map field to pixel colour: 
			float amp_R = sqrt(pixelField[0] * pixelField[0] + pixelField[1] * pixelField[1]);
			amplitudes[py*imageSize[0] + px] = amp_R;
			//Some stats:
			avg_Amp += amp_R;
			min_Amp = (amp_R < min_Amp ? amp_R : min_Amp);
			max_Amp = (amp_R > max_Amp ? amp_R : max_Amp);
		}

	}
	float ampToColour = 3 * 256 / max_Amp;
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {

			unsigned int amp_R = (unsigned int)(ampToColour*amplitudes[py*imageSize[0] + px]);
			unsigned int R_aux = 0, G_aux = 0, B_aux = 0;
			if (amp_R < 256) R_aux = amp_R;
			else if (amp_R < 512) { R_aux = 255; G_aux = (amp_R - 256); }
			else { R_aux = 255; G_aux = 255; B_aux = amp_R - 512; }
			unsigned char R = (unsigned char)(R_aux);
			unsigned char G = (unsigned char)(G_aux);
			unsigned char B = (unsigned char)(B_aux);
			img(px, py, 0) = R; img(px, py, 1) = G; img(px, py, 2) = B;
		}
	}

	// saving the image
	if (saveImages) {
		char newLabel[18];
		newLabel[0] = label[0];
		newLabel[1] = label[1];
		newLabel[2] = label[2];
		newLabel[3] = label[3];
		newLabel[4] = label[4];
		newLabel[5] = label[5];
		newLabel[6] = label[6];
		newLabel[7] = label[7];
		newLabel[8] = label[8];
		newLabel[9] = '_';
		newLabel[10] = '0';
		newLabel[11] = '0';
		newLabel[12] = '0';
		newLabel[13] = '.';
		newLabel[14] = 'b';
		newLabel[15] = 'm';
		newLabel[16] = 'p';
		newLabel[17] = '\0';

		if (!isFloorLevel)
			img.save(newLabel);
		else {
			newLabel[10] = '1';
			newLabel[11] = '1';
			newLabel[12] = '1';
			img.save(newLabel);
		}
		
	}
	static CImgDisplay disp(img, "Playing with images...", false);
	disp.assign(img);
	delete amplitudes;
	//printf("\nAVG Amp= %f;\n MIN Amp=%f;\n MAX amp=%f;\n", avg_Amp/ (imageSize[0] * imageSize[1]), min_Amp, max_Amp);
}
*/

/*
void VisualizePlane::Figure2a(float A[3], float B[3], float C[3], int imageSize[2], float separation) {
	CImg<unsigned char> img = CImg<unsigned char>(imageSize[0], imageSize[1], 1, 3);
	float* amplitudes = new float[imageSize[0] * imageSize[1]];
	float avg_Amp = 0, min_Amp = 1000000, max_Amp = 0;;
	float t1_pos[3] = { 0,0,0 };
	float t2_pos[3] = { 0,0, separation };
	float t1_phase = 0;
	float t2_phase = separation / lambda();


	//1.Initialize iterator vectors.
	float step_AB[3], step_AC[3];
	step_AB[0] = (B[0] - A[0]) / imageSize[0]; //Step vector in direction AB
	step_AB[1] = (B[1] - A[1]) / imageSize[0];
	step_AB[2] = (B[2] - A[2]) / imageSize[0];
	step_AC[0] = (C[0] - A[0]) / imageSize[1]; //Step vector in direction AC
	step_AC[1] = (C[1] - A[1]) / imageSize[1];
	step_AC[2] = (C[2] - A[2]) / imageSize[1];
	//2. Step through the pixels: 
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			//2.1. Compute 3D position of pixel (px,py)
			float pixelCoords[3];
			pixelCoords[0] = A[0] + px*step_AB[0] + py*step_AC[0];
			pixelCoords[1] = A[1] + px*step_AB[1] + py*step_AC[1];
			pixelCoords[2] = A[2] + px*step_AB[2] + py*step_AC[2];
			float pixelField[3] = { 0,0,0.0f };
			//2.2. Add contribution from t1: 
			{
			//b. Compute distance and amplitude: 
			float distance, amplitude;
			computeAmplitudeAndDistance(t1_pos, pixelCoords, &amplitude, &distance, 0.0025f);
			
			//c. Compute complex field: It will be the product of the transducer state and its propagated field
			//   Propagated:
			float phaseDelay = K()*distance;
			float Re_propagated = amplitude*cosf(phaseDelay);
			float Im_propagated = amplitude*sinf(phaseDelay);
			//   Transducer state: the buffer is stored as {{Re1, Im1}, {Re2, Im2}...}
			float Re_q = cos(t1_phase);
			float Im_q = sin(t1_phase);
			//d. Add to the pixel field (this is still NOT A COLOUR).
			pixelField[0] += Re_q*Re_propagated - Im_q*Im_propagated;
			pixelField[1] += Im_q*Re_propagated + Re_q*Im_propagated;
			}
			//2.2. Add contribution from t2: 
			{
			//b. Compute distance and amplitude: 
			float distance, amplitude;
			computeAmplitudeAndDistance(t2_pos, pixelCoords, &amplitude, &distance, 0.0025f);
			
			//c. Compute complex field: It will be the product of the transducer state and its propagated field
			//   Propagated:
			float phaseDelay = K()*distance;
			float Re_propagated = amplitude*cosf(phaseDelay);
			float Im_propagated = amplitude*sinf(phaseDelay);
			//   Transducer state: the buffer is stored as {{Re1, Im1}, {Re2, Im2}...}
			float Re_q = cos(t2_phase);
			float Im_q = sin(t2_phase);
			//d. Add to the pixel field (this is still NOT A COLOUR).
			pixelField[0] += Re_q*Re_propagated - Im_q*Im_propagated;
			pixelField[1] += Im_q*Re_propagated + Re_q*Im_propagated;
			}
			//2.3. Map field to pixel colour: 
			float amp_R = sqrt(pixelField[0] * pixelField[0] + pixelField[1] * pixelField[1]);
			amplitudes[py*imageSize[0] + px] = amp_R;
			//Some stats:
			avg_Amp += amp_R ;
			min_Amp = (amp_R < min_Amp ? amp_R : min_Amp);
			max_Amp = (amp_R > max_Amp ? amp_R : max_Amp);
		}

	}
	float ampToColour = 3 * 256 / max_Amp;
	for (int px = 0; px < imageSize[0]; px++) {
		for (int py = 0; py < imageSize[1]; py++) {
			
			unsigned int amp_R = (unsigned int)(ampToColour*amplitudes[py*imageSize[0] + px]);
			unsigned int R_aux = 0, G_aux = 0, B_aux = 0;
			if (amp_R < 256) R_aux = amp_R;
			else if (amp_R < 512) { R_aux = 255; G_aux = (amp_R - 256); }
			else { R_aux = 255; G_aux = 255; B_aux = amp_R - 512; }
			unsigned char R = (unsigned char)(R_aux);
			unsigned char G = (unsigned char)(G_aux);
			unsigned char B = (unsigned char)(B_aux);
			img(px, py, 0) = R; img(px, py, 1) = G; img(px, py, 2) = B;
		}
	}
		
	static CImgDisplay disp(img, "Playing with images...", false);
	disp.assign(img);
	delete amplitudes;
	//printf("\nAVG Amp= %f;\n MIN Amp=%f;\n MAX amp=%f;\n", avg_Amp/ (imageSize[0] * imageSize[1]), min_Amp, max_Amp);
}*/