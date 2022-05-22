#include "./GSPAT_RenderingEngine/src/BufferManager.h"
#include "./GSPAT_RenderingEngine/src/PrimitiveDescriptors.h"
#include "./GSPAT_RenderingEngine/src/PrimitiveManager.h"
#include <GSPAT_Solver.h>
#include <AsierInho.h>
#include <OpenCLSolverImpl_Interoperability.h>
#include <pthread.h>
#include <deque>
#include <Helper\TimeFunctions.h>
#include <conio.h>
#include <math.h>
#define _USE_MATH_DEFINES

void printMessage_PBDEngine(const char* msg) { printf("%s\n", msg); }
void printError_PBDEngine(const char* msg){ printf("%s\n", msg); }
void printWarning_PBDEngine(const char* msg){ printf("%s\n", msg); }

struct ThreadEngine_SyncData {
	cl_uint numGeometries =32;
	cl_uint targetUPS=1024;
	AsierInho::AsierInhoBoard* driver;
	GSPAT::Solver* solver;
	pthread_mutex_t mutex_solution_queue;
	pthread_mutex_t solution_available;
	std::deque<GSPAT::Solution*> solutions;
	pthread_t readerThread, writerThread;
	cl_uint runningThreads;
	bool running = true;
} ;

cl_uint pri1, pri2;
cl_uint pos1, pos2, circle1, circle2;
cl_uint amp1, amp2;
cl_uint numSamplesCircle = 10240;
GSPAT::Solver* initGSPAT(ThreadEngine_SyncData &threadEngine_SyncData) {
//Create driver and connect to it
	AsierInho::RegisterPrintFuncs(printMessage_PBDEngine, printMessage_PBDEngine, printMessage_PBDEngine);	
	threadEngine_SyncData.driver= AsierInho::createAsierInho();
	//Create solver:
	GSPAT::RegisterPrintFuncs(printMessage_PBDEngine, printMessage_PBDEngine, printMessage_PBDEngine);
	GSPAT::Solver* solver = GSPAT::createSolver(32, 16);
	//while(!driver->connect(AsierInho::BensDesign, 3, 6))
	//	printf("Failed to connect to board.");
	threadEngine_SyncData.driver->connect(AsierInho::BensDesign, 3, 6);
	int mappings[512], phaseDelays[512];
	threadEngine_SyncData.driver->readAdjustments(mappings, phaseDelays);
	solver->setBoardConfig(mappings, phaseDelays);	
	return solver;
}

float* createSampledArc(float origin[3], float p0[3], float angleInRads, cl_uint numSamples) {
	//static float buffer[4 * 8];
	float* buffer = new float[numSamples*4];
	float radius[] = { p0[0] - origin[0], p0[1] - origin[1], p0[2] - origin[2]};
	float curP[4];
	//Fill in all the samples:
	for (int s = 0; s < numSamples; s++) {
		float curAngle = (s*angleInRads) / numSamples;
		curP[0] = cosf(curAngle)*radius[0] - sinf(curAngle)*radius[1] + origin[0];
		curP[1] = sinf(curAngle)*radius[0] + cosf(curAngle)*radius[1] + origin[1];
		curP[2] = origin[2];
		curP[3] = 1;
		memcpy(&(buffer[4 * s]), curP, 4 * sizeof(float));
	}
	return buffer;
}

void declareContent(GSPAT::Solver* solver) {
	struct OpenCL_ExecutionContext * context = (struct OpenCL_ExecutionContext *)solver->getSolverContext();
	PBDEngine::PrimitiveManager::instance().initialize(context, 2000000);//2Mfloats (64MB?)
	//printf("Size of float: %zd\n", sizeof(cl_float));
	float p1_data[] = {  0.02f,0,0.1f,1, };
	float p2_data[] = { -0.02f,0,0.1f,1, };
	float origin[] = { 0,0,0.1f }, startPoint1[] = { 0.02f,0,0.1f }, startPoint2[] = { -0.02f,0,0.1f };
	float* circle_data1 = createSampledArc(origin, startPoint1, 2 * 3.14159265f, numSamplesCircle);
	float* circle_data2 = createSampledArc(origin, startPoint2, 2 * 3.14159265f, numSamplesCircle);

	float a1_data[] = { 1.0f, 1, 1}, a2_data[] = { 1.0f,1,1};
	PBDEngine::IPrimitiveUpdater& pm=PBDEngine::PrimitiveManager::primitiveUpdaterInstance();
	//Create descriptors
	pos1=pm.createPositionsDescriptor(p1_data, 1);
	pos2=pm.createPositionsDescriptor(p2_data, 1);
	circle1 = pm.createPositionsDescriptor(circle_data1, numSamplesCircle);
	circle2 = pm.createPositionsDescriptor(circle_data2, numSamplesCircle );
	amp1=pm.createAmplitudesDescriptor(a1_data, 3);
	amp2=pm.createAmplitudesDescriptor(a2_data, 3);
	//Create Primitives
	pri1 = pm.declarePrimitive(pos1, amp1);
	pri2 = pm.declarePrimitive(pos2, amp2);
	pm.commitUpdates();
	pm.setPrimitiveEnabled(pri1,true);
	pm.setPrimitiveEnabled(pri2,true);
	pm.commitUpdates();
	
}

void* monoThreadEngine(void *arg) {
ThreadEngine_SyncData* threadEngine_SyncData = (ThreadEngine_SyncData*)arg;
	GSPAT::Solver* solver = threadEngine_SyncData->solver;
	struct OpenCL_ExecutionContext * context = (struct OpenCL_ExecutionContext *)solver->getSolverContext();
	cl_kernel fillDataFromPrimitives;
	{//Initialize engine:
		//Create program
		cl_program program;
		OpenCLUtilityFunctions::createProgramFromFile("PBDEngine.cl", context->context, context->device, &program);
		cl_int err;
		fillDataFromPrimitives = clCreateKernel(program, "fillDataFromPrimitives2", &err);
		if (err < 0) {
			printWarning_PBDEngine("Couldn't create kernel\n");
		}
	}
	//Run engine:
	PBDEngine::PrimitiveManager& pm2 = PBDEngine::PrimitiveManager::instance();
	PBDEngine::BufferManager& bm = PBDEngine::BufferManager::instance();

	while (threadEngine_SyncData->running){
		//0. Wait for update time:
		bool firstTime = true;
		float timePerUpdateInMilis = 1000.0f/ threadEngine_SyncData->targetUPS; //this needs to change every time we change dynamicTargetUPS
		struct timeval prevTime, curTime, prevResetTime;
		float timeSinceLastUpdate;		
		if (firstTime) {
			gettimeofday(&prevTime, 0x0);
			prevResetTime = prevTime;
			firstTime = false;
		}
		do {
			gettimeofday(&curTime, 0x0);
			timeSinceLastUpdate = computeTimeElapsedInMilis(prevTime, curTime);
		} while (timeSinceLastUpdate <  threadEngine_SyncData->numGeometries*timePerUpdateInMilis && timeSinceLastUpdate>0);

		//1. Acess variables to store input:		
		cl_int err=0;
		cl_mem memory, bufferDescriptors;
		std::vector<cl_event> *committedEvents;
		cl_mem solutionBuffers[5];
		//Retrive values for input variables:
		cl_mem primitives;
		cl_event primitivesUploaded= pm2.getCurrentPrimitives(&primitives);
		PBDEngine::PrimitiveTargets& targets = pm2.getPrimitiveTargets();	
		if(targets.numPrimitivesBeingRendered==0)
			continue;
		bm.getBufferDescriptors(&memory, &bufferDescriptors, &committedEvents);
		committedEvents->push_back(primitivesUploaded);
		//Create GSPAT Solution
		GSPAT::Solution* solution = solver->createSolutionExternalData(targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries, true, GSPAT::MatrixAlignment::RowMajorAlignment);
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
			printMessage_PBDEngine("PBDEngine::Couldn't set kernel arguments to upload solution data"); 
			pm2.postRender(0);
			break; }
		//2. Run kernel and compute solution:
		size_t global_size[2] = { targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries };
		size_t local_size[2] = { targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries };
		cl_event finishedUpload;
		err |= clEnqueueNDRangeKernel(context->queue, fillDataFromPrimitives, 2, NULL, global_size, local_size, committedEvents->size(), committedEvents->data(), &(finishedUpload));
		solution->dataExternallyUploadedEvent(&finishedUpload, 1);
		solver->compute(solution); 
		//3. Post-render updates:
		pm2.postRender(threadEngine_SyncData->numGeometries);
		bm.releasePendingCommitedCopies(committedEvents);
		if (err < 0) { 
			printMessage_PBDEngine("PBDEngine:: Kernel error uploading external solution data."); 
			break; }
		//4. Notify reader thread@
		/*pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
		threadEngine_SyncData->solutions.push_back(solution);
		pthread_mutex_unlock(&(threadEngine_SyncData->solution_available));
		pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));*/
		unsigned char* msgTop, *msgBottom;		
		solution->finalMessages(&msgTop, &msgBottom);
		//threadEngine_SyncData->driver->updateMessage(msgBottom, msgTop, threadEngine_SyncData->numGeometries);
		solver->releaseSolution(solution);		
		
		
	}
	//Reduce number of running threads before exiting:
	pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
	threadEngine_SyncData->runningThreads--;
	printf("Engine thread finished. Num threads remaining %d\n",threadEngine_SyncData->runningThreads );
	pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
	printf("Engine thread finished");

	return NULL; 
	/*ThreadEngine_SyncData* threadEngine_SyncData = (ThreadEngine_SyncData*)arg;
	GSPAT::Solver* solver = threadEngine_SyncData->solver;
	struct OpenCL_ExecutionContext * context = (struct OpenCL_ExecutionContext *)solver->getSolverContext();
	cl_kernel fillDataFromPrimitives;
	{//Initialize engine:
		//Create program
		cl_program program;

		OpenCLUtilityFunctions::createProgramFromFile("PBDEngine.cl", context->context, context->device, &program);
		cl_int err;
		fillDataFromPrimitives = clCreateKernel(program, "fillDataFromPrimitives2", &err);
		if (err < 0) {
			printWarning_PBDEngine("Couldn't create kernel\n");
		}
	}
	//Run engine:
	PBDEngine::PrimitiveManager& pm2 = PBDEngine::PrimitiveManager::instance();
	PBDEngine::BufferManager& bm = PBDEngine::BufferManager::instance();

	while (threadEngine_SyncData->running){
		cl_int err=0;
		//Variables to store input:		
		cl_mem memory, bufferDescriptors;
		std::vector<cl_event> *committedEvents;
		cl_mem solutionBuffers[5];
		//Retrive values for input variables:
		cl_mem primitives;
		cl_event primitivesUploaded= pm2.getCurrentPrimitives(&primitives);
		PBDEngine::PrimitiveTargets& targets = pm2.getPrimitiveTargets();		
		bm.getBufferDescriptors(&memory, &bufferDescriptors, &committedEvents);
		committedEvents->push_back(primitivesUploaded);
		//Create GSPAT Solution
		GSPAT::Solution* solution = solver->createSolutionExternalData(targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries, true, GSPAT::MatrixAlignment::RowMajorAlignment);
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
			printMessage_PBDEngine("PBDEngine::Couldn't set kernel arguments to upload solution data"); 
			pm2.postRender(0);
			break; }
		//Run kernel and compute solution:
		size_t global_size[2] = { targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries };
		size_t local_size[2] = { targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries };
		cl_event finishedUpload;
		err |= clEnqueueNDRangeKernel(context->queue, fillDataFromPrimitives, 2, NULL, global_size, local_size, committedEvents->size(), committedEvents->data(), &(finishedUpload));
		solution->dataExternallyUploadedEvent(&finishedUpload, 1);
		solver->compute(solution); 
		//Post-render updates:
		pm2.postRender(threadEngine_SyncData->numGeometries);
		bm.releasePendingCommitedCopies(committedEvents);
		if (err < 0) { 
			printMessage_PBDEngine("PBDEngine:: Kernel error uploading external solution data."); 
			break; }
		//Read results and release solution:
		float positions[256], amplitudes[64], mStartsOut[32], mEndsOut[32], initialGuess[4];
		PBDEngine::Primitive primitivesGPU[32];
		float B[512 * 32 * 2 * 2], F[512 * 32 * 2 * 2], R[2 * 2 * 32], points[32 * 2 * 2];
		{//DEBUG
			err = clEnqueueReadBuffer(context->queue, (primitives), CL_TRUE, 0, PBDEngine::MAX_PRIMITIVES* sizeof(PBDEngine::Primitive ), &(primitivesGPU), 1, &(finishedUpload), NULL);		
			err = clEnqueueReadBuffer(context->queue, (solutionBuffers[0]), CL_TRUE, 0, targets.numPrimitivesBeingRendered* threadEngine_SyncData->numGeometries * 4 * sizeof(float), &(positions), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, (solutionBuffers[1]), CL_TRUE, 0, targets.numPrimitivesBeingRendered* threadEngine_SyncData->numGeometries * sizeof(float), &(amplitudes), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, (solutionBuffers[2]), CL_TRUE, 0, targets.numPrimitivesBeingRendered * 2 * sizeof(float), &(initialGuess), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, (solutionBuffers[3]), CL_TRUE, 0, targets.numPrimitivesBeingRendered * 16 * sizeof(float), &(mStartsOut), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, (solutionBuffers[4]), CL_TRUE, 0, targets.numPrimitivesBeingRendered * 16 * sizeof(float), &(mEndsOut), 1, &(finishedUpload), NULL);	
			solution->readTargetPointsReIm(points, threadEngine_SyncData->numGeometries);
			solution->readMatrixR(R, threadEngine_SyncData->numGeometries);
		}//END DEBUG

		unsigned char* msgTop, *msgBottom;		
		solution->finalMessages(&msgTop, &msgBottom);
		threadEngine_SyncData->driver->updateMessage(msgBottom, msgTop, threadEngine_SyncData->numGeometries);
		solver->releaseSolution(solution);		
	}
	pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
	threadEngine_SyncData->runningThreads--;
	printf("Engine thread finished. Num threads remaining %d\n",threadEngine_SyncData->runningThreads );
	pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
	printf("Engine thread finished");
	return NULL;*/

}

void* engineWritter(void* arg) { 
	ThreadEngine_SyncData* threadEngine_SyncData = (ThreadEngine_SyncData*)arg;
	GSPAT::Solver* solver = threadEngine_SyncData->solver;
	struct OpenCL_ExecutionContext * context = (struct OpenCL_ExecutionContext *)solver->getSolverContext();
	cl_kernel fillDataFromPrimitives;
	{//Initialize engine:
		//Create program
		cl_program program;
		OpenCLUtilityFunctions::createProgramFromFile("PBDEngine.cl", context->context, context->device, &program);
		cl_int err;
		fillDataFromPrimitives = clCreateKernel(program, "fillDataFromPrimitives2", &err);
		if (err < 0) {
			printWarning_PBDEngine("Couldn't create kernel\n");
		}
	}
	//Run engine:
	PBDEngine::PrimitiveManager& pm2 = PBDEngine::PrimitiveManager::instance();
	PBDEngine::BufferManager& bm = PBDEngine::BufferManager::instance();

	while (threadEngine_SyncData->running){
		//0. Wait for update time:
		bool firstTime = true;
		float timePerUpdateInMilis = 1000.0f/ threadEngine_SyncData->targetUPS; //this needs to change every time we change dynamicTargetUPS
		struct timeval prevTime, curTime, prevResetTime;
		float timeSinceLastUpdate;		
		if (firstTime) {
			gettimeofday(&prevTime, 0x0);
			prevResetTime = prevTime;
			firstTime = false;
		}
		do {
			gettimeofday(&curTime, 0x0);
			timeSinceLastUpdate = computeTimeElapsedInMilis(prevTime, curTime);
		} while (timeSinceLastUpdate <  threadEngine_SyncData->numGeometries*timePerUpdateInMilis && timeSinceLastUpdate>0);

		//1. Acess variables to store input:		
		cl_int err=0;
		cl_mem memory, bufferDescriptors;
		std::vector<cl_event> *committedEvents;
		cl_mem solutionBuffers[5];
		//Retrive values for input variables:
		cl_mem primitives;
		cl_event primitivesUploaded= pm2.getCurrentPrimitives(&primitives);
		PBDEngine::PrimitiveTargets& targets = pm2.getPrimitiveTargets();		
		bm.getBufferDescriptors(&memory, &bufferDescriptors, &committedEvents);
		committedEvents->push_back(primitivesUploaded);
		//Create GSPAT Solution
		GSPAT::Solution* solution = solver->createSolutionExternalData(targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries, true, GSPAT::MatrixAlignment::RowMajorAlignment);
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
			printMessage_PBDEngine("PBDEngine::Couldn't set kernel arguments to upload solution data"); 
			pm2.postRender(0);
			break; }
		//2. Run kernel and compute solution:
		size_t global_size[2] = { targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries };
		size_t local_size[2] = { targets.numPrimitivesBeingRendered, threadEngine_SyncData->numGeometries };
		cl_event finishedUpload;
		err |= clEnqueueNDRangeKernel(context->queue, fillDataFromPrimitives, 2, NULL, global_size, local_size, committedEvents->size(), committedEvents->data(), &(finishedUpload));
		solution->dataExternallyUploadedEvent(&finishedUpload, 1);
		solver->compute(solution); 
		//3. Post-render updates:
		pm2.postRender(threadEngine_SyncData->numGeometries);
		bm.releasePendingCommitedCopies(committedEvents);
		if (err < 0) { 
			printMessage_PBDEngine("PBDEngine:: Kernel error uploading external solution data."); 
			break; }
		//4. Notify reader thread@
		pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
		threadEngine_SyncData->solutions.push_back(solution);
		pthread_mutex_unlock(&(threadEngine_SyncData->solution_available));
		pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
		
		
	}
	//Reduce number of running threads before exiting:
	pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
	threadEngine_SyncData->runningThreads--;
	printf("Engine thread finished. Num threads remaining %d\n",threadEngine_SyncData->runningThreads );
	pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
	printf("Engine thread finished");

	return NULL; 
}

void updateGeometries( ThreadEngine_SyncData* threadEngine_SyncData, unsigned char* messagesBottom, unsigned char* messagesTop, int numMessagesToSend) {
	static bool firstTime = true;
	static float timePerUpdateInMilis = 1000.0f / threadEngine_SyncData->targetUPS; //this needs to change every time we change dynamicTargetUPS
	static int numSolutions = threadEngine_SyncData->targetUPS;
	static struct timeval prevTime, curTime, prevResetTime;
	static float timeSinceLastUpdate;

	if (firstTime) {
		gettimeofday(&prevTime, 0x0);
		prevResetTime = prevTime;
		firstTime = false;
	}
	//Send phases and update time
	threadEngine_SyncData->driver->updateMessage(messagesBottom, messagesTop, numMessagesToSend);
	//let's check the current time and wait untill next update is due
	//gettimeofday(&curTime, 0x0);
	do {
		gettimeofday(&curTime, 0x0);
		timeSinceLastUpdate = computeTimeElapsedInMilis(prevTime, curTime);
	} while (timeSinceLastUpdate < numMessagesToSend*timePerUpdateInMilis && timeSinceLastUpdate>0);


	prevTime = curTime;
	//Plot performance (should be 1s)
	numSolutions -= numMessagesToSend;
	if (numSolutions <= 0) {
		timePerUpdateInMilis =1000.0/(threadEngine_SyncData->targetUPS);
		numSolutions = threadEngine_SyncData->targetUPS;
		//printf("Time Per computation = %f; UPS: %d\n",  computeTimeElapsedInMilis(prevResetTime, curTime)/threadEngine_SyncData.targetUPS, threadEngine_SyncData.targetUPS);
		prevResetTime = curTime;
	}
}

void* engineReader(void* arg) { 
	ThreadEngine_SyncData* threadEngine_SyncData = (ThreadEngine_SyncData*)arg;
	GSPAT::Solver* solver = threadEngine_SyncData->solver;

	while (threadEngine_SyncData->running) {
		//0. Check the Queue:
		pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
		while (threadEngine_SyncData->solutions.size() == 0) {
			//1. There is nothing to be done... wait until some solution is produced.
			pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
			pthread_mutex_lock(&(threadEngine_SyncData->solution_available));
			pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
		}
		//1. We got access! 
		{
			//2. Let's get the first solution in the queue (and unlock, so others can keep adding/removing)
			GSPAT::Solution* curSolution = threadEngine_SyncData->solutions[0];
			threadEngine_SyncData->solutions.pop_front();
			pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
			// read the final phases and discretise. 
			unsigned char* topMessages = NULL, *bottomMessages = NULL;
			curSolution->finalMessages(&topMessages, &bottomMessages);
			for (int g = 0; g < threadEngine_SyncData->numGeometries; g ++)
				updateGeometries(threadEngine_SyncData,&(bottomMessages[512 * g]), &(topMessages[512 * g]), 1);
			solver->releaseSolution(curSolution);
		}
	}
	//Reduce number of running threads before exiting:
	pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
	threadEngine_SyncData->runningThreads--;
	printf("Engine thread finished. Num threads remaining %d\n",threadEngine_SyncData->runningThreads );
	pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
	printf("Engine thread finished");
	return NULL;
}

void* client(void* arg) { 
	char c;
	ThreadEngine_SyncData* threadEngine_SyncData = (ThreadEngine_SyncData*)arg;	
	PBDEngine::IPrimitiveUpdater& pm = PBDEngine::PrimitiveManager::primitiveUpdaterInstance();
	cl_uint primitives[] = { pri1, pri2 };//Declared as global shared variables
	static const size_t X_index = 3, Y_index = 7, Z_index = 11;
	float matrices[]={1,0,0,0,
					  0,1,0,0,
					  0,0,1,0,
					  0,0,0,1,
					  1,0,0,0,
					  0,1,0,0,
					  0,0,1,0,
					  0,0,0,1};
	float *cur = &(matrices[0]), *prev = &(matrices[16]);
	while (threadEngine_SyncData->running) {
		bool commit = false;
		//LEFT_RIGHT
		//scanf("%c", &c);
		switch (getch()) {
		case 'a':
			commit = true;
			prev[X_index] = cur[X_index];
			cur[X_index] += 0.001f;
			prev[Y_index] = cur[Y_index];
			break;

		case 'd':
			commit = true;
			prev[X_index] = cur[X_index];
			cur[X_index] -= 0.001f;
			prev[Y_index] = cur[Y_index];
			break;
		case 'w':
			commit = true;
			prev[Y_index] = cur[Y_index];
			cur[Y_index] += 0.001f;
			prev[X_index] = cur[X_index];
			break;
		case 's':
			commit = true;
			prev[Y_index] = cur[Y_index];
			cur[Y_index] -= 0.001f;
			prev[X_index] = cur[X_index];
			break;
		case ' ':
			printf("SPACE BAR pressed");
			threadEngine_SyncData->running = false;
			break;
		case '1':
			pm.updatePrimitive_Positions(pri1, circle1, 0);
			pm.updatePrimitive_Positions(pri2, circle1, numSamplesCircle/2);
			pm.commitUpdates();
			break;
		case '2':
			pm.updatePrimitive_Positions(pri1, pos1, 0);
			pm.updatePrimitive_Positions(pri2, pos2, 0);
			pm.commitUpdates();
			Sleep(100);//This will fail. I need a callback for when the circle is finished.
			//pm.updatePrimitive_Positions(pri2, pos2, 0);
			
			break;
		}
		//Update engine:
		if (commit) {
			printf("(%f, %f, %f)\n", cur[X_index], cur[Y_index], cur[Z_index]);
			float mStarts[32], mEnds[32];
			memcpy(&(mStarts[0]), prev, 16 * sizeof(float));
			memcpy(&(mStarts[16]), prev, 16 * sizeof(float));
			memcpy(&(mEnds[0]), cur, 16 * sizeof(float));
			memcpy(&(mEnds[16]), cur, 16 * sizeof(float));
			pm.update_HighLevel(primitives, 2, mStarts, mEnds/*, GSPAT::MatrixAlignment::RowMajorAlignment*/);
			//pm.commitUpdates();
		}
	}

	return NULL; 
}
void main() {
	ThreadEngine_SyncData threadEngine_SyncData;
	threadEngine_SyncData.solver= initGSPAT(threadEngine_SyncData);
	declareContent(threadEngine_SyncData.solver);
	pthread_t engineWritterThread, engineReaderThread, userThread;
	//Initialize sync data:
	pthread_mutex_init(&(threadEngine_SyncData.solution_available), NULL);
	pthread_mutex_lock(&(threadEngine_SyncData.solution_available));
	pthread_mutex_init(&(threadEngine_SyncData.mutex_solution_queue), NULL);
	threadEngine_SyncData.running = true;
	//Create threads:
	bool monothread = true;
	if (monothread) {
		threadEngine_SyncData.runningThreads = 1;
		pthread_create(&engineWritterThread, NULL, monoThreadEngine, &threadEngine_SyncData);
	}
	else {
		threadEngine_SyncData.runningThreads = 2;
		pthread_create(&engineWritterThread, NULL, engineWritter, &threadEngine_SyncData);
		pthread_create(&engineReaderThread, NULL, engineReader, &threadEngine_SyncData);
	}
	client((void*)&(threadEngine_SyncData));
	//Wait until theads are finished: 
	while (threadEngine_SyncData.runningThreads > 0) {
		Sleep(1000);
		printf("Waiting for %d threads to finish\n", threadEngine_SyncData.runningThreads);
	}
	pthread_mutex_destroy(&(threadEngine_SyncData.solution_available));
	pthread_mutex_destroy(&(threadEngine_SyncData.mutex_solution_queue));

}


