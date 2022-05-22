#ifndef _OPENCL_UTILITY_FUNCTIONS
#define _OPENCL_UTILITY_FUNCTIONS

#include <CL/cl_gl.h>

class OpenCLUtilityFunctions{
public:
static char* read_file(const char* filename, size_t* size) ;
static bool createProgramFromFile(const char* programFile, cl_context context, cl_device_id device, cl_program* out_program, char* compilerOptions=NULL);
};
#endif