#ifndef _PRIMITIVE_UPDATES_LISTENER
#define _PRIMITIVE_UPDATES_LISTENER
#include <include\OpenMPD_Prerequisites.h>

namespace OpenMPD {
	class PrimitiveUpdatesListener {
	public:
		virtual void primitiveDeclared(cl_uint primitiveID, cl_uint posDesc, cl_uint ampDesc, cl_uint startPosIndex, cl_uint endPosIndex) = 0;
		virtual void primitiveUpdate_HighLevel(cl_uint* primitiveIDs, cl_uint numPrimitives, float* mStart, float* mEnds) = 0;
		virtual void primitiveUpdate_LowLevel(cl_uint primitiveID, Primitive& p) = 0;
		virtual void commit() = 0;
		virtual void primitiveReleased(cl_uint primitiveID) = 0;
	};
}
#endif
