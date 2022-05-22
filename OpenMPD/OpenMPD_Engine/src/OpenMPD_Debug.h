#pragma once
#include <OpenMPD_Prerequisites.h>
#include <OpenMPD.h>
#include <src\BufferManager.h>
#include <sstream>
void debug_ShowPrimitives(cl_command_queue queue, OpenMPD::PrimitiveTargets& targets, cl_mem primitives
							, cl_mem bufferMemory, cl_mem bufferDescriptors, OpenMPD::BufferManager& bm);