#ifndef OpenMPD_BUFFER
#define OpenMPD_BUFFER
#include <OpenMPD_Prerequisites.h>
#include <OpenMPD.h>
namespace OpenMPD {
	class Buffer {	
		friend class BufferManager;
		cl_uint ID;
		cl_uint start, end;
		static int instanceCounter;
		Buffer(cl_uint start, cl_uint end, cl_uint ID) :start(start), end(end),	ID(ID) {
			instanceCounter++;
		}
		virtual ~Buffer() {
			instanceCounter--;
		}
	public:
		bool operator< (const Buffer& other) const {
			return (size() < other.size());
		}
		inline cl_uint getID() const { return ID; }
		inline cl_uint getStart() const { return start; }
		inline cl_uint getEnd() const { return end; }
		inline cl_uint size() const { return end-start; }
		inline bool postAdjacent(Buffer& b) {
			return (b.end == start);
		}
		inline bool preAdjacent(Buffer& b) {
			return b.postAdjacent(*this);
		}
		inline bool adjacent(Buffer& b) {
			return postAdjacent(b) || preAdjacent(b);
		}

		bool split(cl_uint size, Buffer** first, Buffer** second);

		/**
			Creates a new buffer as a result of merging two existing ADJACENT buffers. 
			Fails if buffers are not adjacent. 
			Does not destroy the two buffers merged.
		*/
		static Buffer* merge(Buffer* a, Buffer* b);
	};

}
#endif