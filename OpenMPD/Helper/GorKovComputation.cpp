#include "GorKovComputation.h"

using namespace GorKovComputation;

/**
	Computes the derivative of the complex field at a given point, relative to a given direction (e.g. axis X,Y,Z, or any other).
	The direction vector MUST BE unitary
	We compute the derivative of the complex field at a given point (required for GorKov).
	To compute the derivatives, we use a 5 point kernell:
		-see http://web.media.mit.edu/~crtaylor/calculator.html
		- Locations (-2,-1,0,1,2), Derivative Order (1)
*/
lapack_complex_float GorKovComputation::computeDerivativeOfFieldAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta, float P0) {
	int w =  boardSize[0], h = boardSize[1];
	float transducerPositions[512*3];
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {		
			int t_index[2]; t_index[0] = i; t_index[1] = j;
			computeTransducerPos_SideBySide(t_index, pitch, &(transducerPositions[3*(w*j+i)]));
		}
	}
	return computeDerivativeOfFieldAtPoint(point, direction, singlePointField, (const float*)transducerPositions, 512, delta, P0);
	////Compute four points around P, for our central differences.
	//float P_2delta[3] = {	point[0]+2*delta*direction[0], 
	//						point[1]+2*delta*direction[1], 
	//						point[2]+2*delta*direction[2] };
	//float P_1delta[3] = {	point[0]+delta*direction[0], 
	//						point[1]+delta*direction[1], 
	//						point[2]+delta*direction[2] };
	//float P_neg1delta[3] = {point[0]-delta*direction[0], 
	//						point[1]-delta*direction[1], 
	//						point[2]-delta*direction[2] };
	//float P_neg2delta[3] = {point[0]-2*delta*direction[0], 
	//						point[1]-2*delta*direction[1], 
	//						point[2]-2*delta*direction[2] };
	////1. compute complex fields at those 4 points:
	//lapack_complex_float field_P_2delta =	propagateFieldToPoint(P_2delta, singlePointField, boardSize, pitch);
	//lapack_complex_float field_P_1delta =	propagateFieldToPoint(P_1delta, singlePointField, boardSize, pitch);
	//lapack_complex_float field_P_neg1delta = propagateFieldToPoint(P_neg1delta, singlePointField, boardSize, pitch);
	//lapack_complex_float field_P_neg2delta = propagateFieldToPoint(P_neg2delta, singlePointField, boardSize, pitch);
	////2. Compute derivative:
	//lapack_complex_float result;
	//result.real = (field_P_neg2delta.real - 8 * field_P_neg1delta.real + 8 * field_P_1delta.real - field_P_2delta.real)/(12*delta);
	//result.imag = (field_P_neg2delta.imag - 8 * field_P_neg1delta.imag + 8 * field_P_1delta.imag - field_P_2delta.imag)/(12*delta);
	//return result;

}

lapack_complex_float GorKovComputation::computeDerivativeOfFieldAtPoint(float point[3], float direction[3], lapack_complex_float* field, const float* transducerPositions, int numTransducers, float delta, float P0) {
	//Compute four points around P, for our central differences.
	float P_2delta[3] = {	point[0]+2*delta*direction[0], 
							point[1]+2*delta*direction[1], 
							point[2]+2*delta*direction[2] };
	float P_1delta[3] = {	point[0]+delta*direction[0], 
							point[1]+delta*direction[1], 
							point[2]+delta*direction[2] };
	float P_neg1delta[3] = {point[0]-delta*direction[0], 
							point[1]-delta*direction[1], 
							point[2]-delta*direction[2] };
	float P_neg2delta[3] = {point[0]-2*delta*direction[0], 
							point[1]-2*delta*direction[1], 
							point[2]-2*delta*direction[2] };
	//1. compute complex fields at those 4 points:
	lapack_complex_float field_P_2delta =	propagateFieldToPoint(P_2delta, field,transducerPositions, numTransducers, P0 );
	lapack_complex_float field_P_1delta =	propagateFieldToPoint(P_1delta, field,transducerPositions, numTransducers, P0);
	lapack_complex_float field_P_neg1delta = propagateFieldToPoint(P_neg1delta, field,transducerPositions, numTransducers, P0);
	lapack_complex_float field_P_neg2delta = propagateFieldToPoint(P_neg2delta, field,transducerPositions, numTransducers, P0);
	//2. Compute derivative:
	lapack_complex_float result;
	result.real = (field_P_neg2delta.real - 8 * field_P_neg1delta.real + 8 * field_P_1delta.real - field_P_2delta.real)/(12*delta);
	result.imag = (field_P_neg2delta.imag - 8 * field_P_neg1delta.imag + 8 * field_P_1delta.imag - field_P_2delta.imag)/(12*delta);
	return result;

}

/**
	This way of computing K1 and K2 is not straight from literature (e.g. Asier's Nature Comms paper). 
	This is the expression they use in their Java framework for their computations (numerical stability?). 
*/
static float GorKovComputation::precomputeGorKovK1() {
         float kapa = 1.0f / (rho_a() * (c_a()*c_a()));
         float kapa_p = 1.0f / (rho_p() * (c_p()*c_p()));
         float k_tilda = kapa_p / kapa;
         float f_1_bruus = 1.0f - k_tilda;
        
         float roh_tilda = rho_p() / rho_a();
         float f_2_bruus = (2.0f * (roh_tilda - 1.0f)) / ((2.0f * roh_tilda) + 1.0f);
        
         float vkPreToVel = 1.0f / (rho_a()*omega());
         float vkPre = f_1_bruus*0.5f*kapa*0.5f;
         float vkVel = f_2_bruus*(3.0f/4.0f)*rho_a()*0.5f;
         float vpVol = (float)((4.0f/3.0f)*M_PI*(particleRadius()*particleRadius()*particleRadius()));
        
         return vpVol * vkPre;
}
/**
	This way of computing K1 and K2 is not straight from literature (e.g. Asier's Nature Comms paper). 
	This is the expression they use in their Java framework for their computations (numerical stability?). 
*/
static float GorKovComputation::precomputeGorKovK2() {
         float kapa = 1.0f / (rho_a() * (c_a()*c_a()));
         float kapa_p = 1.0f / (rho_p() * (c_p()*c_p()));
         float k_tilda = kapa_p / kapa;
         float f_1_bruus = 1.0f - k_tilda;
        
         float roh_tilda = rho_p() / rho_a();
         float f_2_bruus = (2.0f * (roh_tilda - 1.0f)) / ((2.0f * roh_tilda) + 1.0f);
        
         float vkPreToVel = 1.0f / (rho_a()*omega());
         float vkPre = f_1_bruus*0.5f*kapa*0.5f;
         float vkVel = f_2_bruus*(3.0f/4.0f)*rho_a()*0.5f;
         float vpVol = (float)((4.0f/3.0f)*M_PI*(particleRadius()*particleRadius()*particleRadius()));
        
         return vpVol * vkVel*vkPreToVel*vkPreToVel;   
}


/**
	Computes GorKov potential at a given point P. 
	It uses the parameters (lambda, K, densitites) described in HelperMethods.h to precompute K1 and K2.
*/
float GorKovComputation::computeGorKovAtPoint(float point[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta, float P0) {
	int w =  boardSize[0], h = boardSize[1];
	float transducerPositions[512*3];
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {		
			int t_index[2]; t_index[0] = i; t_index[1] = j;
			computeTransducerPos_SideBySide(t_index, pitch, &(transducerPositions[3*(w*j+i)]));
		}
	}
	return computeGorKovAtPoint(point, singlePointField, (const float*)transducerPositions, 512, delta, P0);
	
	//static float X []= { 1.0f, 0,0 };
	//static float Y []= { 0,1.0f,0 };
	//static float Z []= { 0,0,1.0f };
	////0. Compute field at point P, as well as all 3 spatial derivatives (X,Y,Z)
	//lapack_complex_float field_P = propagateFieldToPoint(point, singlePointField, boardSize, pitch);
	//lapack_complex_float derivative_field_X = computeDerivativeOfFieldAtPoint(point, X, singlePointField, boardSize, pitch, delta);
	//lapack_complex_float derivative_field_Y = computeDerivativeOfFieldAtPoint(point, Y, singlePointField, boardSize, pitch, delta);
	//lapack_complex_float derivative_field_Z = computeDerivativeOfFieldAtPoint(point, Z, singlePointField, boardSize, pitch, delta);
	////1. Compute magnitudes squared: 
	//float field_P2 = field_P.real*field_P.real + field_P.imag*field_P.imag;
	//float derivative_field_X2 = derivative_field_X.real*derivative_field_X.real + derivative_field_X.imag*derivative_field_X.imag;
	//float derivative_field_Y2 = derivative_field_Y.real*derivative_field_Y.real + derivative_field_Y.imag*derivative_field_Y.imag;
	//float derivative_field_Z2 = derivative_field_Z.real*derivative_field_Z.real + derivative_field_Z.imag*derivative_field_Z.imag;
	////2. Compute GorKov related constants: 
	//static float K1 = precomputeGorKovK1();
	//static float K2 = precomputeGorKovK2();

	////3. Return GorKov potential
	//return K1*field_P2 - K2*(derivative_field_X2+derivative_field_Y2+derivative_field_Z2);
}

float GorKovComputation::computeGorKovAtPoint(float point[3], lapack_complex_float* singlePointField, const float* transducerPositions, int numTransducers, float delta, float P0) {
	static float X []= { 1.0f, 0,0 };
	static float Y []= { 0,1.0f,0 };
	static float Z []= { 0,0,1.0f };
	//0. Compute field at point P, as well as all 3 spatial derivatives (X,Y,Z)
	lapack_complex_float field_P = propagateFieldToPoint(point, singlePointField, transducerPositions, numTransducers,P0);
	lapack_complex_float derivative_field_X = computeDerivativeOfFieldAtPoint(point, X, singlePointField, transducerPositions, numTransducers, delta,P0);
	lapack_complex_float derivative_field_Y = computeDerivativeOfFieldAtPoint(point, Y, singlePointField, transducerPositions, numTransducers, delta,P0);
	lapack_complex_float derivative_field_Z = computeDerivativeOfFieldAtPoint(point, Z, singlePointField, transducerPositions, numTransducers, delta,P0);
	//1. Compute magnitudes squared: 
	float field_P2 = field_P.real*field_P.real + field_P.imag*field_P.imag;
	float derivative_field_X2 = derivative_field_X.real*derivative_field_X.real + derivative_field_X.imag*derivative_field_X.imag;
	float derivative_field_Y2 = derivative_field_Y.real*derivative_field_Y.real + derivative_field_Y.imag*derivative_field_Y.imag;
	float derivative_field_Z2 = derivative_field_Z.real*derivative_field_Z.real + derivative_field_Z.imag*derivative_field_Z.imag;
	//2. Compute GorKov related constants: 
	static float K1 = precomputeGorKovK1();
	static float K2 = precomputeGorKovK2();

	//3. Return GorKov potential
	return K1*field_P2 - K2*(derivative_field_X2+derivative_field_Y2+derivative_field_Z2);
}

/**
	Computes the derivative of the Gorkov potential field at a given point, relative to a given direction (e.g. axis X,Y,Z, or any other).
	The direction vector MUST BE unitary
	We compute the derivative of the complex field at a given point (required for GorKov).
	To compute the derivatives, we use a 5 point kernell:
		-see http://web.media.mit.edu/~crtaylor/calculator.html
		- Locations (-2,-1,0,1,2), Derivative Order (1)
*/
float GorKovComputation::computeDerivativeOfGorKovAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta, float P0) {
	int w =  boardSize[0], h = boardSize[1];
	float transducerPositions[512*3];
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {		
			int t_index[2]; t_index[0] = i; t_index[1] = j;
			computeTransducerPos_SideBySide(t_index, pitch, &(transducerPositions[3*(w*j+i)]));
		}
	}
	return computeDerivativeOfGorKovAtPoint(point, direction, singlePointField, (const float*)transducerPositions, 512, delta, P0);
	////Compute four points around P, for our central differences.
	//float P_2delta[3] = {	point[0]+2*delta*direction[0], 
	//						point[1]+2*delta*direction[1], 
	//						point[2]+2*delta*direction[2] };
	//float P_1delta[3] = {	point[0]+delta*direction[0], 
	//						point[1]+delta*direction[1], 
	//						point[2]+delta*direction[2] };
	//float P_neg1delta[3] = {point[0]-delta*direction[0], 
	//						point[1]-delta*direction[1], 
	//						point[2]-delta*direction[2] };
	//float P_neg2delta[3] = {point[0]-2*delta*direction[0], 
	//						point[1]-2*delta*direction[1], 
	//						point[2]-2*delta*direction[2] };
	////1. compute GorKovs at those 4 points:
	//float gorkov_P_2delta =	computeGorKovAtPoint(P_2delta, singlePointField, boardSize, pitch, delta);
	//float gorkov_P_1delta =	computeGorKovAtPoint(P_1delta, singlePointField, boardSize, pitch, delta);
	//float gorkov_P_neg1delta = computeGorKovAtPoint(P_neg1delta, singlePointField, boardSize, pitch, delta);
	//float gorkov_P_neg2delta = computeGorKovAtPoint(P_neg2delta, singlePointField, boardSize, pitch, delta);
	////2. Compute derivative:
	//float result;
	//result= (gorkov_P_neg2delta - 8 * gorkov_P_neg1delta+ 8 * gorkov_P_1delta- gorkov_P_2delta)/(12*delta);
	//return result;
}

float GorKovComputation::computeDerivativeOfGorKovAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField, const float* transducerPositions, int numTransducers, float delta, float P0) {
	//Compute four points around P, for our central differences.
	float P_2delta[3] = {	point[0]+2*delta*direction[0], 
							point[1]+2*delta*direction[1], 
							point[2]+2*delta*direction[2] };
	float P_1delta[3] = {	point[0]+delta*direction[0], 
							point[1]+delta*direction[1], 
							point[2]+delta*direction[2] };
	float P_neg1delta[3] = {point[0]-delta*direction[0], 
							point[1]-delta*direction[1], 
							point[2]-delta*direction[2] };
	float P_neg2delta[3] = {point[0]-2*delta*direction[0], 
							point[1]-2*delta*direction[1], 
							point[2]-2*delta*direction[2] };
	//1. compute GorKovs at those 4 points:
	float gorkov_P_2delta =	computeGorKovAtPoint(P_2delta, singlePointField, transducerPositions, numTransducers, delta);
	float gorkov_P_1delta =	computeGorKovAtPoint(P_1delta, singlePointField, transducerPositions, numTransducers, delta);
	float gorkov_P_neg1delta = computeGorKovAtPoint(P_neg1delta, singlePointField, transducerPositions, numTransducers, delta);
	float gorkov_P_neg2delta = computeGorKovAtPoint(P_neg2delta, singlePointField, transducerPositions, numTransducers, delta);
	//2. Compute derivative:
	float result;
	result= (gorkov_P_neg2delta - 8 * gorkov_P_neg1delta+ 8 * gorkov_P_1delta- gorkov_P_2delta)/(12*delta);
	return result;
}


/**
	Computes forces as the gradient of Gorkov potential along each direction (Force = - Nabla dot Gorkov)
*/
void GorKovComputation::computeAcousticForceAtPoint(float* out_force, float point[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta, float P0 ) {
	int w =  boardSize[0], h = boardSize[1];
	float transducerPositions[512*3];
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {		
			int t_index[2]; t_index[0] = i; t_index[1] = j;
			computeTransducerPos_SideBySide(t_index, pitch, &(transducerPositions[3*(w*j+i)]));
		}
	}
	computeAcousticForceAtPoint(out_force, point, singlePointField,(const float*) transducerPositions, 512, delta, P0);
	////static float X []= { 1.0f, 0,0 };
	////static float Y []= { 0,1.0f,0 };
	////static float Z []= { 0,0,1.0f };
	//////Return gradient of GorKov along the 3 dimansions:
	////out_force[0] = - computeDerivativeOfGorKovAtPoint(point, X, singlePointField, boardSize, pitch, delta);
	////out_force[1] = - computeDerivativeOfGorKovAtPoint(point, Y, singlePointField, boardSize, pitch, delta);
	////out_force[2] = - computeDerivativeOfGorKovAtPoint(point, Z, singlePointField, boardSize, pitch, delta);
}

void GorKovComputation::computeAcousticForceAtPoint(float* out_force, float point[3], lapack_complex_float* singlePointField, const float* transducerPositions, int numTransducers, float delta, float P0) {
	static float X []= { 1.0f, 0,0 };
	static float Y []= { 0,1.0f,0 };
	static float Z []= { 0,0,1.0f };
	//Return gradient of GorKov along the 3 dimansions:
	out_force[0] = - computeDerivativeOfGorKovAtPoint(point, X, singlePointField, transducerPositions, numTransducers, delta);
	out_force[1] = - computeDerivativeOfGorKovAtPoint(point, Y, singlePointField, transducerPositions, numTransducers, delta);
	out_force[2] = - computeDerivativeOfGorKovAtPoint(point, Z, singlePointField, transducerPositions, numTransducers, delta);
}


void GorKovComputation::computeDerivativeOfAcousticForceAtPoint(float* out_grad_force, float point[3], float direction[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta , float P0) {
	int w =  boardSize[0], h = boardSize[1];
	float transducerPositions[512*3];
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {		
			int t_index[2]; t_index[0] = i; t_index[1] = j;
			computeTransducerPos_SideBySide(t_index, pitch, &(transducerPositions[3*(w*j+i)]));
		}
	}
	computeDerivativeOfAcousticForceAtPoint(out_grad_force, point, direction, singlePointField, (const float*)transducerPositions, 512, delta, P0);
	////Compute  four points around P, for our central differences.
	//float P_2delta[3] = {	point[0]+2*delta*direction[0], 
	//						point[1]+2*delta*direction[1], 
	//						point[2]+2*delta*direction[2] };
	//float P_1delta[3] = {	point[0]+delta*direction[0], 
	//						point[1]+delta*direction[1], 
	//						point[2]+delta*direction[2] };
	//float P_neg1delta[3] = {point[0]-delta*direction[0], 
	//						point[1]-delta*direction[1], 
	//						point[2]-delta*direction[2] };
	//float P_neg2delta[3] = {point[0]-2*delta*direction[0], 
	//						point[1]-2*delta*direction[1], 
	//						point[2]-2*delta*direction[2] };
	////1. compute Acoustic forces at those 4 points:
	//float force_P_2delta[3];
	//float force_P_1delta[3];
	//float force_P_neg1delta[3];
	//float force_P_neg2delta[3];
	//computeAcousticForceAtPoint(force_P_2delta, P_2delta, singlePointField, boardSize, pitch, delta);
	//computeAcousticForceAtPoint(force_P_1delta,P_1delta, singlePointField, boardSize, pitch, delta);
	//computeAcousticForceAtPoint(force_P_neg1delta,P_neg1delta, singlePointField, boardSize, pitch, delta);
	//computeAcousticForceAtPoint(force_P_neg2delta,P_neg2delta, singlePointField, boardSize, pitch, delta);
	////2. Compute derivative:
	//out_grad_force[0] = (force_P_neg2delta[0] - 8 * force_P_neg1delta[0] + 8 * force_P_1delta[0] - force_P_2delta[0]) / (12 * delta);
	//out_grad_force[1]= (force_P_neg2delta[1] - 8 * force_P_neg1delta[1]+ 8 * force_P_1delta[1]- force_P_2delta[1])/(12*delta);
	//out_grad_force[2]= (force_P_neg2delta[2] - 8 * force_P_neg1delta[2]+ 8 * force_P_1delta[2]- force_P_2delta[2])/(12*delta);
}


void GorKovComputation::computeDerivativeOfAcousticForceAtPoint(float* out_grad_force, float point[3], float direction[3], lapack_complex_float* singlePointField, const float* transducerPositions, int numTransducers, float delta, float P0) {
	//Compute  four points around P, for our central differences.
	float P_2delta[3] = {	point[0]+2*delta*direction[0], 
							point[1]+2*delta*direction[1], 
							point[2]+2*delta*direction[2] };
	float P_1delta[3] = {	point[0]+delta*direction[0], 
							point[1]+delta*direction[1], 
							point[2]+delta*direction[2] };
	float P_neg1delta[3] = {point[0]-delta*direction[0], 
							point[1]-delta*direction[1], 
							point[2]-delta*direction[2] };
	float P_neg2delta[3] = {point[0]-2*delta*direction[0], 
							point[1]-2*delta*direction[1], 
							point[2]-2*delta*direction[2] };
	//1. compute Acoustic forces at those 4 points:
	float force_P_2delta[3];
	float force_P_1delta[3];
	float force_P_neg1delta[3];
	float force_P_neg2delta[3];
	computeAcousticForceAtPoint(force_P_2delta, P_2delta, singlePointField,transducerPositions,numTransducers, delta, P0);
	computeAcousticForceAtPoint(force_P_1delta,P_1delta, singlePointField,transducerPositions,numTransducers, delta, P0);
	computeAcousticForceAtPoint(force_P_neg1delta,P_neg1delta, singlePointField,transducerPositions,numTransducers, delta, P0);
	computeAcousticForceAtPoint(force_P_neg2delta,P_neg2delta, singlePointField,transducerPositions,numTransducers, delta, P0);
	//2. Compute derivative:
	out_grad_force[0] = (force_P_neg2delta[0] - 8 * force_P_neg1delta[0] + 8 * force_P_1delta[0] - force_P_2delta[0]) / (12 * delta);
	out_grad_force[1]= (force_P_neg2delta[1] - 8 * force_P_neg1delta[1]+ 8 * force_P_1delta[1]- force_P_2delta[1])/(12*delta);
	out_grad_force[2]= (force_P_neg2delta[2] - 8 * force_P_neg1delta[2]+ 8 * force_P_1delta[2]- force_P_2delta[2])/(12*delta);
}

/**
	To compute the second derivatives, we use a 5 point kernell 
		- see http://web.media.mit.edu/~crtaylor/calculator.html
		- Locations (-2,-1,0,1,2), Derivative Order (2)
*/
float GorKovComputation::computeSecondDerivativeOfGorKovAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta, float P0) {
	int w =  boardSize[0], h = boardSize[1];
	float transducerPositions[512*3];
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {		
			int t_index[2]; t_index[0] = i; t_index[1] = j;
			computeTransducerPos_SideBySide(t_index, pitch, &(transducerPositions[3*(w*j+i)]));
		}
	}
	return computeSecondDerivativeOfGorKovAtPoint(point, direction, singlePointField,(const float*) transducerPositions, 512, delta, P0);
	////Compute four points around P, for our central differences.
	//float P_2delta[3] = {	point[0]+2*delta*direction[0], 
	//						point[1]+2*delta*direction[1], 
	//						point[2]+2*delta*direction[2] };
	//float P_1delta[3] = {	point[0]+delta*direction[0], 
	//						point[1]+delta*direction[1], 
	//						point[2]+delta*direction[2] };
	//float P_neg1delta[3] = {point[0]-delta*direction[0], 
	//						point[1]-delta*direction[1], 
	//						point[2]-delta*direction[2] };
	//float P_neg2delta[3] = {point[0]-2*delta*direction[0], 
	//						point[1]-2*delta*direction[1], 
	//						point[2]-2*delta*direction[2] };
	////1. compute GorKovs at those 5 points:
	//float gorkov_P_2delta =	   computeGorKovAtPoint(P_2delta, singlePointField, boardSize, pitch, delta);
	//float gorkov_P_1delta =	   computeGorKovAtPoint(P_1delta, singlePointField, boardSize, pitch, delta);
	//float gorkov_P		  =    computeGorKovAtPoint(point, singlePointField, boardSize, pitch, delta);
	//float gorkov_P_neg1delta = computeGorKovAtPoint(P_neg1delta, singlePointField, boardSize, pitch, delta);
	//float gorkov_P_neg2delta = computeGorKovAtPoint(P_neg2delta, singlePointField, boardSize, pitch, delta);
	////2. Compute 2nd derivative:
	//float result;
	//result= (-gorkov_P_neg2delta +16 * gorkov_P_neg1delta - 30*gorkov_P + 16 * gorkov_P_1delta- gorkov_P_2delta)/(12*delta*delta);
	//return result;
}

float GorKovComputation::computeSecondDerivativeOfGorKovAtPoint(float point[3], float direction[3], lapack_complex_float* singlePointField, const float* transducerPositions, int numTransducers, float delta, float P0) {
	//Compute four points around P, for our central differences.
	float P_2delta[3] = {	point[0]+2*delta*direction[0], 
							point[1]+2*delta*direction[1], 
							point[2]+2*delta*direction[2] };
	float P_1delta[3] = {	point[0]+delta*direction[0], 
							point[1]+delta*direction[1], 
							point[2]+delta*direction[2] };
	float P_neg1delta[3] = {point[0]-delta*direction[0], 
							point[1]-delta*direction[1], 
							point[2]-delta*direction[2] };
	float P_neg2delta[3] = {point[0]-2*delta*direction[0], 
							point[1]-2*delta*direction[1], 
							point[2]-2*delta*direction[2] };
	//1. compute GorKovs at those 5 points:
	float gorkov_P_2delta =	   computeGorKovAtPoint(P_2delta, singlePointField, transducerPositions, numTransducers, delta, P0);
	float gorkov_P_1delta =	   computeGorKovAtPoint(P_1delta, singlePointField, transducerPositions, numTransducers, delta, P0);
	float gorkov_P		  =    computeGorKovAtPoint(point, singlePointField, transducerPositions, numTransducers, delta, P0);
	float gorkov_P_neg1delta = computeGorKovAtPoint(P_neg1delta, singlePointField, transducerPositions, numTransducers, delta, P0);
	float gorkov_P_neg2delta = computeGorKovAtPoint(P_neg2delta, singlePointField, transducerPositions, numTransducers, delta, P0);
	//2. Compute 2nd derivative:
	float result;
	result= (-gorkov_P_neg2delta +16 * gorkov_P_neg1delta - 30*gorkov_P + 16 * gorkov_P_1delta- gorkov_P_2delta)/(12*delta*delta);
	return result;
}


/*
	Compute Stiffness (minus Laplaccian of Gorkov potential) at a given point. 
	We compute this by adding the second derivatives of GorKov along each direction. 
	To compute the second derivatives, we use a 5 point kernell:
		-see http://web.media.mit.edu/~crtaylor/calculator.html
		- Locations (-2,-1,0,1,2), Derivative Order (2)
*/
void GorKovComputation::computeStiffness(float* out_grad_force,float point[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta, float P0 ) {
	int w =  boardSize[0], h = boardSize[1];
	float transducerPositions[512*3];
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {		
			int t_index[2]; t_index[0] = i; t_index[1] = j;
			computeTransducerPos_SideBySide(t_index, pitch, &(transducerPositions[3*(w*j+i)]));
		}
	}
	computeStiffness(out_grad_force, point, singlePointField, (const float*)transducerPositions, 512, delta, P0);
	//static float X []= { 1.0f, 0,0 };
	//static float Y []= { 0,1.0f,0 };
	//static float Z []= { 0,0,1.0f };
	////Compute 2nd derivative of GorKov along the 3 dimensions:
	//float secondDerixX= - computeSecondDerivativeOfGorKovAtPoint(point, X, singlePointField, boardSize, pitch, delta);
	//float secondDerixY= - computeSecondDerivativeOfGorKovAtPoint(point, Y, singlePointField, boardSize, pitch, delta);
	//float secondDerixZ= - computeSecondDerivativeOfGorKovAtPoint(point, Z, singlePointField, boardSize, pitch, delta);
	////Return sumation:
	////return secondDerixX + secondDerixY + secondDerixZ;
	////return sqrtf(secondDerixX*secondDerixX + secondDerixY*secondDerixY + secondDerixZ*secondDerixZ);
	//out_grad_force[0] = secondDerixX;
	//out_grad_force[1] = secondDerixY;
	//out_grad_force[2] = secondDerixZ;
}

void GorKovComputation::computeStiffness(float* out_grad_force,float point[3], lapack_complex_float* singlePointField, const float* transducerPositions, int numTransducers, float delta, float P0) {
	static float X []= { 1.0f, 0,0 };
	static float Y []= { 0,1.0f,0 };
	static float Z []= { 0,0,1.0f };
	//Compute 2nd derivative of GorKov along the 3 dimensions:
	float secondDerixX= - computeSecondDerivativeOfGorKovAtPoint(point, X, singlePointField, transducerPositions, numTransducers, delta, P0);
	float secondDerixY= - computeSecondDerivativeOfGorKovAtPoint(point, Y, singlePointField, transducerPositions, numTransducers, delta, P0);
	float secondDerixZ= - computeSecondDerivativeOfGorKovAtPoint(point, Z, singlePointField, transducerPositions, numTransducers, delta, P0);
	//Return sumation:
	//return secondDerixX + secondDerixY + secondDerixZ;
	//return sqrtf(secondDerixX*secondDerixX + secondDerixY*secondDerixY + secondDerixZ*secondDerixZ);
	out_grad_force[0] = secondDerixX;
	out_grad_force[1] = secondDerixY;
	out_grad_force[2] = secondDerixZ;
}


void GorKovComputation::computeForceGradients(float* out_grad_force,float point[3], lapack_complex_float* singlePointField, int boardSize[2], float pitch, float delta, float P0 ) {
	int w =  boardSize[0], h = boardSize[1];
	float transducerPositions[512*3];
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {		
			int t_index[2]; t_index[0] = i; t_index[1] = j;
			computeTransducerPos_SideBySide(t_index, pitch, &(transducerPositions[3*(w*j+i)]));
		}
	}
	computeForceGradients(out_grad_force, point, singlePointField,(const float*) transducerPositions, 512, delta, P0);
	//static float X []= { 1.0f, 0,0 };
	//static float Y []= { 0,1.0f,0 };
	//static float Z []= { 0,0,1.0f };
	////Calc gradient of forces along axis
	//float gradForceX[3], gradForceY[3], gradForceZ[3];
	//computeDerivativeOfAcousticForceAtPoint(gradForceX, point, X, singlePointField, boardSize, pitch);
	//computeDerivativeOfAcousticForceAtPoint(gradForceY, point, Y, singlePointField, boardSize, pitch);
	//computeDerivativeOfAcousticForceAtPoint(gradForceZ, point, Z, singlePointField, boardSize, pitch);
	//
	//out_grad_force[0] = gradForceX[0] + gradForceY[0] + gradForceZ[0];
	//out_grad_force[1] = gradForceX[1] + gradForceY[1] + gradForceZ[1];
	//out_grad_force[2] = gradForceX[2] + gradForceY[2] + gradForceZ[2];
	////return sqrtf(sumForces[0] * sumForces[0] + sumForces[1] * sumForces[1] + sumForces[2]*sumForces[2]);
}

void GorKovComputation::computeForceGradients(float* out_grad_force,float point[3], lapack_complex_float* singlePointField, const float* transducerPositions, int numTransducers, float delta, float P0) {
	static float X []= { 1.0f, 0,0 };
	static float Y []= { 0,1.0f,0 };
	static float Z []= { 0,0,1.0f };
	//Calc gradient of forces along axis
	float gradForceX[3], gradForceY[3], gradForceZ[3];
	computeDerivativeOfAcousticForceAtPoint(gradForceX, point, X, singlePointField, transducerPositions, numTransducers, delta, P0);
	computeDerivativeOfAcousticForceAtPoint(gradForceY, point, Y, singlePointField, transducerPositions, numTransducers, delta, P0);
	computeDerivativeOfAcousticForceAtPoint(gradForceZ, point, Z, singlePointField, transducerPositions, numTransducers, delta, P0);
	
	out_grad_force[0] = gradForceX[0] + gradForceY[0] + gradForceZ[0];
	out_grad_force[1] = gradForceX[1] + gradForceY[1] + gradForceZ[1];
	out_grad_force[2] = gradForceX[2] + gradForceY[2] + gradForceZ[2];
	//return sqrtf(sumForces[0] * sumForces[0] + sumForces[1] * sumForces[1] + sumForces[2]*sumForces[2]);
}

