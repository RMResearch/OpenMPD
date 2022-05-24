#include <include/OpenMPD.h>
#include "ThreadEngine_SyncData.h"
#include <src/EngineThreads.h>
#include <Helper\TimeFunctions.h>
#include <OpenCLSolverImpl_Interoperability.h>
#include <Helper/OpenCLUtilityFunctions.h>
#include "./OpenMPD_Engine/src/BufferManager.h"
#include "./OpenMPD_Engine/src/PrimitiveDescriptors.h"
#include "./OpenMPD_Engine/src/OpenMPD_Context.h"
#include "src/OpenMPD_Debug.h"
#include <Windows.h>

#include "VisualRender\RenderResources.h"
//Variables local to the thread:
	ThreadEngine_SyncData* syncData;
	struct OpenCL_ExecutionContext * context;
	cl_program program;
	cl_kernel fillDataFromPrimitives;
	std::list<EngineWriterListener*> writerListeners;

	//Forward declaration of thread's subroutines
	bool alocateResourcesWriterThread();
	void waitForWriterUpdateTime(bool& firstTime);
	void runCycleWriterThread(OpenMPD::OpenMPD_Context& pm2, OpenMPD::BufferManager& bm);
	void releaseResourcesWriterThread();
	void endWriterThread();

	//Main thread (defined using subroutines above)
	void* engineWritter(void* arg) {
		//1. Initialize engine
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		//OpenMPD::printMessage_OpenMPD("Initialising writer thread.\n");
		syncData = (ThreadEngine_SyncData*)arg;
		GSPAT::Solver* solver = syncData->solver;
		context = (struct OpenCL_ExecutionContext *)solver->getSolverContext();
		if (!alocateResourcesWriterThread()) {
			endWriterThread();
			return NULL;
		}
		//2. Run engine:
		//OpenMPD::printMessage_OpenMPD("Writer thread running.\n");
		OpenMPD::OpenMPD_Context& pm2 = OpenMPD::OpenMPD_Context::instance();
		OpenMPD::BufferManager& bm = OpenMPD::BufferManager::instance();
		bool firstTime = true;
		while (syncData->running) {
			//1. Acess variables to store input:	
			//OpenMPD::printMessage_OpenMPD("Writer: Run cycle.\n");
			runCycleWriterThread(pm2, bm);
			//0. Wait for update time:
			//OpenMPD::printMessage_OpenMPD("Writer: wait update.\n");
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
		OpenCLUtilityFunctions::createProgramFromFile("OpenMPD.cl", context->context, context->device, &program);
		cl_int err;
		fillDataFromPrimitives = clCreateKernel(program, "fillDataFromPrimitives2", &err);
		if (err < 0) {
			OpenMPD::printWarning_OpenMPD("Couldn't create kernel\n");
			return false;
		}
		OpenMPD::printMessage_OpenMPD("Writer thread resources allocated successfully\n");
		return true;
	}
	void waitForWriterUpdateTime(bool& firstTime) {
		//0. notify listener we will now wait until ready to submit another computation 
		std::list<EngineWriterListener*>::iterator it;		
		it = writerListeners.begin();
		for (; it != writerListeners.end(); it++)
			(*it)->waitNextCycle();
		//1. Wait for next computing cycle
		//a. Initialize
		static struct timeval prevTime, curTime, prevResetTime;
		if (firstTime) {
			gettimeofday(&prevTime, 0x0);
			prevResetTime = prevTime;
			syncData->lastMissedDeadline_writer = prevTime.tv_sec + prevTime.tv_usec / 1000000.0f;
			firstTime = false;
		}
		//b. Check 
		float timePerUpdateInMilis = (1000.0f*syncData->numGeometries )/ syncData->targetUPS; //this needs recomputing, as targetUPS can change dynamically
		struct timeval cycleDuration, timeDeadline, timeRemaining, timeElapsed ;
		createTimeval(&cycleDuration, timePerUpdateInMilis);
		timeval_add(&timeDeadline, prevTime, cycleDuration);
		int tRemaining_usec;
		do {
			gettimeofday(&curTime, 0x0);
			tRemaining_usec = timeval_subtract(&timeRemaining, timeDeadline, curTime);
		} while (tRemaining_usec>0);
		//Report time waited: 
		float tElapsed_usec = timeval_subtract(&timeElapsed, curTime, prevTime);
		//printf("Write: Batches Per Second = %f; UPS: %f\n",  1000000/tElapsed_usec, syncData->targetUPS);
		
		//If we fall behind by more than 0.1 seconds (100K usec), we reset:
		if (tRemaining_usec < -100000) {
			prevTime = curTime;
			syncData->lastMissedDeadline_writer = curTime.tv_sec+curTime.tv_usec/1000000.0f;
		}
		else //This is the usual case. 
			prevTime = timeDeadline;
		//printf("Write: Batches Per Second = %f; UPS: %d\n",  1000/timeSinceLastUpdate, syncData->targetUPS);

	}
	void runCycleWriterThread(OpenMPD::OpenMPD_Context& pm2, OpenMPD::BufferManager& bm) {
		GSPAT::Solution* solution;
		std::list<EngineWriterListener*>::iterator it;		
		it = writerListeners.begin();
		for (; it != writerListeners.end(); it++)
			(*it)->startCycle();
		//0. Retrive values for input variables:
		cl_int err = 0;
		cl_mem memory, bufferDescriptors;
		std::vector<cl_event> *committedEvents;
		cl_mem solutionBuffers[5];
		cl_mem primitives;
		cl_event primitivesUploaded = pm2.getCurrentPrimitives(&primitives);
		OpenMPD::PrimitiveTargets& targets = pm2.getPrimitiveTargets();
		//Check if we need to set a new divider value:
		if (syncData->newDivider) {
			//1. Create an empty solution, turning transducers off.
			solution = syncData->solver->createNewDividerSolution(syncData->newDivider);
			syncData->targetUPS = 40000.0f / syncData->newDivider;
			syncData->newDivider = 0;
			//3. Post-render updates
			pm2.postRender(syncData->numGeometries);//Needed to unlock mutex (commit). Does not update anything (no active Primitives)
			clReleaseEvent(primitivesUploaded);
		}
		//Check if there is nothing to render
		else if (targets.numPrimitivesBeingRendered == 0) {
				//1. Create an empty solution, turning transducers off.
				solution = syncData->solver->createTransducersOffSolution();				
				//3. Post-render updates
				pm2.postRender(syncData->numGeometries);//Needed to unlock mutex (commit). Does not update anything (no active Primitives)
				clReleaseEvent(primitivesUploaded);				
		}
		else {
			bm.getBufferDescriptors(&memory, &bufferDescriptors, &committedEvents);
			committedEvents->push_back(primitivesUploaded);
			//1.Create a GSPAT Solution
			solution = syncData->solver->createSolutionExternalData(targets.numPrimitivesBeingRendered, syncData->numGeometries, syncData->phaseOnly, GSPAT::MatrixAlignment::RowMajorAlignment);
			solution->dataBuffers((void*)solutionBuffers);
			//2.Configure parameters (kernel args)
			err |= clSetKernelArg(fillDataFromPrimitives, 0, sizeof(OpenMPD::PrimitiveTargets), &(targets));
			err |= clSetKernelArg(fillDataFromPrimitives, 1, sizeof(cl_mem), &primitives);
			err |= clSetKernelArg(fillDataFromPrimitives, 2, sizeof(cl_mem), &(bufferDescriptors));
			err |= clSetKernelArg(fillDataFromPrimitives, 3, sizeof(cl_mem), &(memory));
			err |= clSetKernelArg(fillDataFromPrimitives, 4, sizeof(cl_mem), &(solutionBuffers[0]));
			err |= clSetKernelArg(fillDataFromPrimitives, 5, sizeof(cl_mem), &(solutionBuffers[1]));
			err |= clSetKernelArg(fillDataFromPrimitives, 6, sizeof(cl_mem), &(solutionBuffers[2]));
			err |= clSetKernelArg(fillDataFromPrimitives, 7, sizeof(cl_mem), &(solutionBuffers[3]));
			err |= clSetKernelArg(fillDataFromPrimitives, 8, sizeof(cl_mem), &(solutionBuffers[4]));
			if (err < 0) {
				OpenMPD::printMessage_OpenMPD("OpenMPD::Couldn't set kernel arguments to upload solution data");
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
				OpenMPD::printMessage_OpenMPD("OpenMPD:: Kernel error uploading external solution data.");
				return;
			}
		}


		//4. Notify reader thread
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
		if (syncData->running)
			syncData->running = false;
		//Finishe this thread
		pthread_mutex_lock(&(syncData->mutex_solution_queue));
		syncData->runningThreads--;
		pthread_mutex_unlock(&(syncData->mutex_solution_queue));
		//Unlock reader thread (in case it is waiting for a solution from us)
		pthread_mutex_unlock(&(syncData->solution_available));
		writerListeners.clear();
		OpenMPD::printMessage_OpenMPD("Writer thread finished\n");
	}

	void addEngineWriterListener(EngineWriterListener* e) {
		writerListeners.push_back(e);
	}
	void removeEngineWriterListener(EngineWriterListener* e) {
		writerListeners.remove(e);
	}
//};