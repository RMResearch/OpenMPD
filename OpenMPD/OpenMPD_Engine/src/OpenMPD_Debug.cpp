#include <src/OpenMPD_Debug.h>
#include <src/BufferManager.h>
void debug_ShowPrimitives(cl_command_queue queue, OpenMPD::PrimitiveTargets& targets, cl_mem primitives
						 , cl_mem bufferMemory, cl_mem bufferDescriptors, OpenMPD::BufferManager& bm
) {
	std::ostringstream msg;
	msg << "OpenMPD_Debug: Printing current scene:\n";
	//0.Load what is currently held in the GPU:
	OpenMPD::Primitive currentPrimitives[OpenMPD::MAX_PRIMITIVES];
	cl_int err = clEnqueueReadBuffer(queue, primitives, CL_TRUE, 0, OpenMPD::MAX_PRIMITIVES* sizeof(OpenMPD::Primitive), &currentPrimitives, 0, NULL, NULL);
	OpenMPD::BufferDescriptor currentBufferDescriptors[OpenMPD::MAX_BUFFERS];
	err = clEnqueueReadBuffer(queue, bufferDescriptors, CL_TRUE, 0, OpenMPD::MAX_BUFFERS* sizeof(OpenMPD::BufferDescriptor), &currentBufferDescriptors, 0, NULL, NULL);
	//1. Print current scene (Primitives and related data)
	for (cl_uint p = 0; p < targets.numPrimitivesBeingRendered; p++) {
		int curID = targets.primitiveIDs[p];
		int curPosDescID = currentPrimitives[curID - 1].curPosDescriptorID;
		int curAmpDescID = currentPrimitives[curID - 1].curAmpDescriptorID;
		if (curID <1 || curID>OpenMPD::MAX_PRIMITIVES
			|| curPosDescID < 1 || curPosDescID >OpenMPD::MAX_BUFFERS
			|| curAmpDescID < 1 || curAmpDescID >OpenMPD::MAX_BUFFERS) {
			msg << "Error in scene description (Invalid primitive or descriptor ID)\n";
			OpenMPD::printMessage_OpenMPD(msg.str().c_str());
			return;
		}

		msg << "Primitive " << curID << "/"<<currentPrimitives[curID-1].primitiveID<< ":\n";
		msg << "   PosID(" << curPosDescID << ", index:" << currentPrimitives[curID - 1].curPosIndex << "; [" << currentBufferDescriptors[curPosDescID-1].start <<"-"<<(currentBufferDescriptors[curPosDescID-1].start + currentBufferDescriptors[curPosDescID-1].numSamplesFloat)<<"])\n";
		//Show content
		{
		float* bufferContent = new float[currentBufferDescriptors[curPosDescID - 1].numSamplesFloat];
		err = clEnqueueReadBuffer(queue, bufferMemory, CL_TRUE, currentBufferDescriptors[curPosDescID - 1].start* sizeof(float), currentBufferDescriptors[curPosDescID - 1].numSamplesFloat * sizeof(float), bufferContent, 0, NULL, NULL);
		msg << "    Content [";
		for (cl_uint p = 0; p < currentBufferDescriptors[curPosDescID - 1].numSamplesFloat; p++)
			msg << bufferContent[p] << ", ";
		msg << "]\n";
		delete bufferContent;
		}
		msg << "   AmpID(" << curAmpDescID << ", index:" << currentPrimitives[curID - 1].curAmpIndex << "; [" << currentBufferDescriptors[curAmpDescID-1].start <<"-"<<(currentBufferDescriptors[curAmpDescID-1].start + currentBufferDescriptors[curAmpDescID-1].numSamplesFloat)<<"])\n";
				//Show content
		{float* bufferContent = new float[currentBufferDescriptors[curAmpDescID - 1].numSamplesFloat];
		err = clEnqueueReadBuffer(queue, bufferMemory, CL_TRUE, currentBufferDescriptors[curAmpDescID - 1].start* sizeof(float), currentBufferDescriptors[curAmpDescID - 1].numSamplesFloat * sizeof(float), bufferContent, 0, NULL, NULL);
		msg << "    Content [";
		for (cl_uint p = 0; p < currentBufferDescriptors[curAmpDescID - 1].numSamplesFloat; p++)
			msg << bufferContent[p] << ", ";
		msg << "]\n";
		delete bufferContent;
		}
	}
	//2. Print all Buffers allocated: 
	msg << "\nCURRENT BUFFERS IN LOCAL (CPU) - (USED OR NOT):\n";
	for(int b=0;b<OpenMPD::MAX_BUFFERS;b++)
		if (bm.isValid(b + 1)) {
			msg << "   -Buffer" << (b+1) << "(" << bm.getBufferDescriptor(b+1).start << "," << (bm.getBufferDescriptor(b+1).start +bm.getBufferDescriptor(b+1).numSamplesFloat) << ")\n";
		}
	
	msg << "\nCURRENT BUFFERS IN DEVICE (GPU) - (USED OR NOT):\n";
	for(int b=0;b<OpenMPD::MAX_BUFFERS;b++)
		if (bm.isValid(b + 1)) {
			msg << "   -Buffer" << (b+1) << "(" << currentBufferDescriptors[b].start << "," << (currentBufferDescriptors[b].start +currentBufferDescriptors[b].numSamplesFloat) << ")\n";
		}

	OpenMPD::printMessage_OpenMPD(msg.str().c_str());
}

