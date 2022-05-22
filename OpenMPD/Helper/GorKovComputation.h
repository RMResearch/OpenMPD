#pragma once
#include <Helper/HelperMethods.h>
/**
	Contains methods for simple computation of forces and stiffnesses, based on Gor'Kov. 
	Each method contains two versions, one for a fized top-bottom setup with 512 transducers (legacy, this was the only setup supported back in time)
	and a second version where the position and number of transducers can be adjusted (specified as a parameter).
	In either case, all transducers are assumed equal and their propertiesdetermined by the constants
	in HelperMethods.h. The reference pressure P0 for the transducers is set as a last (optional)parameter, 
	which defaults to 8.02Pa at 1 meter (value measured from our transducers). 
*/

namespace GorKovComputation {

	/**
		Computes the derivative of the complex field at a given point, relative to a given direction (e.g. axis X,Y,Z, or any other).
		The direction vector MUST BE unitary
		We compute the derivative of the complex field at a given point (required for GorKov).
		To compute the derivatives, we use a 5 point kernell:
			-see http://web.media.mit.edu/~crtaylor/calculator.html
			- Locations (-2,-1,0,1,2), Derivative Order (1)
	*/
	lapack_complex_float computeDerivativeOfFieldAtPoint(float point[3], float direction[3], lapack_complex_float* field, int boardSize[2], float pitch, float delta = lambda() / 64, float P0=8.02f);
	lapack_complex_float computeDerivativeOfFieldAtPoint(float point[3], float direction[3], lapack_complex_float* field, const float* transducerPositions,int numTransducers, float delta = lambda() / 64, float P0=8.02f);

	/**
		Computes GorKov potential at a given point P.
		It uses the parameters (lambda, K, densitites) described in HelperMethods.h
	*/
	float computeGorKovAtPoint(float point[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta = lambda() / 64, float P0=8.02f);
	float computeGorKovAtPoint(float point[3], lapack_complex_float* singlePointField, const float* transducerPositions,int numTransducers, float delta = lambda() / 64, float P0=8.02f);

	/**
		This way of computing K1 and K2 is not straight from literature (e.g. Asier's Nature Comms paper).
		This is the expression they use in their Java framework for their computations (numerical stability?).
	*/
	float precomputeGorKovK1();

	/**
		This way of computing K1 and K2 is not straight from literature (e.g. Asier's Nature Comms paper).
		This is the expression they use in their Java framework for their computations (numerical stability?).
	*/
	float precomputeGorKovK2();


	/**
		Computes the derivative of the Gorkov potential field at a given point, relative to a given direction (e.g. axis X,Y,Z, or any other).
		The direction vector MUST BE unitary
		We compute the derivative of the complex field at a given point (required for GorKov).
		To compute the derivatives, we use a 5 point kernell:
			-see http://web.media.mit.edu/~crtaylor/calculator.html
			- Locations (-2,-1,0,1,2), Derivative Order (1)
	*/
	float computeDerivativeOfGorKovAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta = lambda() / 64, float P0=8.02f);
	float computeDerivativeOfGorKovAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField, const float* transducerPositions,int numTransducers, float delta = lambda() / 64, float P0=8.02f);

	/**
		Computes forces as the gradient of Gorkov potential along each direction (Force = - Nabla dot Gorkov)
	*/
	void computeAcousticForceAtPoint(float* out_force, float point[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta = lambda() / 64, float P0=8.02f);
	void computeAcousticForceAtPoint(float* out_force, float point[3], lapack_complex_float* singlePointField, const float* transducerPositions,int numTransducers, float delta = lambda() / 64, float P0=8.02f);

	/*
		Compute Stiffness (Laplaccian of Gorkov potential) at a given point.
		We compute this by adding the second derivatives of GorKov along each direction.
		To compute the second derivatives, we use a 5 point kernell:
			-see http://web.media.mit.edu/~crtaylor/calculator.html
			- Locations (-2,-1,0,1,2), Derivative Order (2)
	*/
	void computeStiffness(float* out_grad_force, float point[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta = lambda() / 64, float P0=8.02f);
	void computeStiffness(float* out_grad_force, float point[3], lapack_complex_float* singlePointField, const float* transducerPositions,int numTransducers, float delta = lambda() / 64, float P0=8.02f);
	
	/**
		To compute the second derivatives, we use a 5 point kernell
			- see http://web.media.mit.edu/~crtaylor/calculator.html
			- Locations (-2,-1,0,1,2), Derivative Order (2)
	*/

	float computeSecondDerivativeOfGorKovAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta = lambda() / 64, float P0=8.02f);
	void computeDerivativeOfAcousticForceAtPoint(float* out_grad_force, float point[3], float direction[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta = lambda() / 64,float P0=8.02f);
	void computeForceGradients(float* out_grad_force, float point[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta= lambda() / 64, float P0=8.02f);

	float computeSecondDerivativeOfGorKovAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField	, const float* transducerPositions,int numTransducers, float delta = lambda() / 64, float P0=8.02f);
	void computeDerivativeOfAcousticForceAtPoint(float* out_grad_force, float point[3], float direction[3], lapack_complex_float* singlePointField, const float* transducerPositions ,int numTransducers, float delta = lambda() / 64, float P0=8.02f);
	void computeForceGradients(float* out_grad_force, float point[3], lapack_complex_float* singlePointField	, const float* transducerPositions,int numTransducers, float delta = lambda() / 64, float P0=8.02f);

};