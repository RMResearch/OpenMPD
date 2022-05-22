#ifndef _OPENCL_SOLVER_INTEROP
#define _OPENCL_SOLVER_INTEROP
#include <CL/opencl.h>
struct OpenCL_ExecutionContext {
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
};

/*struct OpenCL_DataBuffers {
	

};*/

#endif 
