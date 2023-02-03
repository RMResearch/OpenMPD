//#define NUM_TRANSDUCERS 1024		//We will make this a compile-time argument.
#define NUM_TRANSDUCERS_PER_GROUP 256 //TODO: Make this an argument.
#define MAX_POINTS_PER_GEOMETRY 32
#define NUM_ITERATIONS 20
#define PI  3.14159265359f
#define PI2 6.28318530718f
#define K 726.379761f

#define DELTA 0.0002703125f // lambda / 32
#define K1 7.28326108e-15
#define K2 1.92661948e-20
#define WU 1000000.f
#define WS 100.f

#define IMG_MIN 0.001f
#define IMG_MAX (1.f - 2.f * IMG_MIN)// 0.998f; // = 1 - 2 * img_min


//__constant sampler_t sampleDirTexture = CLK_NORMALIZED_COORDS_TRUE | 
//CLK_ADDRESS_NONE | CLK_FILTER_NEAREST; 
__constant sampler_t sampleDirTexture = CLK_NORMALIZED_COORDS_TRUE |
CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

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
}

__kernel void applyTransformationMatrix(
	global float4* pointPositions,
	global float4* matrixG0,
	global float4* matrixGN
) {
	//0. Get indexes:
	int p_ = get_global_id(0);	//point hologram to create
	int g_ = get_global_id(1);
	int numPoints = get_global_size(0);
	int numGeometries = get_global_size(1);

	int inIndex = g_ * numPoints + p_;
	int outIndex = g_ * numPoints + p_;

	//1. Let's work out the matrix we need to apply:
	float singleGeometry = (float)(numGeometries == 1);
	float interpolationRatio = (1.0f * g_) / (numGeometries - 1 + singleGeometry);
	float4 ourMatrix[4];					//Common buffer to store our local transformation matrix (interpolated from extremes)
	ourMatrix[0] = (1 - interpolationRatio) * matrixG0[p_ * 4 + 0] + interpolationRatio * matrixGN[p_ * 4 + 0];
	ourMatrix[1] = (1 - interpolationRatio) * matrixG0[p_ * 4 + 1] + interpolationRatio * matrixGN[p_ * 4 + 1];
	ourMatrix[2] = (1 - interpolationRatio) * matrixG0[p_ * 4 + 2] + interpolationRatio * matrixGN[p_ * 4 + 2];
	ourMatrix[3] = (1 - interpolationRatio) * matrixG0[p_ * 4 + 3] + interpolationRatio * matrixGN[p_ * 4 + 3];
	//2. Get position of point in world coordinates (using the matrix we computed):
	float4 localPosition = pointPositions[inIndex]; //Local position in the geometry is read from the descriptor
	float4 newPosition = (float4)(
		  dot(ourMatrix[0], localPosition)	//Absolute position computed by multiplying with our matrix.
		, dot(ourMatrix[1], localPosition)
		, dot(ourMatrix[2], localPosition)
		, dot(ourMatrix[3], localPosition));
	pointPositions[outIndex] = newPosition;
}

__kernel void compute_t2pMatrix(
	global float4* transducerPositionsWorld,
	global float4* transducerNormals,
	global float4* positions,
	image2d_t directivity_cos_alpha,
	global float2* t2pMatrix
) {
	//0. Get indexes:
	int t_offset = get_global_id(0);			//coord x of the transducer	
	int point_ = get_global_id(1);				//point hologram to create
	int numTransducers = get_global_size(0);
	uint offset = numTransducers * point_;	//Offset where we write the hologram

	//STAGE 1: Build point hologram 
	//A. Get position of point in world coordinates (using the matrix we computed):
	float4 p_pos = positions[point_];					//Local position in the geometry is read from the descriptor

	//B. Get the position of our transducer 
	float4 t_pos = transducerPositionsWorld[t_offset];
	float4 transducerToPoint = p_pos - t_pos;
	float distance = native_sqrt(transducerToPoint.x * transducerToPoint.x + transducerToPoint.y * transducerToPoint.y + transducerToPoint.z * transducerToPoint.z);
	//This computes cos_alpha NOT ASSUMING transducer normal is (0,0,1)
	float4 t_norm = transducerNormals[t_offset];
	float cos_alpha = fabs((transducerToPoint.x * t_norm.x + transducerToPoint.y * t_norm.y + transducerToPoint.z * t_norm.z) / distance);		

	//c. Sample 1D texture: 
	//float4 amplitude = read_imagef(directivity_cos_alpha, sampleDirTexture, (float2)(cos_alpha, 0.5f)) / distance;
	float4 amplitude = read_imagef(directivity_cos_alpha, sampleDirTexture, (float2)(cos_alpha * IMG_MAX + IMG_MIN, 0.5f)) / distance;
	float Re = amplitude.x * native_cos(K * distance);
	float Im = amplitude.x * native_sin(K * distance);
	//STAGE 2: Building the holograms:
	//a. compute normal propagator (point hologram):	
	t2pMatrix[offset + t_offset] = (float2)(Re, Im);
}

__kernel void compute_m2pMatrix(
	global float4* pointPositions,
	global float4* meshPositions,
	global float* meshAreas,
	global float4* meshNormals,
	global float2* m2pMatrix
) {
	//0. Get indexes:
	int numMeshes = get_global_size(0);
	int mesh_ = get_global_id(0);					//mesh the sound scattered from
	int point_ = get_global_id(1);					//point where to creat a trap		
	uint offset = numMeshes * point_;				//Offset where we write the hologram

	//STAGE 1: Build point hologram from texture
	//A. Get position of point in world coordinates (using the matrix we computed):
	float4 p_pos = pointPositions[point_];			//Local position in the geometry is read from the descriptor

	//B. Get the position and surface area of our mesh
	float4 m_pos = meshPositions[mesh_];
	float m_sur = meshAreas[mesh_];
	float4 m_norm = meshNormals[mesh_];

	//c. Sample texture:
	float4 difference = p_pos - m_pos;
	float distance = native_sqrt(difference.x * difference.x + difference.y * difference.y + difference.z * difference.z);
	float4 unitary = difference / distance; // from mesh to point
	float cosa = unitary.x * m_norm.x + unitary.y * m_norm.y + unitary.z * m_norm.z;

	float notClose = (float)(0.001f < distance);

	float phaseG = distance * K;
	float amplitudeG = -1.f / (4.f * PI * distance);
	float2 G = notClose * amplitudeG * (float2)(native_cos(phaseG), native_sin(phaseG));
	float2 F = m_sur * cosa * (float2)(-1.f / distance, K);

	float Re = G.x * F.x - G.y * F.y;
	float Im = G.x * F.y + G.y * F.x;
	m2pMatrix[offset + mesh_] = (float2)(Re, Im);
}

__kernel void solveWGS(
	int numPoints,
	int numIterations,
	global float2* points_Re_Im,
	global float* amplitudesPerPoint,
	global float2* F,
	global float* finalPhases,
	global float* initTransducerPhases,
	global float* initPointPhases,
	global float* initPointAmplitudes
) {
	int t_ = get_global_id(0); // transducer index
	int g_ = get_global_id(1); // geometry index
	int numTransducers = get_global_size(0);
	int numGeometries = get_global_size(1);

	//Copy propagator value to this transducer, for each point. 
	//Copy points [shared bya all member/transducers in the group](amplitude of initial guess =1)
	float2 _F_z_t[MAX_POINTS_PER_GEOMETRY];
	float2 _points[MAX_POINTS_PER_GEOMETRY]; // not local anymore...
	float a_ref[MAX_POINTS_PER_GEOMETRY];
	for (int z = 0; z < numPoints; z++) {
		_F_z_t[z] = F[(numPoints * g_ + z) * numTransducers + t_];
		a_ref[z] = amplitudesPerPoint[g_ * numPoints + z];
		_points[z] = a_ref[z] * (float2)(1.f, 0.f);// points_Re_Im[numPoints * g_ + z];		//NOTE: All geometries copy the same initial guess, stored at geometry 0. 
	}
	//Transducer contribution to each target point (for iterative step 3);
	__local float2 _totalTransducerContribToPoint[NUM_TRANSDUCERS];

	//ITERATIVE PART OF THE ALGORITHM: 
	for (int iter = 0; iter < numIterations; iter++) {
		float2 transducerState = (float2)(0, 0);
		//1. Back propagate points to local transducer: 
		for (int z = 0; z < numPoints; z++) {
			transducerState += (float2)(_F_z_t[z].x * _points[z].x + _F_z_t[z].y * _points[z].y
				, _F_z_t[z].x * _points[z].y - _F_z_t[z].y * _points[z].x);
		}
		//2. Normalise: 
		float amplitude = native_sqrt(transducerState.x * transducerState.x + transducerState.y * transducerState.y);
		transducerState /= amplitude;

		//3. Forward propagate trasducer contrib to each point:
		for (int z = 0; z < numPoints; z++) {
			_totalTransducerContribToPoint[t_] = (float2)
				(_F_z_t[z].x * transducerState.x - _F_z_t[z].y * transducerState.y
					, _F_z_t[z].x * transducerState.y + _F_z_t[z].y * transducerState.x);
			barrier(CLK_LOCAL_MEM_FENCE);
			//Reduce (add contribution from all transducers)
			for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
				if (t_ < i)
					_totalTransducerContribToPoint[t_] += _totalTransducerContribToPoint[t_ + i];
				barrier(CLK_LOCAL_MEM_FENCE);
			}

			//4. Constraint points (and copy to array of current point states, for next iteration):
			//// A. IBP
			//_points[z] = _totalTransducerContribToPoint[0];
			//_points[z] = a_ref[z] * _points[z] / a_out;
			//if (t_ == 0) points_Re_Im[numPoints * g_ + z] = _points[z];

			// B. WGS
			float a_in = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
			_points[z] = _totalTransducerContribToPoint[0];
			if (t_ == 0) points_Re_Im[numPoints * g_ + z] = _points[z];
			float a_out = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
			_points[z] = (a_ref[z] * a_in / a_out) * _points[z] / a_out;
		}

		// B. WGS
		float a_0 = native_sqrt(_points[0].x * _points[0].x + _points[0].y * _points[0].y);
		for (int z = 0; z < numPoints; z++) { 
			_points[z] /= a_0;
			//if (t_ == 0) points_Re_Im[numPoints * g_ + z] = _points[z];
		}
	}

	// store the previous point phases
	for (int z = 0; z < numPoints; z++) {
		if (g_ == 0) {
			initPointPhases[z] = atan2(_points[z].y, _points[z].x);
			initPointAmplitudes[z] = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
		}
	}

	//RETRIEVE FINAL SOLUTION
	//1. Back propagate: 
	float2 finalTransducerState = (float2)(0, 0);
	for (int z = 0; z < numPoints; z++) {
		finalTransducerState += (float2)(_F_z_t[z].x * _points[z].x + _F_z_t[z].y * _points[z].y
			, _F_z_t[z].x * _points[z].y - _F_z_t[z].y * _points[z].x);
	}
	//2. Constraint transducers amplitude and save:
	float transducerAmplitude = native_sqrt(finalTransducerState.x * finalTransducerState.x + finalTransducerState.y * finalTransducerState.y);
	finalTransducerState /= transducerAmplitude;
	float finalPhase = (atan2(finalTransducerState.y, finalTransducerState.x));
	finalPhases[g_ * NUM_TRANSDUCERS + t_] = finalPhase;
	// store the previous transducer phases
	if (g_ == 0) initTransducerPhases[t_] = finalPhase;
}

//__kernel void solveTemporalWGS(
//	int numPoints,
//	int numGeometries,
//	int numIterations,
//	global float2* points_Re_Im,
//	global float* amplitudesPerPoint,
//	global float2* F,
//	global float* finalPhases,
//	float transDiffThreshold,
//	float pointDiffThreshold,
//	global float* prevTransPhases,
//	global float* prevPointPhases,
//	global float* prevPointAmplitudes
//) {
//	int t_ = get_global_id(0); // transducer index
//	int numTransducers = get_global_size(0);
//	//Transducer contribution to each target point (for iterative step 3);
//	__local float2 _totalTransducerContribToPoint[NUM_TRANSDUCERS];
//
//	float2 _F_z_t[MAX_POINTS_PER_GEOMETRY];
//	float2 _points[MAX_POINTS_PER_GEOMETRY];
//	float a_ref[MAX_POINTS_PER_GEOMETRY];
//	for (int g = 0; g < numGeometries; g++) {
//		//Copy propagator value to this transducer, for each point. 
//		//Copy points [shared bya all member/transducers in the group](amplitude of initial guess =1)
//		for (int z = 0; z < numPoints; z++) {
//			_F_z_t[z] = F[(g * numPoints + z) * numTransducers + t_];
//			a_ref[z] = amplitudesPerPoint[g * numPoints + z];
//			//_points[z] = a_ref[z] * (float2)(1.f, 0.f); //NOTE: All geometries copy the same initial guess, stored at geometry 0. 
//			_points[z] = prevPointAmplitudes[z] * (float2)(native_cos(prevPointPhases[z]), native_sin(prevPointPhases[z])); //NOTE: All geometries copy the same initial guess, stored at geometry 0. 
//		}
//
//		//ITERATIVE PART OF THE ALGORITHM: 
//		for (int iter = 0; iter < numIterations; iter++) {
//			float2 transducerState = (float2)(0, 0);
//			//1. Back propagate points to local transducer: 
//			for (int z = 0; z < numPoints; z++) {
//				transducerState += (float2)(_F_z_t[z].x * _points[z].x + _F_z_t[z].y * _points[z].y
//					, _F_z_t[z].x * _points[z].y - _F_z_t[z].y * _points[z].x);
//			}
//			////2. Normalise: 
//			float amplitude = native_sqrt(transducerState.x * transducerState.x + transducerState.y * transducerState.y);
//			transducerState /= amplitude;
//			//float prevTransPhase = prevTransPhases[t_];
//			//float currTransPhase = atan2(transducerState.y, transducerState.x);
//			//float diffTransPhase = fmod(currTransPhase - prevTransPhase + PI + PI2, PI2) - PI;
//			//float largeTransCase = (float)(diffTransPhase > +transDiffThreshold);
//			//float smallTransCase = (float)(diffTransPhase < -transDiffThreshold);
//			//float clipTransPhase = prevTransPhase +
//			//	(largeTransCase - smallTransCase) * transDiffThreshold +
//			//	(1.f - largeTransCase) * (1.f - smallTransCase) * diffTransPhase;
//			//transducerState = (float2)(native_cos(clipTransPhase), native_sin(clipTransPhase));
//
//			//3. Forward propagate trasducer contrib to each point:
//			for (int z = 0; z < numPoints; z++) {
//				_totalTransducerContribToPoint[t_] = (float2)
//					(_F_z_t[z].x * transducerState.x - _F_z_t[z].y * transducerState.y
//						, _F_z_t[z].x * transducerState.y + _F_z_t[z].y * transducerState.x);
//				barrier(CLK_LOCAL_MEM_FENCE);
//				//Reduce (add contribution from all transducers)
//				for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
//					if (t_ < i)
//						_totalTransducerContribToPoint[t_] += _totalTransducerContribToPoint[t_ + i];
//					barrier(CLK_LOCAL_MEM_FENCE);
//				}
//
//				//4. Constraint points (and copy to array of current point states, for next iteration):
//				//// A. IBP
//				//_points[z] = _totalTransducerContribToPoint[0];
//				//_points[z] = a_ref[z] * _points[z] / a_out;
//				//if (t_ == 0) points_Re_Im[numPoints * g_ + z] = _points[z];
//
//				// B. WGS
//				float a_in = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
//				_points[z] = _totalTransducerContribToPoint[0];
//				if (t_ == 0) points_Re_Im[numPoints * g + z] = _points[z];
//				float a_out = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
//				_points[z] = (a_ref[z] * a_in / a_out) * _points[z] / a_out;
//
//				//// C. TGS
//				//float a_in = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
//				//_points[z] = _totalTransducerContribToPoint[0];
//				//if (t_ == 0) points_Re_Im[numPoints * g + z] = _points[z];
//				//float a_out = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
//				//float prevPointPhase = prevPointPhases[z];
//				//float currPointPhase = atan2(_points[z].y, _points[z].x);
//				//float diffPointPhase = fmod(currPointPhase - prevPointPhase + PI + PI2, PI2) - PI;
//				//float largePointCase = (float)(diffPointPhase > +pointDiffThreshold);
//				//float smallPointCase = (float)(diffPointPhase < -pointDiffThreshold);
//				//float clipPointPhase = prevPointPhase +
//				//	(largePointCase - smallPointCase) * pointDiffThreshold +
//				//	(1.f - largePointCase) * (1.f - smallPointCase) * diffPointPhase;
//				//_points[z] = (a_ref[z] * a_in / a_out) * (float2)(native_cos(clipPointPhase), native_sin(clipPointPhase));
//			}
//
//			// B/C. WGS/TGS
//			float a_0 = native_sqrt(_points[0].x * _points[0].x + _points[0].y * _points[0].y);
//			for (int z = 0; z < numPoints; z++) {
//				_points[z] /= a_0;
//				//if (t_ == 0) points_Re_Im[numPoints * g_ + z] = _points[z];
//			}
//		}
//
//		for (int z = 0; z < numPoints; z++) {
//			prevPointPhases[z] = atan2(_points[z].y, _points[z].x);
//			prevPointAmplitudes[z] = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
//		}
//
//		//RETRIEVE FINAL SOLUTION
//		//1. Back propagate: 
//		float2 finalTransducerState = (float2)(0, 0);
//		for (int z = 0; z < numPoints; z++) {
//			finalTransducerState += (float2)(_F_z_t[z].x * _points[z].x + _F_z_t[z].y * _points[z].y
//				, _F_z_t[z].x * _points[z].y - _F_z_t[z].y * _points[z].x);
//		}
//		//B. WGS
//		//2. Constraint transducers amplitude and save:
//		float transducerAmplitude = native_sqrt(finalTransducerState.x * finalTransducerState.x + finalTransducerState.y * finalTransducerState.y);
//		finalTransducerState /= transducerAmplitude;
//		float finalPhase = (atan2(finalTransducerState.y, finalTransducerState.x));
//		finalPhases[g * NUM_TRANSDUCERS + t_] = finalPhase;
//		prevTransPhases[t_] = finalPhase;
//		 
//		////C. TGS
//		//float finalPhase;
//		//{
//		//	float prevTransPhase = prevTransPhases[t_];
//		//	float currTransPhase = atan2(finalTransducerState.y, finalTransducerState.x);
//		//	float diffTransPhase = fmod(currTransPhase - prevTransPhase + PI + PI2, PI2) - PI;
//		//	float largeTransCase = (float)(diffTransPhase > +transDiffThreshold);
//		//	float smallTransCase = (float)(diffTransPhase < -transDiffThreshold);
//		//	finalPhase = prevTransPhase +
//		//		(largeTransCase - smallTransCase) * transDiffThreshold +
//		//		(1.f - largeTransCase) * (1.f - smallTransCase) * diffTransPhase;
//		//}
//		//finalPhases[g * NUM_TRANSDUCERS + t_] = finalPhase;
//		//prevTransPhases[t_] = finalPhase;
//	}
//}

__kernel void solveTemporalWGS(
	int numPoints,
	int numGeometries,
	int numIterations,
	global float2* points_Re_Im,
	global float* amplitudesPerPoint,
	global float2* F,
	global float* finalPhases,
	float transDiffThreshold,
	float pointDiffThreshold,
	global float* prevTransPhases,
	global float* prevPointPhases,
	global float* prevPointAmplitudes
) {
	int t_ = get_global_id(0); // transducer index
	int numTransducers = get_global_size(0);
	//Transducer contribution to each target point (for iterative step 3);
	__local float2 _totalTransducerContribToPoint[NUM_TRANSDUCERS];

	float2 _F_z_t[MAX_POINTS_PER_GEOMETRY];
	float2 _points[MAX_POINTS_PER_GEOMETRY];
	float a_ref[MAX_POINTS_PER_GEOMETRY];
	float prevTransPhase = prevTransPhases[t_];
	for (int g = 0; g < numGeometries; g++) {
		//Copy propagator value to this transducer, for each point. 
		//Copy points [shared bya all member/transducers in the group](amplitude of initial guess =1)
		for (int z = 0; z < numPoints; z++) {
			_F_z_t[z] = F[(g * numPoints + z) * numTransducers + t_];
			a_ref[z] = amplitudesPerPoint[g * numPoints + z];
			//_points[z] = a_ref[z] * (float2)(1.f, 0.f); //NOTE: All geometries copy the same initial guess, stored at geometry 0. 
			_points[z] = prevPointAmplitudes[z] * (float2)(native_cos(prevPointPhases[z]), native_sin(prevPointPhases[z])); //NOTE: All geometries copy the same initial guess, stored at geometry 0. 
		}

		//ITERATIVE PART OF THE ALGORITHM: 
		for (int iter = 0; iter < numIterations; iter++) {
			float2 transducerState = (float2)(0, 0);
			//1. Back propagate points to local transducer: 
			for (int z = 0; z < numPoints; z++) {
				transducerState += (float2)(_F_z_t[z].x * _points[z].x + _F_z_t[z].y * _points[z].y
					, _F_z_t[z].x * _points[z].y - _F_z_t[z].y * _points[z].x);
			}
			//2. Normalise: 
			//float amplitude = native_sqrt(transducerState.x * transducerState.x + transducerState.y * transducerState.y);
			//transducerState /= amplitude;
			float currTransPhase = atan2(transducerState.y, transducerState.x);
			float diffTransPhase = fmod(currTransPhase - prevTransPhase + PI + PI2, PI2) - PI;
			float largeTransCase = (float)(diffTransPhase > +transDiffThreshold);
			float smallTransCase = (float)(diffTransPhase < -transDiffThreshold);
			float clipTransPhase = prevTransPhase +
				(largeTransCase - smallTransCase) * transDiffThreshold +
				(1.f - largeTransCase) * (1.f - smallTransCase) * diffTransPhase;
			transducerState = (float2)(native_cos(clipTransPhase), native_sin(clipTransPhase));

			//3. Forward propagate trasducer contrib to each point:
			for (int z = 0; z < numPoints; z++) {
				_totalTransducerContribToPoint[t_] = (float2)
					(_F_z_t[z].x * transducerState.x - _F_z_t[z].y * transducerState.y
						, _F_z_t[z].x * transducerState.y + _F_z_t[z].y * transducerState.x);
				barrier(CLK_LOCAL_MEM_FENCE);
				//Reduce (add contribution from all transducers)
				for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
					if (t_ < i)
						_totalTransducerContribToPoint[t_] += _totalTransducerContribToPoint[t_ + i];
					barrier(CLK_LOCAL_MEM_FENCE);
				}

				//4. Constraint points (and copy to array of current point states, for next iteration):
				//// A. IBP
				//_points[z] = _totalTransducerContribToPoint[0];
				//_points[z] = a_ref[z] * _points[z] / a_out;
				//if (t_ == 0) points_Re_Im[numPoints * g_ + z] = _points[z];

				//// B. WGS
				//float a_in = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
				//_points[z] = _totalTransducerContribToPoint[0];
				//if (t_ == 0) points_Re_Im[numPoints * g + z] = _points[z];
				//float a_out = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
				//_points[z] = (a_ref[z] * a_in / a_out) * _points[z] / a_out;

				// C. TGS
				float a_in = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
				_points[z] = _totalTransducerContribToPoint[0];
				if (t_ == 0) points_Re_Im[numPoints * g + z] = _points[z];
				float a_out = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
				float prevPointPhase = prevPointPhases[z];
				float currPointPhase = atan2(_points[z].y, _points[z].x);
				float diffPointPhase = fmod(currPointPhase - prevPointPhase + PI + PI2, PI2) - PI;
				float largePointCase = (float)(diffPointPhase > +pointDiffThreshold);
				float smallPointCase = (float)(diffPointPhase < -pointDiffThreshold);
				float clipPointPhase = prevPointPhase +
					(largePointCase - smallPointCase) * pointDiffThreshold +
					(1.f - largePointCase) * (1.f - smallPointCase) * diffPointPhase;
				_points[z] = (a_ref[z] * a_in / a_out) * (float2)(native_cos(clipPointPhase), native_sin(clipPointPhase));
			}

			// B/C. WGS/TGS
			float a_0 = native_sqrt(_points[0].x * _points[0].x + _points[0].y * _points[0].y);
			for (int z = 0; z < numPoints; z++) {
				_points[z] /= a_0;
				//if (t_ == 0) points_Re_Im[numPoints * g_ + z] = _points[z];
			}
		}

		for (int z = 0; z < numPoints; z++) {
			prevPointPhases[z] = atan2(_points[z].y, _points[z].x);
			prevPointAmplitudes[z] = native_sqrt(_points[z].x * _points[z].x + _points[z].y * _points[z].y);
		}

		//RETRIEVE FINAL SOLUTION
		//1. Back propagate: 
		float2 finalTransducerState = (float2)(0, 0);
		for (int z = 0; z < numPoints; z++) {
			finalTransducerState += (float2)(_F_z_t[z].x * _points[z].x + _F_z_t[z].y * _points[z].y
				, _F_z_t[z].x * _points[z].y - _F_z_t[z].y * _points[z].x);
		}
		////B. WGS
		////2. Constraint transducers amplitude and save:
		//float transducerAmplitude = native_sqrt(finalTransducerState.x * finalTransducerState.x + finalTransducerState.y * finalTransducerState.y);
		//finalTransducerState /= transducerAmplitude;
		//float finalPhase = (atan2(finalTransducerState.y, finalTransducerState.x));
		//finalPhases[g * NUM_TRANSDUCERS + t_] = finalPhase;
		//prevTransPhases[t_] = finalPhase;

		//C. TGS
		float finalPhase;
		{
			//float prevTransPhase = prevTransPhases[t_];
			float currTransPhase = atan2(finalTransducerState.y, finalTransducerState.x);
			float diffTransPhase = fmod(currTransPhase - prevTransPhase + PI + PI2, PI2) - PI;
			float largeTransCase = (float)(diffTransPhase > +transDiffThreshold);
			float smallTransCase = (float)(diffTransPhase < -transDiffThreshold);
			finalPhase = prevTransPhase +
				(largeTransCase - smallTransCase) * transDiffThreshold +
				(1.f - largeTransCase) * (1.f - smallTransCase) * diffTransPhase;
		}
		finalPhases[g * NUM_TRANSDUCERS + t_] = fmod(finalPhase + PI2, PI2);
		prevTransPhase = finalPhase;
	}
	prevTransPhases[t_] = prevTransPhase;
}


__kernel void solveGradientProblemSimplified(
	int numPoints,
	int numIterations,
	global float2* totalMatrices,
	global float* finalPhases,
	global float* finalAmplitudes,
	global float2* finalHolograms
) {
	int t_ = get_global_id(0); // transducer index (usually 0 to 255)
	int g_ = get_global_id(1); // geometry index
	int numTransducers = get_global_size(0);
	int numGeometries = get_global_size(1);

	float2 _F_z_t0[MAX_POINTS_PER_GEOMETRY], _F_z_t1[MAX_POINTS_PER_GEOMETRY];
	for (int z = 0; z < numPoints; z++) {
		_F_z_t0[z] = totalMatrices[(numPoints * g_ + z) * numTransducers + t_];
		_F_z_t1[z] = totalMatrices[(numPoints * (g_ + numGeometries) + z) * numTransducers + t_];
	}

	float currX = finalPhases[g_ * numTransducers + t_];

	float U_j[MAX_POINTS_PER_GEOMETRY], grad_j_t[MAX_POINTS_PER_GEOMETRY];
	//Transducer contribution to each target point (for iterative step 3);
	__local float2 local_points0[NUM_TRANSDUCERS], local_points1[NUM_TRANSDUCERS];
	__local float local_gnorm[NUM_TRANSDUCERS];

	//ITERATIVE PART OF THE ALGORITHM: 
	for (int iter = 0; iter < numIterations; iter++) {
		float2 holo = (float2)(native_cos(currX), native_sin(currX));
		//3. Forward propagate trasducer contrib to each point:
		float U_ave = 0, grad_t_ave = 0;
		for (int j = 0; j < numPoints; j++) {
			float2 P_j_t = (float2)(_F_z_t0[j].x * holo.x - _F_z_t0[j].y * holo.y, _F_z_t0[j].x * holo.y + _F_z_t0[j].y * holo.x);
			float2 Pz_j_t = (float2)(_F_z_t1[j].x * holo.x - _F_z_t1[j].y * holo.y, _F_z_t1[j].x * holo.y + _F_z_t1[j].y * holo.x);
			Pz_j_t = (Pz_j_t - P_j_t) / DELTA;
			local_points0[t_] = P_j_t;
			local_points1[t_] = Pz_j_t;
			barrier(CLK_LOCAL_MEM_FENCE);
			for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
				if (t_ < i && t_ + i < numTransducers) {
					local_points0[t_] += local_points0[t_ + i];
					local_points1[t_] += local_points1[t_ + i];
				}
				barrier(CLK_LOCAL_MEM_FENCE);
			}

			float2 P_j = local_points0[0];
			float2 Pz_j = local_points1[0];
			// Compute cost function
			U_j[j] = WU * (K1 * (P_j.x * P_j.x + P_j.y * P_j.y) - K2 * (Pz_j.x * Pz_j.x + Pz_j.y * Pz_j.y));
			U_ave += U_j[j];
			grad_j_t[j] = WU * (2.f * K1 * (P_j.y * P_j_t.x - P_j.x * P_j_t.y) - 2.f * K2 * (Pz_j.y * Pz_j_t.x - Pz_j.x * Pz_j_t.y));
			grad_t_ave += grad_j_t[j];
		}
		U_ave /= (float)numPoints;
		grad_t_ave /= (float)numPoints;

		//float stdev = 0;
		float gradStd_t = 0;
		for (int j = 0; j < numPoints; j++) {
			//stdev += (U_j[z] - U_ave) * (U_j[z] - U_ave) / (float)numPoints;
			gradStd_t += 2.f * (U_j[j] - U_ave) * (grad_j_t[j] - grad_t_ave) / (float)numPoints;
		}
		//float fx = U_ave * numPoints + wsig * stdev;
		float grad_t = grad_t_ave * numPoints + WS * gradStd_t;
		finalPhases[g_ * numTransducers + t_] = grad_t;

		local_gnorm[t_] = grad_t * grad_t;
		barrier(CLK_LOCAL_MEM_FENCE);
		//Reduce (add contribution from all transducers)
		for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
			if (t_ < i && t_ + i < numTransducers)
				local_gnorm[t_] += local_gnorm[t_ + i];
			barrier(CLK_LOCAL_MEM_FENCE);
		}

		float step = -1.f / sqrt(local_gnorm[0]);
		currX += step * grad_t;
	}

	finalPhases[g_ * numTransducers + t_] = currX;
	finalAmplitudes[g_ * numTransducers + t_] = 1.f;
	finalHolograms[g_ * numTransducers + t_] = (float2)(native_cos(currX), native_sin(currX));
}

__kernel void solveGradientProblemGorkov(
	int numPoints,
	int numIterations,
	global float2* totalMatrices,
	global float* finalPhases,
	global float* finalAmplitudes,
	global float2* finalHolograms
) {
	int t_ = get_global_id(0); // transducer index (usually 0 to 255)
	int g_ = get_global_id(1); // geometry index
	int numTransducers = get_global_size(0);
	int numGeometries = get_global_size(1);

	float2 _F_z_t0[MAX_POINTS_PER_GEOMETRY], _F_z_t1[MAX_POINTS_PER_GEOMETRY], _F_z_t2[MAX_POINTS_PER_GEOMETRY], _F_z_t3[MAX_POINTS_PER_GEOMETRY];
	for (int z = 0; z < numPoints; z++) {
		_F_z_t0[z] = totalMatrices[(numPoints * g_ + z) * numTransducers + t_];
		_F_z_t1[z] = totalMatrices[(numPoints * (g_ + numGeometries) + z) * numTransducers + t_];
		_F_z_t2[z] = totalMatrices[(numPoints * (g_ + 2 * numGeometries) + z) * numTransducers + t_];
		_F_z_t3[z] = totalMatrices[(numPoints * (g_ + 3 * numGeometries) + z) * numTransducers + t_];
	}

	float currX = finalPhases[g_ * numTransducers + t_];

	float U_j[MAX_POINTS_PER_GEOMETRY], grad_j_t[MAX_POINTS_PER_GEOMETRY];
	//Transducer contribution to each target point (for iterative step 3);
	__local float2 local_points0[NUM_TRANSDUCERS], local_points1[NUM_TRANSDUCERS];
	__local float2 local_points2[NUM_TRANSDUCERS], local_points3[NUM_TRANSDUCERS];
	__local float local_gnorm[NUM_TRANSDUCERS];

	//ITERATIVE PART OF THE ALGORITHM: 
	for (int iter = 0; iter < numIterations; iter++) {
		float2 holo = (float2)(native_cos(currX), native_sin(currX));
		//3. Forward propagate trasducer contrib to each point:
		float U_ave = 0, grad_t_ave = 0;
		for (int j = 0; j < numPoints; j++) {
			float2 P_j_t = (float2)(_F_z_t0[j].x * holo.x - _F_z_t0[j].y * holo.y, _F_z_t0[j].x * holo.y + _F_z_t0[j].y * holo.x);
			float2 Pz_j_t = (float2)(_F_z_t1[j].x * holo.x - _F_z_t1[j].y * holo.y, _F_z_t1[j].x * holo.y + _F_z_t1[j].y * holo.x);
			float2 Py_j_t = (float2)(_F_z_t2[j].x * holo.x - _F_z_t2[j].y * holo.y, _F_z_t2[j].x * holo.y + _F_z_t2[j].y * holo.x);
			float2 Px_j_t = (float2)(_F_z_t3[j].x * holo.x - _F_z_t3[j].y * holo.y, _F_z_t3[j].x * holo.y + _F_z_t3[j].y * holo.x);
			Pz_j_t = (Pz_j_t - P_j_t) / DELTA;
			Py_j_t = (Py_j_t - P_j_t) / DELTA;
			Px_j_t = (Px_j_t - P_j_t) / DELTA;
			local_points0[t_] = P_j_t;
			local_points1[t_] = Pz_j_t;
			local_points2[t_] = Py_j_t;
			local_points3[t_] = Px_j_t;
			barrier(CLK_LOCAL_MEM_FENCE);
			for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
				if (t_ < i && t_ + i < numTransducers) {
					local_points0[t_] += local_points0[t_ + i];
					local_points1[t_] += local_points1[t_ + i];
					local_points2[t_] += local_points2[t_ + i];
					local_points3[t_] += local_points3[t_ + i];
				}
				barrier(CLK_LOCAL_MEM_FENCE);
			}

			float2 P_j = local_points0[0];
			float2 Pz_j = local_points1[0];
			float2 Py_j = local_points2[0];
			float2 Px_j = local_points3[0];
			// Compute cost function
			U_j[j] = WU * (K1 * (P_j.x * P_j.x + P_j.y * P_j.y) - K2 * (Pz_j.x * Pz_j.x + Pz_j.y * Pz_j.y + Py_j.x * Py_j.x + Py_j.y * Py_j.y + Px_j.x * Px_j.x + Px_j.y * Px_j.y));
			U_ave += U_j[j];
			grad_j_t[j] = WU * (2.f * K1 * (P_j.y * P_j_t.x - P_j.x * P_j_t.y) - 2.f * K2 * (Pz_j.y * Pz_j_t.x - Pz_j.x * Pz_j_t.y + Py_j.y * Py_j_t.x - Py_j.x * Py_j_t.y + Px_j.y * Px_j_t.x - Px_j.x * Px_j_t.y));
			grad_t_ave += grad_j_t[j];
		}
		U_ave /= (float)numPoints;
		grad_t_ave /= (float)numPoints;

		//float stdev = 0;
		float gradStd_t = 0;
		for (int j = 0; j < numPoints; j++) {
			//stdev += (U_j[z] - U_ave) * (U_j[z] - U_ave) / (float)numPoints;
			gradStd_t += 2.f * (U_j[j] - U_ave) * (grad_j_t[j] - grad_t_ave) / (float)numPoints;
		}
		//float fx = U_ave * numPoints + wsig * stdev;
		float grad_t = grad_t_ave * numPoints + WS * gradStd_t;
		finalPhases[g_ * numTransducers + t_] = grad_t;

		local_gnorm[t_] = grad_t * grad_t;
		barrier(CLK_LOCAL_MEM_FENCE);
		//Reduce (add contribution from all transducers)
		for (int i = NUM_TRANSDUCERS / 2; i > 0; i >>= 1) {
			if (t_ < i && t_ + i < numTransducers)
				local_gnorm[t_] += local_gnorm[t_ + i];
			barrier(CLK_LOCAL_MEM_FENCE);
		}

		float step = -1.f / sqrt(local_gnorm[0]);
		currX += step * grad_t;
	}

	finalPhases[g_ * numTransducers + t_] = currX;
	finalAmplitudes[g_ * numTransducers + t_] = 1.f;
	finalHolograms[g_ * numTransducers + t_] = (float2)(native_cos(currX), native_sin(currX));
}


__kernel void computeVectorB(
	global float4* transducerPositionsWorld,
	global float4* transducerNormals,
	global float4* meshPositions,
	image2d_t directivity_cos_alpha,
	global float2* vectorB
) {
	//0. Get indexes:
	int m_ = get_global_id(0);			//
	int t_ = get_global_id(1);			//coord x of the transducer	
	int numMeshes = get_global_size(0);
	int numTransducers = get_global_size(1);
	uint offset = numMeshes * t_;	//Offset where we write the hologram

	//STAGE 1: Build point hologram 
	//A. Get position of point in world coordinates (using the matrix we computed):
	float4 m_pos = meshPositions[m_];					//Local position in the geometry is read from the descriptor

	//B. Get the position of our transducer 
	float4 t_pos = transducerPositionsWorld[t_];
	float4 transducerToPoint = m_pos - t_pos;
	float distance = native_sqrt(transducerToPoint.x * transducerToPoint.x + transducerToPoint.y * transducerToPoint.y + transducerToPoint.z * transducerToPoint.z);
	//This computes cos_alpha NOT ASSUMING transducer normal is (0,0,1)
	float4 t_norm = transducerNormals[t_];
	float cos_alpha = fabs((transducerToPoint.x * t_norm.x + transducerToPoint.y * t_norm.y + transducerToPoint.z * t_norm.z) / distance);

	//c. Sample 1D texture: 
	float4 amplitude = read_imagef(directivity_cos_alpha, sampleDirTexture, (float2)(cos_alpha * IMG_MAX + IMG_MIN, 0.5f)) / distance;
	float Re = amplitude.x * native_cos(K * distance);
	float Im = amplitude.x * native_sin(K * distance);
	//STAGE 2: Building the holograms:
	//a. compute normal propagator (point hologram):	
	vectorB[offset + m_] = (float2)(Re, Im);
}

__kernel void computeMatrixA(
	int inOffset,
	global float4* meshPositions,
	global float* meshAreas,
	global float4* meshNormals,
	global float2* matrixA
) {
	//0. Get indexes:
	int numMi = get_global_size(0);
	int numMj = get_global_size(1);
	int mi_ = get_global_id(0);					//mesh the sound scattered from
	int mj_ = get_global_id(1) +inOffset;		//point where to creat a trap		
	uint outOffset = numMi * get_global_id(1);	//Offset where we write the hologram

	//STAGE 1: Build point hologram from texture
	//A. Get position of point in world coordinates (using the matrix we computed):
	float4 p_pos = meshPositions[mi_];			//Local position in the geometry is read from the descriptor

	//B. Get the position and surface area of our mesh
	float4 m_pos = meshPositions[mj_];
	float m_sur = meshAreas[mj_];
	float4 m_norm = meshNormals[mj_];

	//c. Sample texture:
	float4 difference = p_pos - m_pos;
	float distance = native_sqrt(difference.x * difference.x + difference.y * difference.y + difference.z * difference.z);
	float4 unitary = difference / distance; // from mesh to point
	float cosa = unitary.x * m_norm.x + unitary.y * m_norm.y + unitary.z * m_norm.z;

	float phaseG = distance * K;
	float amplitudeG = -1.f / (4.f * PI * distance);
	float2 G = -amplitudeG * (float2)(native_cos(phaseG), native_sin(phaseG));
	float2 F = m_sur * cosa * (float2)(-1.f / distance, K);

	float Re = G.x * F.x - G.y * F.y;
	float Im = G.x * F.y + G.y * F.x;

	if (distance < 0.00001f) {
		Re = 0.5f;
		Im = 0.f;
	}
	matrixA[outOffset + mi_] = (float2)(Re, Im);
}
