// Define GSPAT version
#define _OPEN_MPD_ENGINE_VERSION_MAJOR 1
#define _OPEN_MPD_ENGINE_VERSION_MINOR 0
#define _OPEN_MPD_ENGINE_VERSION_PATCH 0
#define _OPEN_MPD_ENGINE_VERSION_SUFFIX ""
#define _OPEN_MPD_ENGINE_VERSION_NAME "Zoe"

#define _OPEN_MPD_ENGINE_VERSION    ((_OPEN_MPD_ENGINE_VERSION_MAJOR << 16) | (_OPEN_MPD_ENGINE_VERSION_MINOR << 8) | _OPEN_MPD_ENGINE_VERSION_PATCH)
#include <CL/opencl.h>
#include <GSPAT_Solver.h>
#if defined( PBD_ENGINE_NONCLIENT_BUILD )
#    define _OPEN_MPD_ENGINE_Export __declspec( dllexport )
#else
#    if defined( __MINGW32__ )
#        define _OPEN_MPD_ENGINE_Export 
#    else
#        define _OPEN_MPD_ENGINE_Export  __declspec( dllimport )
#    endif
#endif
 
#define OpenMPD_Context_Handler long long
#define OpenMPD_Pointer_Handler long long

//https://stackoverflow.com/questions/1537964/visual-c-equivalent-of-gccs-attribute-packed
////#ifdef __GNUC__
////#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
////#endif
////
////#ifdef _MSC_VER
////#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
////#endif
#ifndef _OPEN_MPD_ENGINE_BASIC_TYPES
#define _OPEN_MPD_ENGINE_BASIC_TYPES

namespace OpenMPD {
	enum GSPAT_SOLVER {NAIVE, IBP, V2, V3, V4, BEM, TS };
	
	/**Only up to MAX_PRIMITIVES can be defined in the system simultaneously*/
	static const cl_uint MAX_PRIMITIVES = 32;
	/**Only up to MAX_BUFFERS can be defined in the system (Amplitude/Positions descriptors)*/
	static const cl_uint MAX_BUFFERS = 256;	
	/**Identifies an invalid primitive (e.g. ran out of IDs, pos/amp descriptors incorrect during declaration, etc.)*/	
	static const cl_uint INVALID_PRIMITIVE_ID = 0;
	/**Identifies an invalid buffer (e.g. ran out of memory, too many buffers declared, etc.)*/	
	static const cl_uint INVALID_BUFFER_ID = 0;
	
	/**Packed structures for interoperability with OpenCL*/
	__pragma( pack(push, 1) )
	struct PrimitiveTargets {
		cl_uint numPrimitivesBeingRendered;
		cl_uint primitiveIDs[MAX_PRIMITIVES];
	} ;
	
	struct Primitive {
		cl_int primitiveID=INVALID_PRIMITIVE_ID;		//Unique identifier for current primitive. 
		//Low-level descriptors for high fps
		cl_int curPosDescriptorID, nextPosDescriptorID;
		cl_int curStartPosIndex, curPosIndex, nextStartPosIndex;
		cl_int curAmpDescriptorID, nextAmpDescriptorID; 
		cl_int curStartAmpIndex, curAmpIndex, nextStartAmpIndex;
		cl_int curColDescriptorID, nextColDescriptorID;
		cl_int curStartColIndex, curColIndex, nextStartColIndex;
		//Highlevel definition of the primitive (position/orientation, low fps)
		cl_float mStart[16] = { 1,0,0,0
							  ,0,1,0,0
							  ,0,0,1,0
							  ,0,0,0,1 };
		cl_float mEnd[16]  = { 1,0,0,0
							  ,0,1,0,0
							  ,0,0,1,0
							  ,0,0,0,1 };
	};

	typedef struct _BufferDescriptor {
			cl_uint start;
			cl_uint numSamplesFloat;
		} BufferDescriptor;
	__pragma(pack(pop))
};
#endif // !_OpenMPD_BASIC_TYPES


