//#define NUM_TRANSDUCERS 1024		//We will make this a compile-time argument.
#define NUM_TRANSDUCERS_PER_GROUP 256 //TODO: Make this an argument.
#define MAX_POINTS_PER_GEOMETRY 8
#define NUM_ITERATIONS 20
#define PI 3.14159265359f
#define K 726.379761f


__constant sampler_t sampleVolTexture = CLK_NORMALIZED_COORDS_TRUE | 
CLK_ADDRESS_NONE | CLK_FILTER_NEAREST; 

__constant sampler_t sampleDirTexture = CLK_NORMALIZED_COORDS_TRUE | 
CLK_ADDRESS_NONE | CLK_FILTER_NEAREST; 

__kernel void computeFandB(global float4* transducerPositionsWorld,
	global float4* positions,
	global float4* matrixG0,
	global float4* matrixGN,
	int pointsPerGeometry,
	int numGeometries,
	read_only image2d_t directivity_cos_alpha,
	global float2* pointHologram,
	global float2* unitaryPointHologram
) {
	//0. Get indexes:
	int t_x = get_global_id(0);		//coord x of the transducer		
	//int t_y = get_global_id(1);		//coord y of the transducer
	//int t_offset = t_y * 32 + t_x;   //Index of the transducer to sample
	int t_offset = t_x;
	int point_ = get_global_id(2);				//point hologram to create
	uint offset = get_global_size(0) * point_;	//Offset where we write the hologram
	uint CUR_NUM_TRANSDUCERS = get_global_size(0);
	//uint matrix_offset = point_;

	//1. Let's work out the matrix we need to apply:
	int geometry = point_ / pointsPerGeometry;		//Work out cour geometry number
	int pointNumber = point_ % pointsPerGeometry;
	float interpolationRatio = (1.0f * geometry) / numGeometries; // (fmax(numGeometries - 1.0f, 1.0f));//Avoid division by zero if numGeometries=1;
	__local float4 ourMatrix[4];					//Common buffer to store our local transformation matrix (interpolated from extremes)
	if (t_x < 4) {
		ourMatrix[t_x] = (1 - interpolationRatio) * matrixG0[pointNumber * 4 + t_x] + (interpolationRatio)*matrixGN[pointNumber * 4 + t_x];
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	//STAGE 1: Build point hologram 
	//A. Get position of point in world coordinates (using the matrix we computed):
	float4 local_p_pos = positions[point_];					//Local position in the geometry is read from the descriptor
	float4 p_pos = (float4)(dot(ourMatrix[0], local_p_pos)	//Absolute position computed by multiplying with our matrix.
		, dot(ourMatrix[1], local_p_pos)
		, dot(ourMatrix[2], local_p_pos)
		, dot(ourMatrix[3], local_p_pos));
	//DEBUG: Debugging behaviour of matrices: It can stay, it does not break anything.
	//positions[point_] = p_pos;
	//END DEBUG

	//B. Get the position of our transducer 
	float4 t_pos = transducerPositionsWorld[t_offset];
	float4 transducerToPoint = p_pos - t_pos;
	float distance = native_sqrt(transducerToPoint.x * transducerToPoint.x + transducerToPoint.y * transducerToPoint.y + transducerToPoint.z * transducerToPoint.z);
	//This computes cos_alpha ASSUMING transducer normal is (0,0,1); Divide by dist to make unitary vector (normalise). 
	float cos_alpha = fabs((float)(transducerToPoint.z / distance));
	//float4 t_norm = transducerNormals[t_offset];
	//float cos_alpha = fabs((transducerToPoint.x * t_norm.x + transducerToPoint.y * t_norm.y + transducerToPoint.z * t_norm.z) / distance);

	//DEBUG:
	//float Re = cos(-K*distance);// distance; //cos(t_offset*PI/(1024));
	//float Im = sin(-K*distance);// cos_alpha;
	//pointHologram[offset + t_offset] = (float2)(Re , Im );
	//unitaryPointHologram[offset + t_offset] = (float2)(Re, Im);		//Re*/
	//ENDDEBUG																

	//c. Sample 1D texture: 
	float4 amplitude = read_imagef(directivity_cos_alpha, sampleDirTexture, (float2)(cos_alpha, 0.5f)) / distance;
	float cos_kd = native_cos(-K * distance);
	float sin_kd = native_sin(-K * distance);
	float Re = amplitude.x * native_cos(-K * distance);
	float Im = amplitude.x * native_sin(-K * distance);
	//STAGE 2: Building the holograms:
	//a. compute normal propagator (point hologram):	
	pointHologram[offset + t_offset] = (float2)(Re, Im);
	//b. compute "normalised" point hologram (reconstruction amplitude exactly = one Pa)
	__local float amplitude2[NUM_TRANSDUCERS];
	amplitude2[t_x] = Re * Re + Im * Im;//Original version (normalized takes directivity into account)
	//amplitude2[t_x] = amplitude.x;		  //New version: normalized, but phase-only	
	barrier(CLK_LOCAL_MEM_FENCE);
	//a. Reduce (add all elements in hologram)
	for (int i = CUR_NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
		if (t_x < i)
			amplitude2[t_x] += amplitude2[t_x + i];
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	//c. Normalise (divide by sumation of contributions squared... see Long et al) 
	//Original version (normalized takes directivity into account)
	Re /= amplitude2[0]; Im /= amplitude2[0];
	unitaryPointHologram[offset + t_offset] = (float2)(Re, Im);		//Re
	//New version: normalized, but phase-only	
	//unitaryPointHologram[offset + t_offset] = (float2)(cos_kd/amplitude2[0], sin_kd/amplitude2[0]);
	//DEBUG:
	//pointHologram[offset + t_offset] = (float2)(t_offset , amplitude2[0]);
	//unitaryPointHologram[offset + t_offset] = (float2)(Re, Im);	
	//END DEBUG
}

//__kernel void solvePhases_IBP(
//	int numPoints,
//	global float2* points_Re_Im,
//	global float* amplitudesPerPoint,
//	global float2* F,
//	global float* finalHologram_Phases, //this contains the final phases to send to the array (with lev signature, phase only, A=1)
//	global float* finalHologram_Amplitudes, //this contains the final phases to send to the array (with lev signature, phase only, A=1)
//	global float2* finalHologram_ReIm   //this contains the "focussing hologram" (Re and Im parts, no lev signature)
//
//) {
//	//SET-UP ENVIRONMENT: Each CU represents a transducer. The group is for all transducers in a geometry 
//	int transducer = get_local_id(0);
//	int geometry = get_global_id(1);
//	float2 transducerState;
//	//Copy propagator value to this transducer, for each point. 
//	float2 _F_z_t[MAX_POINTS_PER_GEOMETRY];
//	for (int z = 0; z < numPoints; z++)
//		_F_z_t[z] = F[numPoints * geometry * NUM_TRANSDUCERS + z * NUM_TRANSDUCERS + transducer];
//	//Copy points [shared bya all member/transducers in the group](amplitude of initial guess =1)
//	__local float2 _localPoints[MAX_POINTS_PER_GEOMETRY];
//	if (transducer < numPoints)
//		_localPoints[transducer] = amplitudesPerPoint[geometry * numPoints + transducer] * points_Re_Im[transducer];		//NOTE: All geometries copy the same initial guess, stored at geometry 0. 
//	barrier(CLK_LOCAL_MEM_FENCE);
//	//Transducer contribution to each target point (for iterative step 3);
//	__local float2 _totalTransducerContribToPoint[NUM_TRANSDUCERS * MAX_POINTS_PER_GEOMETRY];
//
//	//ITERATIVE PART OF THE ALGORITHM: 
//	for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
//		transducerState = (float2)(0, 0);
//		//1. Back propagate points to local transducer: 
//		for (int z = 0; z < numPoints; z++) {
//			transducerState += (float2)(_F_z_t[z].x * _localPoints[z].x - _F_z_t[z].y * _localPoints[z].y
//				, _F_z_t[z].x * _localPoints[z].y + _F_z_t[z].y * _localPoints[z].x);
//		}
//		//2. Normalise: 
//		float amplitude = native_sqrt(transducerState.x * transducerState.x + transducerState.y * transducerState.y);
//		transducerState /= amplitude;
//		barrier(CLK_LOCAL_MEM_FENCE);
//		//3. Forward propagate trasducer contrib to each point: 
//		for (int z = 0; z < numPoints; z++) {
//			_totalTransducerContribToPoint[z * NUM_TRANSDUCERS + transducer] = (float2)
//				(_F_z_t[z].x * transducerState.x + _F_z_t[z].y * transducerState.y
//					, _F_z_t[z].x * transducerState.y - _F_z_t[z].y * transducerState.x);
//		}
//		barrier(CLK_LOCAL_MEM_FENCE);
//		//Reduce (add contribution from all transducers)
//		for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
//			if (transducer < i)
//				for (int z = 0; z < numPoints; z++)
//					_totalTransducerContribToPoint[z * NUM_TRANSDUCERS + transducer] += _totalTransducerContribToPoint[z * NUM_TRANSDUCERS + transducer + i];
//			barrier(CLK_LOCAL_MEM_FENCE);
//		}
//		//4. Constraint points (and copy to array of current point states, for next iteration): 
//		if (transducer < numPoints) {
//			float amplitude = native_sqrt(_totalTransducerContribToPoint[NUM_TRANSDUCERS * transducer].x * _totalTransducerContribToPoint[NUM_TRANSDUCERS * transducer].x
//				+ _totalTransducerContribToPoint[NUM_TRANSDUCERS * transducer].y * _totalTransducerContribToPoint[NUM_TRANSDUCERS * transducer].y);
//			_localPoints[transducer] = _totalTransducerContribToPoint[NUM_TRANSDUCERS * transducer] * amplitudesPerPoint[geometry * numPoints + transducer] / amplitude;
//		}
//		barrier(CLK_LOCAL_MEM_FENCE);
//		//Make points relative to first point
//		/*if (transducer < numPoints) {
//			_localPoints[transducer] = (float2)	(	_localPoints[transducer].x*_localPoints[0].x + _localPoints[transducer].y*_localPoints[0].y
//												,	_localPoints[transducer].y*_localPoints[0].x - _localPoints[transducer].x*_localPoints[0].y)/amplitudesPerPoint[geometry*numPoints + 0];
//		}*/
//	}
//
//	//RETRIEVE FINAL SOLUTION
//	//1. Back propagate: 
//	float2 finalTransducerState = (float2)(0, 0);
//	for (int z = 0; z < numPoints; z++) {
//		//_localPoints[z] = (float2) (1, 0);
//		finalTransducerState += (float2)(_F_z_t[z].x * _localPoints[z].x - _F_z_t[z].y * _localPoints[z].y
//			, _F_z_t[z].x * _localPoints[z].y + _F_z_t[z].y * _localPoints[z].x);
//	}
//	//2. Constraint transducers amplitude and safe:
//	float transducerAmplitude = native_sqrt(finalTransducerState.x * finalTransducerState.x + finalTransducerState.y * finalTransducerState.y);
//	finalTransducerState /= transducerAmplitude;
//	finalHologram_ReIm[geometry * NUM_TRANSDUCERS + transducer] = finalTransducerState;
//	finalHologram_Phases[geometry * NUM_TRANSDUCERS + transducer] = (atan2(finalTransducerState.y, finalTransducerState.x));
//	finalHologram_Amplitudes[geometry * NUM_TRANSDUCERS + transducer] = 1;
//
//	//DEBUG:
//	/*finalHologram_ReIm[geometry*NUM_TRANSDUCERS + transducer] = (float2)(transducer, geometry);
//		finalHologram_Phases[geometry*NUM_TRANSDUCERS + transducer] = transducer;
//	finalHologram_Amplitudes[geometry*NUM_TRANSDUCERS + transducer] = geometry;*/
//	//finalHologram_Phases[geometry*NUM_TRANSDUCERS + transducer] = _localPoints[transducer % numPoints].x;
//	//finalHologram_Amplitudes[geometry*NUM_TRANSDUCERS + transducer] = _localPoints[transducer % numPoints].y;
//	//END DEBUG
//}

__kernel void solvePhases_IBP(
	int numPoints,
	global float2* points_Re_Im,
	global float* amplitudesPerPoint,
	global float2* F,
	global float* finalHologram_Phases, //this contains the final phases to send to the array (with lev signature, phase only, A=1)
	global float* finalHologram_Amplitudes, //this contains the final phases to send to the array (with lev signature, phase only, A=1)
	global float2* finalHologram_ReIm   //this contains the "focussing hologram" (Re and Im parts, no lev signature)

) {
	//SET-UP ENVIRONMENT: Each CU represents a transducer. The group is for all transducers in a geometry 
	int transducer = get_local_id(0);
	int geometry = get_global_id(1);
	float2 transducerState;
	//Copy propagator value to this transducer, for each point. 
	//Copy points [shared bya all member/transducers in the group](amplitude of initial guess =1)
	float2 _F_z_t[MAX_POINTS_PER_GEOMETRY];
	float2 _localPoints[MAX_POINTS_PER_GEOMETRY]; // not local anymore...
	for (int z = 0; z < numPoints; z++) {
		_F_z_t[z] = F[numPoints * geometry * NUM_TRANSDUCERS + z * NUM_TRANSDUCERS + transducer];
		_localPoints[z] = amplitudesPerPoint[geometry * numPoints + z] * points_Re_Im[z];		//NOTE: All geometries copy the same initial guess, stored at geometry 0. 
	}
	//Transducer contribution to each target point (for iterative step 3);
	__local float2 _totalTransducerContribToPoint[NUM_TRANSDUCERS];

	//ITERATIVE PART OF THE ALGORITHM: 
	for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
		transducerState = (float2)(0, 0);
		//1. Back propagate points to local transducer: 
		for (int z = 0; z < numPoints; z++) {
			transducerState += (float2)(_F_z_t[z].x * _localPoints[z].x - _F_z_t[z].y * _localPoints[z].y
				, _F_z_t[z].x * _localPoints[z].y + _F_z_t[z].y * _localPoints[z].x);
		}
		//2. Normalise: 
		float amplitude = native_sqrt(transducerState.x * transducerState.x + transducerState.y * transducerState.y);
		transducerState /= amplitude;

		for (int z = 0; z < numPoints; z++) {
			//3. Forward propagate trasducer contrib to each point: 
			_totalTransducerContribToPoint[transducer] = (float2)
				(_F_z_t[z].x * transducerState.x + _F_z_t[z].y * transducerState.y
					, _F_z_t[z].x * transducerState.y - _F_z_t[z].y * transducerState.x);
			barrier(CLK_LOCAL_MEM_FENCE);
			//Reduce (add contribution from all transducers)
			for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
				if (transducer < i)
					_totalTransducerContribToPoint[transducer] += _totalTransducerContribToPoint[transducer + i];
				barrier(CLK_LOCAL_MEM_FENCE);
			}

			//4. Constraint points (and copy to array of current point states, for next iteration):
			float2 totalPoint = _totalTransducerContribToPoint[0];
			float amplitude = native_sqrt(totalPoint.x * totalPoint.x + totalPoint.y * totalPoint.y);
			float2 updatedPoint = totalPoint * amplitudesPerPoint[geometry * numPoints + z] / amplitude;
			_localPoints[z] = updatedPoint;
		}
		//Make points relative to first point
		/*if (transducer < numPoints) {
			_localPoints[transducer] = (float2)	(	_localPoints[transducer].x*_localPoints[0].x + _localPoints[transducer].y*_localPoints[0].y
												,	_localPoints[transducer].y*_localPoints[0].x - _localPoints[transducer].x*_localPoints[0].y)/amplitudesPerPoint[geometry*numPoints + 0];
		}*/
	}

	//RETRIEVE FINAL SOLUTION
	//1. Back propagate: 
	float2 finalTransducerState = (float2)(0, 0);
	for (int z = 0; z < numPoints; z++) {
		//_localPoints[z] = (float2) (1, 0);
		finalTransducerState += (float2)(_F_z_t[z].x * _localPoints[z].x - _F_z_t[z].y * _localPoints[z].y
			, _F_z_t[z].x * _localPoints[z].y + _F_z_t[z].y * _localPoints[z].x);
	}
	//2. Constraint transducers amplitude and safe:
	float transducerAmplitude = native_sqrt(finalTransducerState.x * finalTransducerState.x + finalTransducerState.y * finalTransducerState.y);
	finalTransducerState /= transducerAmplitude;
	finalHologram_ReIm[geometry * NUM_TRANSDUCERS + transducer] = finalTransducerState;
	finalHologram_Phases[geometry * NUM_TRANSDUCERS + transducer] = (atan2(finalTransducerState.y, finalTransducerState.x));
	finalHologram_Amplitudes[geometry * NUM_TRANSDUCERS + transducer] = 1;

	//DEBUG:
	//if (transducer == 0)
	//	for (int z = 0; z < numPoints; z++)
	//		points_Re_Im[z] = _localPoints[z];		//NOTE: All geometries copy the same initial guess, stored at geometry 0. 
	/*finalHologram_ReIm[geometry*NUM_TRANSDUCERS + transducer] = (float2)(transducer, geometry);
		finalHologram_Phases[geometry*NUM_TRANSDUCERS + transducer] = transducer;
	finalHologram_Amplitudes[geometry*NUM_TRANSDUCERS + transducer] = geometry;*/
	//finalHologram_Phases[geometry*NUM_TRANSDUCERS + transducer] = _localPoints[transducer % numPoints].x;
	//finalHologram_Amplitudes[geometry*NUM_TRANSDUCERS + transducer] = _localPoints[transducer % numPoints].y;
	//END DEBUG
}

__kernel void discretise(int numDiscreteLevels,
	global float* phasesDataBuffer,
	global float* amplitudesDataBuffer,
	float phaseOnly,
	global float* phaseAdjustPerTransducerNumber,
	global float* amplitudeAdjustPerTransducerNumber,
	global unsigned char* transducerNumberToPIN_ID,
	global unsigned char* messages) {
	//1. Get transducer coordinates and geometry number
	int x = get_global_id(0);	
	int y = get_global_id(1);//useless
	int g = get_global_id(2);
	const int hologramSize = get_global_size(0);//Total number of transducers
	const int numGeometries = get_global_size(2);//Num solutions being computed in parallel.
	//2. Map transducer to  message parameters (message number, PIN index, correction , etc):
	unsigned char PIN_index = transducerNumberToPIN_ID[x];//PIN index in its local board (256 elements)
	unsigned char firstCharFlag = (unsigned char)(PIN_index == 0);
	float phaseHardwareCorrection = phaseAdjustPerTransducerNumber[x];
	float amplitudeHardwareCorrection = amplitudeAdjustPerTransducerNumber[x];
	//3. Read input data: 
	float targetAmplitude = amplitudeHardwareCorrection*amplitudesDataBuffer[hologramSize * g + x];
	float targetPhase =		phasesDataBuffer[hologramSize * g + x];
	//3.A. Discretise Amplitude: 
	float targetDutyCycle = 0.5f*phaseOnly*amplitudeHardwareCorrection + (1 - phaseOnly)*asinpi(targetAmplitude);		//Compute duty cycle, given transducer response.
	unsigned char discretisedA = (unsigned char)(numDiscreteLevels * targetDutyCycle);
	float phaseCorrection = (2 * PI*((numDiscreteLevels / 2 - discretisedA) / 2)) / numDiscreteLevels;
	
	//3.B. Discretise Phase: (we add 2PI to make sure fmod will return a positive phase value.
	float correctedPhase = fmod(targetPhase - phaseHardwareCorrection + phaseCorrection + 2*PI
		, 2 * PI);
	//correctedPhase += (float)(correctedPhase < 0) * 2 * PI;//Add 2PI if negative (without using branches...)--> NOT NEEDED ANY MORE (We add 2PI always)
	unsigned char discretisedPhase = correctedPhase * numDiscreteLevels / (2 * PI);
	//4. Store in the buffer (each message has 256 phases and 256 amplitudes ->512 elements)
	int groupNumber = (x >> 8); //Alternative: do groupNumber = x/NUM_TRANSDUCERS_PER_GROUP 
	int posInMessage = (2 * NUM_TRANSDUCERS_PER_GROUP)*(numGeometries*groupNumber + g) + PIN_index;
	messages[posInMessage ] = discretisedPhase +firstCharFlag * numDiscreteLevels;
	messages[posInMessage + NUM_TRANSDUCERS_PER_GROUP] =discretisedA;

	//DEBUG: Phase only 
	/*unsigned char discretisedA = numDiscreteLevels / 2;
	float correctedPhase = fmod(targetPhase - phaseHardwareCorrection + 2 * PI, 2 * PI);
	float negativePhaseRadians = (float)(correctedPhase< 0);
	unsigned char discretisedPhase = (unsigned char) (correctedPhase * numDiscreteLevels / (2 * PI));
	*/
	//END DEBUG	
	//DEBUG: Check mappings, phase corrections...
	//messages[g * hologramSize * 2 + posInMessage ] = discretisedPhase +firstCharFlag * numDiscreteLevels;;
	//messages[g * hologramSize * 2 + posInMessage +256] =discretisedA;
	//messages[g * hologramSize * 2 + x ] = PIN_index;
	//messages[g * hologramSize * 2 + hologramSize + x ] = (unsigned char) negativePhaseRadians ;
	////messages[g * hologramSize * 2 + hologramSize + x] =(unsigned char)(phaseHardwareCorrection*180/(PI));	
	//messages[g * hologramSize * 2 + hologramSize + x] = ( char)(phaseAdjustPerTransducerNumber[x]*180/(PI));
	//messages[g * hologramSize * 2 + hologramSize + x] = messageNumber;
	////messages[g * hologramSize * 2 + x ] = x;
	////messages[g * hologramSize * 2 + hologramSize + x ] = g;	
	//END DEBUG
}

