#pragma once
/**
	Helper class to visualize the complex field propagated from our transducers across a 2D plane
	- Parameters A, B, C define the corners of the plane
	- imageSize is the resolution in pixels of the image to render (WARNING-> It is slow as hell, carefull with increasing resolution)
	- boardsize is the number of transducers in our array. I have been playing with 16x16 and 32x16 (assumes top&bottom config)
	- trans_pitch describes the distance between transducers in our boards.
	- transducer_re_im is a buffer with the complex representation of each transducer.
	  For a 32x16 board it is stored SIDE BY SIDE as follows:
		ROW 0: bottom(0,0)  bottom(0,1) .... bottom(0,15)   top(0,0)   top(0,1)   ...  top(0,15)
		ROW 0: bottom(1,0)  bottom(1,1) .... bottom(1,15)   top(1,0)   top(1,1)   ...  top(1,15)
		           ...                  ....                         ...
		ROW 0: bottom(15,0) bottom(15,1).... bottom(15,15)  top(15,0)  top(15,1)  ...  top(15,15)
	  WHY DID I NOT STORE IT TOP/BOTTOM? a) cause I'm mean; b) cause I'm stupid (pick the one you prefer)
*/

class VisualizePlane {
	int a;
public: 	
	static void  visualize(float A[3], float B[3], float C[3], int imageSize[2], float* transducers_re_im, int boardSize[2], float trans_pitch) ;
	static void  visualize(float A[3], float B[3], float C[3], int imageSize[2], float* transducers_re_im, int numTransducers, float* positions) ;
	static void  visualizeFromPhases(float A[3], float B[3], float C[3], int imageSize[2], float* transducers_phases, int numTransducers, float* positions);
	//static void  visualizeAndSave(float A[3], float B[3], float C[3], int imageSize[2], float* transducers_re_im, int boardSize[2], float trans_pitch, char label[11], bool saveImages, bool isFloorLevel);	
	//static void  Figure2a(float A[3], float B[3], float C[3], int imageSize[2], float separation);
};