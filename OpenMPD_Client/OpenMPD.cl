#define MAX_PRIMITIVES 32
#define MAX_BUFFERS 256

struct __attribute__ ((packed)) PrimitiveTargets {
	int numPrimitivesBeingRendered;
	int primitiveIDs [MAX_PRIMITIVES];
};

struct __attribute__ ((packed)) Primitive {
	int primitiveID;		//Unique identifier for current primitive. 
	//Low-level descriptors for high fps
	int curPosDescriptorID, nextPosDescriptorID;
	int curStartPosIndex, curPosIndex, nextStartPosIndex;
	int curAmpDescriptorID, nextAmpDescriptorID; 
	int curStartAmpIndex, curAmpIndex, nextStartAmpIndex;
	int curColDescriptorID, nextColDescriptorID;
	int curStartColIndex, curColIndex, nextStartColIndex;
	//Highlevel definition of the primitive (position/orientation, low fps)
	float16 mStart;
	float16 mEnd;
};


struct __attribute__((packed)) BufferDescriptor {
	int start;
	int numSamplesFloat;
};

__kernel void fillDataFromPrimitives(global struct PrimitiveTargets* targets,
									 global struct Primitive* primitives,
									 global struct BufferDescriptor* buffers,
									 global float* memory,
									 global float4* positions,
									 global float* amplitudes,
									 global float2* initialGuessReIm,
									 global float16* mStarts,
									 global float16* mEnds)	
 {
	//0. Get indexes:
	int pointNumber = get_global_id(0);					//We read from targets[pointNumber]		
	int geometry = get_global_id(1);					//This determined the sample to use (current+geometry)
	int numGeometries = get_global_size(0);
	int primitive = targets[0].primitiveIDs[pointNumber]-1;	//Actual Primitive used to render (IDS go from 1..32; indexes from 0..31).
	int pBuffer = primitives[primitive].curPosDescriptorID - 1;//Position buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	int aBuffer = primitives[primitive].curAmpDescriptorID - 1;//Amplitude buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	int pBufferNext = primitives[primitive].nextPosDescriptorID - 1;//Position buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	int aBufferNext = primitives[primitive].nextAmpDescriptorID - 1;//Amplitude buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	int writeOffset = geometry*numGeometries + pointNumber; //Offset where we write to.

	//1. Let's read position from memory
	//a. Decide whether we read using the current descriptor or the next one:
	int curPosDescriptorSize = buffers[pBuffer].numSamplesFloat / 4;
	int nextPosDescriptorSize = buffers[pBufferNext].numSamplesFloat / 4;
	int curEndPosIndex_nonCyclic = primitives[primitive].curStartPosIndex + curPosDescriptorSize;
	int curPosIndex = primitives[primitive].curPosIndex + geometry;
	int curPos_nonCyclic = primitives[primitive].curPosIndex + (primitives[primitive].curPosIndex < primitives[primitive].curStartPosIndex)*curPosDescriptorSize;
	bool nextPosCycle = ((curPos_nonCyclic < curEndPosIndex_nonCyclic) && (curPos_nonCyclic + geometry)  >= curEndPosIndex_nonCyclic);
	//int posSamplesRemainingNextCycle = (curPos_nonCyclic + geometry - primitives[primitive].curStartPosIndex) % curPosDescriptorSize;
	int posSamplesRemainingNextCycle = (curPos_nonCyclic + geometry - primitives[primitive].curStartPosIndex)-curPosDescriptorSize;
	//b. read from memory
	int curPosDescriptorStart =buffers[pBuffer].start;		//offset from memory start
	int nextPosDescriptorStart =buffers[pBufferNext].start;	//offset from memory start
	__global float4* readPosFrom = (__global float4*)(memory 
		+(1-nextPosCycle)*(curPosDescriptorStart + (curPosIndex% curPosDescriptorSize)) 
		+(nextPosCycle)*(nextPosDescriptorStart + (primitives[primitive].nextStartPosIndex + posSamplesRemainingNextCycle)%nextPosDescriptorSize));
	positions[writeOffset] = *readPosFrom;

	//2. Let's read amplitude from memory:
	//a. Decide whether we read using the current descriptor or the next one:
	int curAmpDescriptorSize = buffers[aBuffer].numSamplesFloat ;
	int nextAmpDescriptorSize = buffers[aBufferNext].numSamplesFloat ;
	int curAmpIndex = primitives[primitive].curAmpIndex + geometry;
	int curEndAmpIndex_nonCyclic = primitives[primitive].curStartAmpIndex + curAmpDescriptorSize;
	int curAmp_nonCyclic = primitives[primitive].curAmpIndex + (primitives[primitive].curAmpIndex < primitives[primitive].curStartPosIndex)*curAmpDescriptorSize;
	bool nextAmpCycle = ((curAmp_nonCyclic < curEndAmpIndex_nonCyclic) && (curAmp_nonCyclic + geometry)  >= curEndAmpIndex_nonCyclic);
	//int ampSamplesRemainingNextCycle = (curAmp_nonCyclic + geometry - primitives[primitive].curStartAmpIndex) % curAmpDescriptorSize;
	int ampSamplesRemainingNextCycle = (curAmp_nonCyclic + geometry - primitives[primitive].curStartAmpIndex)-curAmpDescriptorSize;
	//b. read from memory					
	int curAmpDescriptorStart =buffers[aBuffer].start;
	int nextAmpDescriptorStart =buffers[aBufferNext].start;	
	__global float* readAmpFrom = (__global float*)(memory 
		+(1-nextAmpCycle)*(curAmpDescriptorStart + (curAmpIndex%curAmpDescriptorSize)) 
		+(nextAmpCycle)*(nextAmpDescriptorStart + (primitives[primitive].nextStartAmpIndex + ampSamplesRemainingNextCycle)%nextAmpDescriptorSize));
	amplitudes[writeOffset] = *readAmpFrom;
	
	//3. Lets write matrices:
	if (geometry == 0) {
		float aux = pointNumber / 32.0f;
		initialGuessReIm[pointNumber] = (float2)(aux, native_sqrt(1 - aux * aux));
		mStarts[pointNumber] = primitives[primitive].mStart;
		mEnds[pointNumber] = primitives[primitive].mEnd;
		//Update Primitives: This is done in PrimitiveManager::postRender  
		/*//Positions:
		primitives[primitive].curPosIndex = (curPosIndex + numGeometries < curPosDescriptorSize ?
			curPosIndex + numGeometries : (primitives[primitive].nextStartPosIndex+curPosIndex + numGeometries - curPosDescriptorSize) % nextPosDescriptorSize);
		primitives[primitive].curPosDescriptorID = (curPosIndex + numGeometries < curPosDescriptorSize ?
			primitives[primitive].curPosDescriptorID : primitives[primitive].nextPosDescriptorID);
		//Amplitudes:
		primitives[primitive].curAmpIndex = (curAmpIndex + numGeometries < curAmpDescriptorSize ?
			curAmpIndex + numGeometries : (primitives[primitive].nextStartAmpIndex+curAmpIndex + numGeometries - curAmpDescriptorSize) % nextAmpDescriptorSize);
		primitives[primitive].curAmpDescriptorID = (curAmpIndex + numGeometries < curAmpDescriptorSize ?
			primitives[primitive].curAmpDescriptorID : primitives[primitive].nextAmpDescriptorID);*/
	}	
}

__kernel void fillDataFromPrimitives2(struct PrimitiveTargets targets,
									 global struct Primitive* primitives,
									 global struct BufferDescriptor* buffers,
									 global float* memory,
									 global float4* positions,
									 global float* amplitudes,
									 global float2* initialGuessReIm,
									 global float16* mStarts,
									 global float16* mEnds)	
 {
	//0. Get indexes:
	int pointNumber = get_global_id(0);					//We read from targets[pointNumber]		
	int geometry = get_global_id(1);					//This determined the sample to use (current+geometry)
	int numPoints = get_global_size(0);
	int numGeometries = get_global_size(1);
	int primitive = targets.primitiveIDs[pointNumber]-1;	//Actual Primitive used to render (IDS go from 1..32; indexes from 0..31).
	int pBuffer = primitives[primitive].curPosDescriptorID - 1;//Position buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	int aBuffer = primitives[primitive].curAmpDescriptorID - 1;//Amplitude buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	int pBufferNext = primitives[primitive].nextPosDescriptorID - 1;//Position buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	int aBufferNext = primitives[primitive].nextAmpDescriptorID - 1;//Amplitude buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	
	int writeOffset = geometry*numPoints + pointNumber; //Offset where we write to.
	int posBufferStart = buffers[pBuffer].start;
	int ampBufferStart = buffers[aBuffer].start;
	//positions[writeOffset] = (float4)(pointNumber, geometry, posBufferStart, ampBufferStart);
	//1. Let's read position from memory

	__global float4* curPosDescriptorStart =(__global float4*)( memory + buffers[pBuffer].start);
	__global float4* nextPosDescriptorStart =(__global float4*)( memory + buffers[pBufferNext].start);
	int curPosDescriptorSize = buffers[pBuffer].numSamplesFloat / 4;
	int nextPosDescriptorSize = buffers[pBufferNext].numSamplesFloat / 4;
	int curPosIndex = primitives[primitive].curPosIndex + geometry;
	__global float4* readPosFrom = (curPosIndex < curPosDescriptorSize ?
		(__global float4*)(curPosDescriptorStart + curPosIndex) :
		(__global float4*)(nextPosDescriptorStart + (curPosIndex-curPosDescriptorSize)%nextPosDescriptorSize));
	positions[writeOffset] = *readPosFrom;
	
	//2. Let's read amplitude from memory:
	__global float* curAmpDescriptorStart =(__global float*)( memory + buffers[aBuffer].start);
	__global float* nextAmpDescriptorStart =(__global float*)( memory + buffers[aBufferNext].start);
	int curAmpDescriptorSize = buffers[aBuffer].numSamplesFloat ;
	int nextAmpDescriptorSize = buffers[aBufferNext].numSamplesFloat ;
	int curAmpIndex = primitives[primitive].curAmpIndex + geometry;
	__global float* readAmpFrom = (curAmpIndex < curAmpDescriptorSize ?
		(__global float*)(curAmpDescriptorStart + curAmpIndex) :
		(__global float*)(nextAmpDescriptorStart + (curAmpIndex-curAmpDescriptorSize)%nextAmpDescriptorSize));
	amplitudes[writeOffset] = *readAmpFrom;
	
	//3. Lets write matrices:
	if (geometry == 0) {
		float aux = pointNumber / 32.0f;
		initialGuessReIm[pointNumber] = (float2)(aux, native_sqrt(1 - aux * aux));
		mStarts[pointNumber] = primitives[primitive].mStart;
		mEnds[pointNumber] = primitives[primitive].mEnd;
		//Update Primitives: Useless... this updates the chached version of the buffer, but not on the CPU side.
		//Positions:
		//float branchP = (float)(curPosIndex + numGeometries < curPosDescriptorSize);
		//float branchA = (float)(curAmpIndex + numGeometries < curAmpDescriptorSize);
		//primitives[primitive].curPosIndex = 
		//	branchP*(curPosIndex + numGeometries) 
		//	+(1-branchP)* ((primitives[primitive].nextStartPosIndex+(curPosIndex + numGeometries - curPosDescriptorSize)) % nextPosDescriptorSize);
		//primitives[primitive].curPosDescriptorID = 
		//	branchP* primitives[primitive].curPosDescriptorID 
		//	+(1-branchP)* primitives[primitive].nextPosDescriptorID;
		//		
		////Amplitudes:
		//primitives[primitive].curAmpIndex = 
		//	branchA*(curAmpIndex + numGeometries)
		//	+(1-branchA)*((primitives[primitive].nextStartAmpIndex+(curAmpIndex + numGeometries - curAmpDescriptorSize)) % nextAmpDescriptorSize);
		//primitives[primitive].curAmpDescriptorID = 
		//	branchA*(primitives[primitive].curAmpDescriptorID) 
		//	+(1-branchA)*( primitives[primitive].nextAmpDescriptorID);
	}	
}

__kernel void fillDataFromPrimitives3(struct PrimitiveTargets targets,
									 struct Primitive primitives[32],
									 global struct BufferDescriptor* buffers,
									 global float* memory,
									 global float4* positions,
									 global float* amplitudes,
									 global float2* initialGuessReIm,
									 global float16* mStarts,
									 global float16* mEnds)	
 {
	////0. Get indexes:
	//int pointNumber = get_global_id(0);					//We read from targets[pointNumber]		
	//int geometry = get_global_id(1);					//This determined the sample to use (current+geometry)
	//int numPoints = get_global_size(0);
	//int numGeometries = get_global_size(1);
	//int primitive = targets.primitiveIDs[pointNumber]-1;	//Actual Primitive used to render (IDS go from 1..32; indexes from 0..31).
	//int pBuffer = primitives[primitive].curPosDescriptorID - 1;//Position buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	//int aBuffer = primitives[primitive].curAmpDescriptorID - 1;//Amplitude buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	//int pBufferNext = primitives[primitive].nextPosDescriptorID - 1;//Position buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	//int aBufferNext = primitives[primitive].nextAmpDescriptorID - 1;//Amplitude buffer used (IDS go from 1..MAX_BUFFERS; indexes from 0..MAX_BUFFERS-1)
	//
	//int writeOffset = geometry*numPoints + pointNumber; //Offset where we write to.
	//int posBufferStart = buffers[pBuffer].start;
	//int ampBufferStart = buffers[aBuffer].start;
	////positions[writeOffset] = (float4)(pointNumber, geometry, posBufferStart, ampBufferStart);
	////1. Let's read position from memory

	//__global float4* curPosDescriptorStart =(__global float4*)( memory + buffers[pBuffer].start);
	//__global float4* nextPosDescriptorStart =(__global float4*)( memory + buffers[pBufferNext].start);
	//int curPosDescriptorSize = buffers[pBuffer].numSamplesFloat / 4;
	//int nextPosDescriptorSize = buffers[pBufferNext].numSamplesFloat / 4;
	//int curPosIndex = primitives[primitive].curPosIndex + geometry;
	//__global float4* readPosFrom = (curPosIndex < curPosDescriptorSize ?
	//	(__global float4*)(curPosDescriptorStart + curPosIndex) :
	//	(__global float4*)(nextPosDescriptorStart + (curPosIndex-curPosDescriptorSize)%nextPosDescriptorSize));
	//positions[writeOffset] = *readPosFrom;
	//
	////2. Let's read amplitude from memory:
	//__global float* curAmpDescriptorStart =(__global float*)( memory + buffers[aBuffer].start);
	//__global float* nextAmpDescriptorStart =(__global float*)( memory + buffers[aBufferNext].start);
	//int curAmpDescriptorSize = buffers[aBuffer].numSamplesFloat ;
	//int nextAmpDescriptorSize = buffers[aBufferNext].numSamplesFloat ;
	//int curAmpIndex = primitives[primitive].curAmpIndex + geometry;
	//__global float* readAmpFrom = (curAmpIndex < curAmpDescriptorSize ?
	//	(__global float*)(curAmpDescriptorStart + curAmpIndex) :
	//	(__global float*)(nextAmpDescriptorStart + (curAmpIndex-curAmpDescriptorSize)%nextAmpDescriptorSize));
	//amplitudes[writeOffset] = *readAmpFrom;
	//
	////3. Lets write matrices:
	//if (geometry == 0) {
	//	float aux = pointNumber / 32.0f;
	//	initialGuessReIm[pointNumber] = (float2)(aux, native_sqrt(1 - aux * aux));
	//	mStarts[pointNumber] = primitives[primitive].mStart;
	//	mEnds[pointNumber] = primitives[primitive].mEnd;
	//	//Update Primitives: Useless... this updates the chached version of the buffer, but not on the CPU side.
	//	//Positions:
	//	//float branchP = (float)(curPosIndex + numGeometries < curPosDescriptorSize);
	//	//float branchA = (float)(curAmpIndex + numGeometries < curAmpDescriptorSize);
	//	//primitives[primitive].curPosIndex = 
	//	//	branchP*(curPosIndex + numGeometries) 
	//	//	+(1-branchP)* ((primitives[primitive].nextStartPosIndex+(curPosIndex + numGeometries - curPosDescriptorSize)) % nextPosDescriptorSize);
	//	//primitives[primitive].curPosDescriptorID = 
	//	//	branchP* primitives[primitive].curPosDescriptorID 
	//	//	+(1-branchP)* primitives[primitive].nextPosDescriptorID;
	//	//		
	//	////Amplitudes:
	//	//primitives[primitive].curAmpIndex = 
	//	//	branchA*(curAmpIndex + numGeometries)
	//	//	+(1-branchA)*((primitives[primitive].nextStartAmpIndex+(curAmpIndex + numGeometries - curAmpDescriptorSize)) % nextAmpDescriptorSize);
	//	//primitives[primitive].curAmpDescriptorID = 
	//	//	branchA*(primitives[primitive].curAmpDescriptorID) 
	//	//	+(1-branchA)*( primitives[primitive].nextAmpDescriptorID);
	//}	
}
