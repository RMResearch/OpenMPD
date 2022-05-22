#pragma once
#include <OpenCLSolverImpl_Interoperability.h>
#include <include/OpenMPD.h>

void declareBuffers(cl_context context, cl_command_queue queue);
size_t getBuffers(cl_mem* vertexBuffer, cl_mem* colourBuffer);
