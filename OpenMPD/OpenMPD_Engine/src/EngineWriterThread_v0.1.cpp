#include <include/PBD_Engine.h>
#include "ThreadEngine_SyncData.h"
#include <src/EngineThreads.h>
#include <Helper\TimeFunctions.h>
#include <OpenCLSolverImpl_Interoperability.h>
#include <src/OpenCLUtilityFunctions.h>
#include "./GSPAT_RenderingEngine/src/BufferManager.h"
#include "./GSPAT_RenderingEngine/src/PrimitiveDescriptors.h"
#include "./GSPAT_RenderingEngine/src/PrimitiveManager.h"
#include "src/PDB_Engine_Debug.h"
//Variables local to the thread:
	ThreadEngine_SyncData* syncData;
	struct OpenCL_ExecutionContext * context;
	cl_program program;
	cl_kernel fillDataFromPrimitives;
	//Forward declaration of thread's subroutines
	bool alocateResourcesWriterThread();
	void waitForWriterUpdateTime(bool& firstTime);
	void runCycleWriterThread(PBDEngine::PrimitiveManager& pm2, PBDEngine::BufferManager& bm);
	void releaseResourcesWriterThread();
	void endWriterThread();

	//Main thread (defined using subroutines above)
	void* engineWritter(void* arg) {
		//1. Initialize engine:
		//PBDEngine::printMessage_PBDEngine("Initialising writer thread.\n");
		syncData = (ThreadEngine_SyncData*)arg;
		GSPAT::Solver* solver = syncData->solver;
		context = (struct OpenCL_ExecutionContext *)solver->getSolverContext();
		if (!alocateResourcesWriterThread()) {
			endWriterThread();
			return NULL;
		}
		//2. Run engine:
		//PBDEngine::printMessage_PBDEngine("Writer thread running.\n");
		PBDEngine::PrimitiveManager& pm2 = PBDEngine::PrimitiveManager::instance();
		PBDEngine::BufferManager& bm = PBDEngine::BufferManager::instance();
		bool firstTime = true;
		while (syncData->running) {
			//1. Acess variables to store input:	
			//PBDEngine::printMessage_PBDEngine("Writer: Run cycle.\n");
			runCycleWriterThread(pm2, bm);
			//0. Wait for update time:
			//PBDEngine::printMessage_PBDEngine("Writer: wait update.\n");			
			waitForWriterUpdateTime(firstTime);
		}
		//Release resources: 
		releaseResourcesWriterThread();
		//Reduce number of running threads before exiting:
		endWriterThread();
		return NULL;
	}
	//Subroutine's implementation:
	bool alocateResourcesWriterThread() {
		//Initialize engine:
		//Create program			
		OpenCLUtilityFunctions::createProgramFromFile("PBDEngine.cl", context->context, context->device, &program);
		cl_int err;
		fillDataFromPrimitives = clCreateKernel(program, "fillDataFromPrimitives2", &err);
		if (err < 0) {
			PBDEngine::printWarning_PBDEngine("Couldn't create kernel\n");
			return false;
		}
		PBDEngine::printMessage_PBDEngine("Writer thread resources allocated successfully\n");
		return true;
	}
	void waitForWriterUpdateTime(bool& firstTime) {
		float timePerUpdateInMilis = 1000.0f / syncData->targetUPS; //this needs to change every time we change dynamicTargetUPS
		static struct timeval prevTime, curTime, prevResetTime;
		float timeSinceLastUpdate;
		if (firstTime) {
			gettimeofday(&prevTime, 0x0);
			prevResetTime = prevTime;
			firstTime = false;
		}
		do {
			gettimeofday(&curTime, 0x0);
			timeSinceLastUpdate = computeTimeElapsedInMilis(prevTime, curTime);
		} while (timeSinceLastUpdate < 0.99f*syncData->numGeometries*timePerUpdateInMilis && timeSinceLastUpdate>=0);
		prevTime = curTime;
		//printf("Write: Batches Per Second = %f; UPS: %d\n",  1000/timeSinceLastUpdate, syncData->targetUPS);

	}
	void runCycleWriterThread(PBDEngine::PrimitiveManager& pm2, PBDEngine::BufferManager& bm) {
		cl_int err = 0;
		cl_mem memory, bufferDescriptors;
		std::vector<cl_event> *committedEvents;
		cl_mem solutionBuffers[5];
		//Retrive values for input variables:
		cl_mem primitives;
		cl_event primitivesUploaded = pm2.getCurrentPrimitives(&primitives);
		PBDEngine::PrimitiveTargets& targets = pm2.getPrimitiveTargets();
		//Production code: USE THIS
		{
			if (targets.numPrimitivesBeingRendered == 0) {
				clReleaseEvent(primitivesUploaded);
				pm2.postRender(syncData->numGeometries);
				return;
			}
			bm.getBufferDescriptors(&memory, &bufferDescriptors, &committedEvents);
			committedEvents->push_back(primitivesUploaded);
		}
		//DEBUG: Print current primitives being rendered (BUFFER EVENTS UNRELIABLE)
		/*{
			bm.getBufferDescriptors(&memory, &bufferDescriptors, &committedEvents);	
			clWaitForEvents(committedEvents->size(), committedEvents->data());
			clWaitForEvents(1, &primitivesUploaded);
			debug_ShowPrimitives(context->queue, targets, primitives, memory, bufferDescriptors, bm);
			if (targets.numPrimitivesBeingRendered == 0) {
				clReleaseEvent(primitivesUploaded);
				for (int e = 0; e < committedEvents->size(); e++) {
					clReleaseEvent(committedEvents->operator[](e));
				}
				pm2.postRender(syncData->numGeometries);
				return;
			}
			else {			
				committedEvents->push_back(primitivesUploaded);
			}

		}*/
		//Create GSPAT Solution
		GSPAT::Solution* solution = syncData->solver->createSolutionExternalData(targets.numPrimitivesBeingRendered, syncData->numGeometries, true, GSPAT::MatrixAlignment::RowMajorAlignment);
		solution->dataBuffers((void*)solutionBuffers);
		//Configure parameters (kernel args)
		err |= clSetKernelArg(fillDataFromPrimitives, 0, sizeof(PBDEngine::PrimitiveTargets), &(targets));
		err |= clSetKernelArg(fillDataFromPrimitives, 1, sizeof(cl_mem), &primitives);
		err |= clSetKernelArg(fillDataFromPrimitives, 2, sizeof(cl_mem), &(bufferDescriptors));
		err |= clSetKernelArg(fillDataFromPrimitives, 3, sizeof(cl_mem), &(memory));
		err |= clSetKernelArg(fillDataFromPrimitives, 4, sizeof(cl_mem), &(solutionBuffers[0]));
		err |= clSetKernelArg(fillDataFromPrimitives, 5, sizeof(cl_mem), &(solutionBuffers[1]));
		err |= clSetKernelArg(fillDataFromPrimitives, 6, sizeof(cl_mem), &(solutionBuffers[2]));
		err |= clSetKernelArg(fillDataFromPrimitives, 7, sizeof(cl_mem), &(solutionBuffers[3]));
		err |= clSetKernelArg(fillDataFromPrimitives, 8, sizeof(cl_mem), &(solutionBuffers[4]));
		if (err < 0) {
			PBDEngine::printMessage_PBDEngine("PBDEngine::Couldn't set kernel arguments to upload solution data");
			pm2.postRender(0);
			return;
		}
		//2. Run kernel and compute solution:
		size_t global_size[2] = { targets.numPrimitivesBeingRendered, syncData->numGeometries };
		size_t local_size[2] = { targets.numPrimitivesBeingRendered, syncData->numGeometries };
		cl_event finishedUpload;
		err |= clEnqueueNDRangeKernel(context->queue, fillDataFromPrimitives, 2, NULL, global_size, local_size, (cl_uint)committedEvents->size(), committedEvents->data(), &(finishedUpload));
		solution->dataExternallyUploadedEvent(&finishedUpload, 1);
		syncData->solver->compute(solution);
		//3. Post-render updates:
		pm2.postRender(syncData->numGeometries);
		bm.releasePendingCommitedCopies(committedEvents);
		if (err < 0) {
			PBDEngine::printMessage_PBDEngine("PBDEngine:: Kernel error uploading external solution data.");
			return;
		}
		//4. Notify reader thread@
		pthread_mutex_lock(&(syncData->mutex_solution_queue));
		syncData->solutions.push_back(solution);
		pthread_mutex_unlock(&(syncData->solution_available));
		pthread_mutex_unlock(&(syncData->mutex_solution_queue));
		Sleep(0);
	}

	void releaseResourcesWriterThread() {
		clReleaseKernel(fillDataFromPrimitives);
		clReleaseProgram(program);
	}

	void endWriterThread() {
		//Finishe this thread
		pthread_mutex_lock(&(syncData->mutex_solution_queue));
		syncData->runningThreads--;
		pthread_mutex_unlock(&(syncData->mutex_solution_queue));
		//Unlock reader thread (in case it is waiting for a solution from us)
		pthread_mutex_unlock(&(syncData->solution_available));
		PBDEngine::printMessage_PBDEngine("Writer thread finished\n");
	}
//};