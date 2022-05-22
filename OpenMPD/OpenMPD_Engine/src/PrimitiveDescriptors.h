#ifndef OpenMPD_PRIMITIVE_DESCRIPTORS
#define OpenMPD_PRIMITIVE_DESCRIPTORS

#include "include/OpenMPD_Prerequisites.h"
#include "src/BufferManager.h" 
#include "src/Buffer.h"

namespace OpenMPD {
	class PositionsDescriptor {
		cl_uint numSamples;
		Buffer& buffer;
		static const cl_int sampleSize = 4;
		static cl_uint instanceCounter;

	public:
		PositionsDescriptor(float* data, cl_uint numPosSamples)
			:numSamples(numPosSamples)
			, buffer (BufferManager::instance().createBuffer(numSamples*sampleSize))
		{
			BufferManager::instance().fillData(buffer.getID(),data);
			instanceCounter++;
		}
		inline cl_uint getID() { return buffer.getID(); }
		~PositionsDescriptor() {
			BufferManager::instance().releaseBuffer(buffer.getID());
			instanceCounter--;

		}
	};

	class AmplitudesDescriptor {
		cl_int numSamples;
		Buffer& buffer;
		static const cl_int sampleSize = 1;
		static cl_uint instanceCounter;

	public:
		AmplitudesDescriptor(float* data, cl_uint numAmpSamples)
			:numSamples(numAmpSamples)
			, buffer(BufferManager::instance().createBuffer((numSamples)*sampleSize))
		{
			BufferManager::instance().fillData(buffer.getID(),data);
			instanceCounter++;
		}
		~AmplitudesDescriptor() {
			BufferManager::instance().releaseBuffer(buffer.getID());
			instanceCounter--;
		}
		inline cl_uint getID() { return buffer.getID(); }
	};

	class ColoursDescriptor {
		cl_uint numSamples;
		Buffer& buffer;
		static const cl_int sampleSize = 4;
		static cl_uint instanceCounter;

	public:
		ColoursDescriptor (float* data, cl_uint numPosSamples)
			:numSamples(numPosSamples)
			, buffer (BufferManager::instance().createBuffer(numSamples*sampleSize))
		{
			BufferManager::instance().fillData(buffer.getID(),data);
			instanceCounter++;
		}
		inline cl_uint getID() { return buffer.getID(); }
		~ColoursDescriptor () {
			BufferManager::instance().releaseBuffer(buffer.getID());
			instanceCounter--;

		}
	};

};
#endif