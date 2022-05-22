#ifndef _BUFFER_UPDATES_LISTENER
#define _BUFFER_UPDATES_LISTENER
#include "OpenMPD_Engine\include\OpenMPD_Prerequisites.h"

namespace OpenMPD {
	class BufferUpdatesListener {
	public:
		virtual void bufferCreated(cl_uint bufferID, cl_uint start, cl_uint end) = 0;
		virtual void bufferFilled(cl_uint bufferID, float* data, size_t numFloatSamples) = 0;
		virtual void bufferReleased(cl_uint) = 0;
	};
};
#endif
