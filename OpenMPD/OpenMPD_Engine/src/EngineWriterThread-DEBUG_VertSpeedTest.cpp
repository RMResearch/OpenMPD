#include <include/PBD_Engine.h>
#include "ThreadEngine_SyncData.h"
#include <src/EngineThreads.h>
#include <Helper\TimeFunctions.h>
#include <OpenCLSolverImpl_Interoperability.h>
#include <Helper/OpenCLUtilityFunctions.h>
#include "./GSPAT_RenderingEngine/src/BufferManager.h"
#include "./GSPAT_RenderingEngine/src/PrimitiveDescriptors.h"
#include "./GSPAT_RenderingEngine/src/PrimitiveManager.h"
#include "src/PDB_Engine_Debug.h"
#include <Windows.h>
//Variables local to the thread:
	ThreadEngine_SyncData* syncData;
	struct OpenCL_ExecutionContext * context;
	cl_program program;
	cl_kernel fillDataFromPrimitives;
	std::list<EngineWriterListener*> writerListeners;

	//Forward declaration of thread's subroutines
	bool alocateResourcesWriterThread();
	void waitForWriterUpdateTime(bool& firstTime);
	void runCycleWriterThread(PBDEngine::PrimitiveManager& pm2, PBDEngine::BufferManager& bm);
	void releaseResourcesWriterThread();
	void endWriterThread();

	//Main thread (defined using subroutines above)
	void* engineWritter(void* arg) {
		//1. Initialize engine
		//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
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
		//0. notify listener we will now wait until ready to submit another computation 
		std::list<EngineWriterListener*>::iterator it;		
		it = writerListeners.begin();
		for (; it != writerListeners.end(); it++)
			(*it)->waitNextCycle();
		//1. Wait for next computing cycle
		//a. Initialize
		/*static struct timeval prevTime, curTime, prevResetTime;
		if (firstTime) {
			gettimeofday(&prevTime, 0x0);
			prevResetTime = prevTime;
			syncData->lastMissedDeadline_writer = prevTime.tv_sec + prevTime.tv_usec / 1000000.0f;
			firstTime = false;
		}
		//b. Check 
		float timePerUpdateInMilis = (1000.0f*syncData->numGeometries )/ syncData->targetUPS; //this needs recomputing, as targetUPS can change dynamically
		struct timeval cycleDuration, timeDeadline, timeRemaining ;
		createTimeval(&cycleDuration, timePerUpdateInMilis);
		timeval_add(&timeDeadline, prevTime, cycleDuration);
		int tRemaining_usec;
		do {
			gettimeofday(&curTime, 0x0);
			tRemaining_usec = timeval_subtract(&timeRemaining, timeDeadline, curTime);
		} while (tRemaining_usec>0);
		//If we fall behind by more than 0.1 seconds (100K usec), we reset:
		if (tRemaining_usec < -100000) {
			prevTime = curTime;
			syncData->lastMissedDeadline_writer = curTime.tv_sec+curTime.tv_usec/1000000.0f;
		}
		else //This is the usual case. 
			prevTime = timeDeadline;*/
		//printf("Write: Batches Per Second = %f; UPS: %d\n",  1000/timeSinceLastUpdate, syncData->targetUPS);
	}

#include <SpeedTests_PBDEngine/GenerateTestPaths.h>

	void runCycleWriterThread(PBDEngine::PrimitiveManager& pm2, PBDEngine::BufferManager& bm) {
	//DEBUG@ Test paths:
		static bool firstTime = true;
		static float *combinedPath;
		static size_t sizeCombinedPath;
		static size_t index;
		static bool runTest = false;
		if (firstTime) {
			firstTime = false;
			static float* testPath_data, *returnPath_data ;
			static size_t sizeTestPath;
			static size_t sizeReturnPath;
			float A[] = { 0, 0, 0.16f,1 }, B[] = { 0.0f, 0, 0.06f, 1 };//0.2388f/2
			float a0 = 850;		//Acceleration to test in m/s2 (assumed v0=0 for these tests).
			float v0 = 0.05f;	//Return speed (particle returns to origin at this speed, no acceleration).
			sizeTestPath = 4*createLinearTest(A, B, 0, a0, 1.0f / 8000, &testPath_data);
			sizeReturnPath = 4*createLinearTest(B, A, v0, 0, 1.0f / 8000, &returnPath_data);
			//Combine paths in one animation
			combinedPath = new float[sizeTestPath + 4*4000 + sizeReturnPath + 4*4000];
			memcpy(combinedPath, testPath_data, sizeTestPath * sizeof(float));//initial path
			for (int stop = 0; stop < 4000; stop++) {//Half second stop there
				combinedPath[sizeTestPath + 4 * stop + 0] = testPath_data[sizeTestPath - 4];
				combinedPath[sizeTestPath + 4 * stop + 1] = testPath_data[sizeTestPath - 3];
				combinedPath[sizeTestPath + 4 * stop + 2] = testPath_data[sizeTestPath - 2];
				combinedPath[sizeTestPath + 4 * stop + 3] = testPath_data[sizeTestPath - 1];
			}
			memcpy(&(combinedPath[sizeTestPath + 4 * 4000]), returnPath_data, sizeReturnPath * sizeof(float));//initial path
			for (int stop = 0; stop < 4000; stop++) {//Half second stop there
				combinedPath[sizeTestPath + 4 * 4000 + sizeReturnPath + 4 * stop + 0] = returnPath_data[sizeReturnPath - 4];
				combinedPath[sizeTestPath + 4 * 4000 + sizeReturnPath + 4 * stop + 1] = returnPath_data[sizeReturnPath - 3];
				combinedPath[sizeTestPath + 4 * 4000 + sizeReturnPath + 4 * stop + 2] = returnPath_data[sizeReturnPath - 2];
				combinedPath[sizeTestPath + 4 * 4000 + sizeReturnPath + 4 * stop + 3] = returnPath_data[sizeReturnPath - 1];
			}
			sizeCombinedPath = sizeTestPath + 4 * 4000 + sizeReturnPath + 4 * 4000;
			index = sizeCombinedPath - 32 * 4;//Initialize at the end of return path (static position)
			delete testPath_data;
			delete returnPath_data;
		}
		float a1_data[] = {  15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f
							,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f 
							,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f 
							,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f,15000.0f };
		float mStart[] = { 1,0,0,0,
					  0,1,0,0,
					  0,0,1,0,
					  0,0,0,1 };
	//END DEBUG
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
		PBDEngine::PrimitiveTargets& targets = pm2.getPrimitiveTargets();
		if (targets.numPrimitivesBeingRendered == 0) {
				//1. Create an empty solution, turning transducers off.
				solution = syncData->solver->createTransducersOffSolution();				
				//3. Post-render updates
				pm2.postRender(syncData->numGeometries);//Needed to unlock mutex (commit). Does not update anything (no active Primitives)
				clReleaseEvent(primitivesUploaded);				
				//return;
		}
		else {
			bm.getBufferDescriptors(&memory, &bufferDescriptors, &committedEvents);
			committedEvents->push_back(primitivesUploaded);
			//1.Create a GSPAT Solution
//DEBUG TEST PAths:
			//(Remove original code)
			/*solution = syncData->solver->createSolutionExternalData(targets.numPrimitivesBeingRendered, syncData->numGeometries, syncData->phaseOnly, GSPAT::MatrixAlignment::RowMajorAlignment);
			solution->dataBuffers((void*)solutionBuffers);
			//2.Configure parameters (kernel args)
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
			syncData->solver->compute(solution);*/

			//REPLACEMENT CODE:
			solution = syncData->solver->createSolution(1,32,true,&(combinedPath[index]),a1_data, mStart, mStart, GSPAT::MatrixAlignment::RowMajorAlignment);
			syncData->solver->compute(solution);
			if (runTest)
				if (index + 32 * 4 >= sizeCombinedPath)
					index = 0;
				else
					index=index + 32 * 4;
//ENDDEBUG
			//3. Post-render updates:
			pm2.postRender(syncData->numGeometries);
			bm.releasePendingCommitedCopies(committedEvents);
			if (err < 0) {
				PBDEngine::printMessage_PBDEngine("PBDEngine:: Kernel error uploading external solution data.");
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
		//Finishe this thread
		pthread_mutex_lock(&(syncData->mutex_solution_queue));
		syncData->runningThreads--;
		pthread_mutex_unlock(&(syncData->mutex_solution_queue));
		//Unlock reader thread (in case it is waiting for a solution from us)
		pthread_mutex_unlock(&(syncData->solution_available));
		writerListeners.clear();
		PBDEngine::printMessage_PBDEngine("Writer thread finished\n");
	}

	void addEngineWriterListener(EngineWriterListener* e) {
		writerListeners.push_back(e);
	}
	void removeEngineWriterListener(EngineWriterListener* e) {
		writerListeners.remove(e);
	}
//};