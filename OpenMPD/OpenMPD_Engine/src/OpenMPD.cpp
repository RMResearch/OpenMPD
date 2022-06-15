#include <OpenMPD.h>
#include <src/EngineThreads.h>
#include <src/ThreadEngine_SyncData.h>
#include <src/BufferManager.h>
#include <src/PrimitiveDescriptors.h>
#include <src/OpenMPD_Context.h>
#include <src/Synchronize/ForceUPS_Sync.h>
#include <sstream>
#include <fstream>

void (*_printMessage_OpenMPD) (const char*) = NULL;
void (*_printError_OpenMPD)(const char*)= NULL;
void (*_printWarning_OpenMPD)(const char*)= NULL;
ForceUPS_Sync* syncWithExternalThread = NULL;

//Parameters set during call to setup (not used until Start is called)
OpenMPD::GSPAT_SOLVER version = OpenMPD::GSPAT_SOLVER::NAIVE;
size_t _memorySizeInBytes;
const int boards2 = 2;
const int boards4 = 4;
const int boards6 = 6;
const int boards8 = 8;
const int boards10 = 10;
const int boards12 = 12;

void OpenMPD::printMessage_OpenMPD(const char* str) {
	void(*aux)(const char*) = _printMessage_OpenMPD;//Atomic read (avoid thread sync issues)
	if (aux) aux(str);
}
void OpenMPD::printError_OpenMPD(const char* str) {
	void(*aux)(const char*) = _printError_OpenMPD;//Atomic read (avoid thread sync issues)
	if (aux) aux(str);
}
void OpenMPD::printWarning_OpenMPD(const char* str) {
	void(*aux)(const char*) = _printWarning_OpenMPD;//Atomic read (avoid thread sync issues)
	if (aux) aux(str);
}

void OpenMPD::RegisterPrintFuncs(void(*p_Message)(const char*), void(*p_Warning)(const char*), void(*p_Error)(const char*)) {
	_printMessage_OpenMPD = p_Message;
	_printError_OpenMPD = p_Error;
	_printWarning_OpenMPD = p_Warning;
	OpenMPD::printMessage_OpenMPD("Got message functions!");
}

static ThreadEngine_SyncData* threadEngine_SyncData=NULL;

bool OpenMPD::Initialize() {
	/*if(threadEngine_SyncData != NULL)
		printMessage_OpenMPD("OpenMPD was already initialized. Command ignored.");	*/
	return true;
}

void OpenMPD::SetupEngine(size_t memorySizeInBytes, OpenMPD::GSPAT_SOLVER v, OpenMPD::IVisualRenderer* renderer){
	//0. Check if this step was already run:
	if (threadEngine_SyncData != NULL) {
		printWarning_OpenMPD("OpenMPD was already setup. Command ignored. \n If you want to redefine the memory size used, you need to completeley destroy (Stop, Release) the engine first.");	
		return;
	}
	//1. Create shared data structures supporting the rendering engine:
	{
		threadEngine_SyncData = new ThreadEngine_SyncData;
		//Create driver and connect to it
		AsierInho_V2::RegisterPrintFuncs(printMessage_OpenMPD, printMessage_OpenMPD, printMessage_OpenMPD);
		threadEngine_SyncData->driver = AsierInho_V2::createAsierInho();
		threadEngine_SyncData->renderer = renderer;
		version = v; 
		_memorySizeInBytes = memorySizeInBytes;		
	}	
}

OpenMPD::IPrimitiveUpdater* OpenMPD::StartEngine( cl_uchar FPS_Divider, cl_uint numParallelGeometries, cl_uint topBoardID, cl_uint bottomBoardID, bool forceSync) {
	if (threadEngine_SyncData == NULL) {
		printError_OpenMPD("OpenMPD is being started without being initialized.\nPlease, call OpenMPD::SetupEngine(<Memory_Size>) before running the engine.");
		return NULL;
	}

	//1. Configure underlying running infrastructure:
	{
		threadEngine_SyncData->numGeometries = numParallelGeometries;
		threadEngine_SyncData->newDivider = FPS_Divider;
		threadEngine_SyncData->targetUPS = 40000.0f / FPS_Divider;
		//Connect to boards and configure solver:
		if (bottomBoardID != 0 || topBoardID != 0)
			threadEngine_SyncData->driver->connect(bottomBoardID, topBoardID);
		else
			OpenMPD::printMessage_OpenMPD("Simulator Mode -> no Device attached!!!");
	}
	//2. Create and setup solver:
	{		
		switch (version) {		
		case GSPAT_SOLVER::V2:
			GSPAT_V2::RegisterPrintFuncs(printMessage_OpenMPD, printMessage_OpenMPD, printMessage_OpenMPD);
			threadEngine_SyncData->solver = GSPAT_V2::createSolver(threadEngine_SyncData->driver->totalTransducers());
			OpenMPD::printMessage_OpenMPD("Engine setup with GSPAT Version 2 (GSPAT_SolverV2.dll)");
			break;
		case GSPAT_SOLVER::IBP:
			GSPAT_IBP::RegisterPrintFuncs(printMessage_OpenMPD, printMessage_OpenMPD, printMessage_OpenMPD);
			threadEngine_SyncData->solver = GSPAT_IBP::createSolver(threadEngine_SyncData->driver->totalTransducers());
			OpenMPD::printMessage_OpenMPD("Engine setup with GSPAT Version 1 (IBP - GSPAT_SolverIBP.dll)");
			break;
		case GSPAT_SOLVER::NAIVE:
		default:
			GSPAT_Naive::RegisterPrintFuncs(printMessage_OpenMPD, printMessage_OpenMPD, printMessage_OpenMPD);
			threadEngine_SyncData->solver = GSPAT_Naive::createSolver(threadEngine_SyncData->driver->totalTransducers());
			OpenMPD::printMessage_OpenMPD("Engine setup with GSPAT Version 0 (Naive - GSPAT_SolverNaive.dll)");
			break;
		}
		//Create joint data structures for direct communication Engine-Solver (Primitives and Buffer Managers)
		struct OpenCL_ExecutionContext * executionContext = (struct OpenCL_ExecutionContext *)threadEngine_SyncData->solver->getSolverContext();
		OpenMPD::OpenMPD_Context::instance().initialize(_memorySizeInBytes / sizeof(float), executionContext, threadEngine_SyncData->renderer);//2Mfloats (64MB?)
		printMessage_OpenMPD("OpenMPD setup successfully.");
		//Configure solver with callibration data
		float transducerPositions[512 * 3], amplitudeAdjust[512];
		int mappings[512], phaseDelays[512], numDiscreteLevels;

		if (bottomBoardID != 0 || topBoardID != 0)
			threadEngine_SyncData->driver->readParameters(transducerPositions, mappings, phaseDelays, amplitudeAdjust, &numDiscreteLevels);

		threadEngine_SyncData->solver->setBoardConfig(transducerPositions, mappings, phaseDelays, amplitudeAdjust, numDiscreteLevels);
	}
	//2. Setup and run working threads: 
	{
		printMessage_OpenMPD("OpenMPD setting up threads.");
		pthread_t engineWritterThread, engineReaderThread;
		//Initialize sync data:
		pthread_mutex_init(&(threadEngine_SyncData->solution_available), NULL);
		pthread_mutex_lock(&(threadEngine_SyncData->solution_available));
		pthread_mutex_init(&(threadEngine_SyncData->mutex_solution_queue), NULL);
		threadEngine_SyncData->running = true;
		printMessage_OpenMPD("OpenMPD synchronization resources setup.");
		//Create threads:
		{
			int rc;
			pthread_attr_t attr;
			struct sched_param param;
			rc = pthread_attr_init(&attr);
			rc = pthread_attr_getschedparam(&attr, &param);
			(param.sched_priority)++;
			rc = pthread_attr_setschedparam(&attr, &param);
		}

		threadEngine_SyncData->runningThreads = 2;
		pthread_create(&engineWritterThread, NULL, engineWritter, threadEngine_SyncData);
		pthread_create(&engineReaderThread, NULL, engineReader, threadEngine_SyncData);
		printMessage_OpenMPD("OpenMPD threads launched.");
	}
	if (bottomBoardID != 0 || topBoardID != 0) {
		threadEngine_SyncData->driver->turnTransducersOn();
		printMessage_OpenMPD("OpenMPD started successfully.");

		if (forceSync) {
			syncWithExternalThread = new ForceUPS_Sync();
			OpenMPD::OpenMPD_Context::instance().addListener(syncWithExternalThread);
			addEngineWriterListener(syncWithExternalThread);
		}
	}
	else
		printMessage_OpenMPD("OpenMPD started successfully. -> Simulation Mode");

	return &(OpenMPD::OpenMPD_Context::primitiveUpdaterInstance());
}

OpenMPD::IPrimitiveUpdater* OpenMPD::StartEngineMultiSetup(cl_uchar FPS_Divider, cl_uint numParallelGeometries, int numBoards, int* boardIDs, float* matBoardToWorld4x4, bool forceSync) {
	if (threadEngine_SyncData == NULL) {
		printError_OpenMPD("OpenMPD is being started without being initialized.\nPlease, call OpenMPD::SetupEngine(<Memory_Size>) before running the engine.");
		return NULL;
	}

	//1. Configure underlying running infrastructure:
	{
		threadEngine_SyncData->numGeometries = numParallelGeometries;
		threadEngine_SyncData->newDivider = FPS_Divider;
		threadEngine_SyncData->targetUPS = 40000.0f / FPS_Divider;
		//Connect to boards and configure solver:
		threadEngine_SyncData->driver->connect(numBoards, boardIDs, matBoardToWorld4x4, numParallelGeometries);
	}
	//2. Create and setup solver:
	{
		switch (version) {
		case GSPAT_SOLVER::V2:
			GSPAT_V2::RegisterPrintFuncs(printMessage_OpenMPD, printMessage_OpenMPD, printMessage_OpenMPD);
			threadEngine_SyncData->solver = GSPAT_V2::createSolver(threadEngine_SyncData->driver->totalTransducers());
			OpenMPD::printMessage_OpenMPD("Engine setup with GSPAT Version 2 (GSPAT_SolverV2.dll)");
			break;
		case GSPAT_SOLVER::IBP:
			GSPAT_IBP::RegisterPrintFuncs(printMessage_OpenMPD, printMessage_OpenMPD, printMessage_OpenMPD);
			threadEngine_SyncData->solver = GSPAT_IBP::createSolver(threadEngine_SyncData->driver->totalTransducers());
			OpenMPD::printMessage_OpenMPD("Engine setup with GSPAT Version 1 (IBP - GSPAT_SolverIBP.dll)");
			break;
		case GSPAT_SOLVER::NAIVE:
		default:
			GSPAT_Naive::RegisterPrintFuncs(printMessage_OpenMPD, printMessage_OpenMPD, printMessage_OpenMPD);
			threadEngine_SyncData->solver = GSPAT_Naive::createSolver(threadEngine_SyncData->driver->totalTransducers());
			OpenMPD::printMessage_OpenMPD("Engine setup with GSPAT Version 0 (Naive - GSPAT_SolverNaive.dll)");
			break;
		}
		//Create joint data structures for direct communication Engine-Solver (Primitives and Buffer Managers)
		struct OpenCL_ExecutionContext* executionContext = (struct OpenCL_ExecutionContext*)threadEngine_SyncData->solver->getSolverContext();
		OpenMPD::OpenMPD_Context::instance().initialize(_memorySizeInBytes / sizeof(float), executionContext, threadEngine_SyncData->renderer);//2Mfloats (64MB?)
		printMessage_OpenMPD("OpenMPD setup successfully.");

		switch (numBoards)
		{
		case 2:
			//Configure solver with callibration data
			float transducerPositions2[256 * 3* boards2], amplitudeAdjust2[256*boards2];//512
			int mappings2[256 * boards2], phaseDelays2[256 * boards2], numDiscreteLevels2;

			threadEngine_SyncData->driver->readParameters(transducerPositions2, mappings2, phaseDelays2, amplitudeAdjust2, &numDiscreteLevels2);
			threadEngine_SyncData->solver->setBoardConfig(transducerPositions2, mappings2, phaseDelays2, amplitudeAdjust2, numDiscreteLevels2);
			break;
		case 4:
			//Configure solver with callibration data
			float transducerPositions4[256 * 3 * boards4], amplitudeAdjust4[256 * boards4];//512
			int mappings4[256 * boards4], phaseDelays4[256 * boards4], numDiscreteLevels4;

			threadEngine_SyncData->driver->readParameters(transducerPositions4, mappings4, phaseDelays4, amplitudeAdjust4, &numDiscreteLevels4);
			threadEngine_SyncData->solver->setBoardConfig(transducerPositions4, mappings4, phaseDelays4, amplitudeAdjust4, numDiscreteLevels4);
			break;
		case 6:
			//Configure solver with callibration data
			float transducerPositions6[256 * 3 * boards6], amplitudeAdjust6[256 * boards6];//512
			int mappings6[256 * boards6], phaseDelays6[256 * boards6], numDiscreteLevels6;

			threadEngine_SyncData->driver->readParameters(transducerPositions6, mappings6, phaseDelays6, amplitudeAdjust6, &numDiscreteLevels6);
			threadEngine_SyncData->solver->setBoardConfig(transducerPositions6, mappings6, phaseDelays6, amplitudeAdjust6, numDiscreteLevels6);
			break;
		case 8:
			//Configure solver with callibration data
			float transducerPositions8[256 * 3 * boards8], amplitudeAdjust8[256 * boards8];//512
			int mappings8[256 * boards8], phaseDelays8[256 * boards8], numDiscreteLevels8;

			threadEngine_SyncData->driver->readParameters(transducerPositions8, mappings8, phaseDelays8, amplitudeAdjust8, &numDiscreteLevels8);
			threadEngine_SyncData->solver->setBoardConfig(transducerPositions8, mappings8, phaseDelays8, amplitudeAdjust8, numDiscreteLevels8);
			break;
		case 10:
			//Configure solver with callibration data
			float transducerPositions10[256 * 3 * boards10], amplitudeAdjust10[256 * boards10];//512
			int mappings10[256 * boards10], phaseDelays10[256 * boards10], numDiscreteLevels10;

			threadEngine_SyncData->driver->readParameters(transducerPositions10, mappings10, phaseDelays10, amplitudeAdjust10, &numDiscreteLevels10);
			threadEngine_SyncData->solver->setBoardConfig(transducerPositions10, mappings10, phaseDelays10, amplitudeAdjust10, numDiscreteLevels10);
			break;
		case 12:
			//Configure solver with callibration data
			float transducerPositions12[256 * 3 * boards12], amplitudeAdjust12[256 * boards12];//512
			int mappings12[256 * boards12], phaseDelays12[256 * boards12], numDiscreteLevels12;

			threadEngine_SyncData->driver->readParameters(transducerPositions12, mappings12, phaseDelays12, amplitudeAdjust12, &numDiscreteLevels12);
			threadEngine_SyncData->solver->setBoardConfig(transducerPositions12, mappings12, phaseDelays12, amplitudeAdjust12, numDiscreteLevels12);
			break;
		default:
			break;
		}				
	}
	//2. Setup and run working threads: 
	{
		printMessage_OpenMPD("OpenMPD setting up threads.");
		pthread_t engineWritterThread, engineReaderThread;
		//Initialize sync data:
		pthread_mutex_init(&(threadEngine_SyncData->solution_available), NULL);
		pthread_mutex_lock(&(threadEngine_SyncData->solution_available));
		pthread_mutex_init(&(threadEngine_SyncData->mutex_solution_queue), NULL);
		threadEngine_SyncData->running = true;
		printMessage_OpenMPD("OpenMPD synchronization resources setup.");
		//Create threads:
		{
			int rc;
			pthread_attr_t attr;
			struct sched_param param;
			rc = pthread_attr_init(&attr);
			rc = pthread_attr_getschedparam(&attr, &param);
			(param.sched_priority)++;
			rc = pthread_attr_setschedparam(&attr, &param);
		}

		threadEngine_SyncData->runningThreads = 2;
		pthread_create(&engineWritterThread, NULL, engineWritter, threadEngine_SyncData);
		pthread_create(&engineReaderThread, NULL, engineReader, threadEngine_SyncData);
		printMessage_OpenMPD("OpenMPD threads launched.");
	}
	
	threadEngine_SyncData->driver->turnTransducersOn();
	printMessage_OpenMPD("OpenMPD started successfully.");

	if (forceSync) {
		syncWithExternalThread = new ForceUPS_Sync();
		OpenMPD::OpenMPD_Context::instance().addListener(syncWithExternalThread);
		addEngineWriterListener(syncWithExternalThread);
	}
	
	return &(OpenMPD::OpenMPD_Context::primitiveUpdaterInstance());
}


_OPEN_MPD_ENGINE_Export void OpenMPD::updateBoardSeparation(float distance)
{
	//0. Check status
	if (threadEngine_SyncData == NULL) {
		printWarning_OpenMPD("OpenMPD is not setup, so board distance cannot be adjusted.\n Please make sure to call SetupEngine and StartEngine before doing this.\n Command ignored.");
		return ;
	}
	pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
	//1. Apply configuration
	float matBoardToWorld[32] = {	/*bottom*/
								1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1,	
								/*top*/
								-1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0,-1, distance,
								0, 0, 0, 1,	
	};
	float transducerPositions[512 * 3], amplitudeAdjust[512];
	int mappings[512], phaseDelays[512], numDiscreteLevels;
	threadEngine_SyncData->driver->updateBoardPositions(matBoardToWorld);
	threadEngine_SyncData->driver->readParameters(transducerPositions, mappings, phaseDelays, amplitudeAdjust, &numDiscreteLevels);
	threadEngine_SyncData->solver->setBoardConfig(transducerPositions, mappings, phaseDelays, amplitudeAdjust, numDiscreteLevels);
	//3. Unlock threads.
	pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));

}

_OPEN_MPD_ENGINE_Export void OpenMPD::setupFPS_Divider(unsigned char FPSDivider)
{
	threadEngine_SyncData->newDivider = FPSDivider;
}

_OPEN_MPD_ENGINE_Export void OpenMPD::setupPhaseOnly(bool phaseOnly)
{
	threadEngine_SyncData->phaseOnly= phaseOnly;
}

_OPEN_MPD_ENGINE_Export void OpenMPD::setupHardwareSync(bool useHardwareSync) {
	threadEngine_SyncData->hardwareSync = useHardwareSync;
}

bool  OpenMPD::StopEngine() {
	if (threadEngine_SyncData == NULL) {
		printWarning_OpenMPD("OpenMPD is not running, so it cannot be stopped either. Command ignored.");
		return false;
	}
	//Disable force sync to allow threads to finish without affecting rach other.
	if (syncWithExternalThread) 
		syncWithExternalThread->disable();			
	//Wait for threads to pick on the message and finish their executions
	threadEngine_SyncData->running = false;
	while (threadEngine_SyncData->runningThreads > 0) {
			Sleep(100);
	}
	//Remove force sync from related modules and delete it (i.e. start engine might decide force sync is not required)	
	if (syncWithExternalThread) {
		OpenMPD::OpenMPD_Context::instance().removeListener(syncWithExternalThread);
		removeEngineWriterListener(syncWithExternalThread);
		delete syncWithExternalThread;
		syncWithExternalThread = NULL;
	}
	pthread_mutex_destroy(&(threadEngine_SyncData->solution_available));
	pthread_mutex_destroy(&(threadEngine_SyncData->mutex_solution_queue));
	threadEngine_SyncData->driver->turnTransducersOff();
	//void removeEngineWriterListener(EngineWriterListener* e);
	printMessage_OpenMPD("OpenMPD stopped.");	
	return true;

}
	
bool OpenMPD::Release() {
	//0. Trivial case:
	if (threadEngine_SyncData == NULL){
		printMessage_OpenMPD("OpenMPD released. There was nothing setup, so no resources needed to be deallocated. ");
		return true;
	}
	//1. Check status:
	if(threadEngine_SyncData->running != false || threadEngine_SyncData->runningThreads != 0){
		printWarning_OpenMPD("OpenMPD cannot be released because some threads are still running.\n Calling StopEngine before releasing... ");
		StopEngine();
	}
	//2. Destroy all related data structures: 
	OpenMPD_Context::instance().releaseAllResources();	
	threadEngine_SyncData->driver->turnTransducersOff();
	threadEngine_SyncData->driver->disconnect();
	delete threadEngine_SyncData->driver;
	delete threadEngine_SyncData->solver;
	delete threadEngine_SyncData;
	threadEngine_SyncData = NULL;
	printMessage_OpenMPD("OpenMPD Released. All resources destroyed.");	
	return true;
}

int OpenMPD::GetCurrentPosIndex() {
	OpenMPD::OpenMPD_Context& pm = OpenMPD::OpenMPD_Context::instance();
	return pm.getCurrentPosIndex();
}
