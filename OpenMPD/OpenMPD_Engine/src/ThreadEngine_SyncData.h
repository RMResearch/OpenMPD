#ifndef _THREAD_ENGINE_SYNC_DATA
#define _THREAD_ENGINE_SYNC_DATA
#include <GSPAT_SolverNaive.h>
#include <GSPAT_SolverIBP.h>
#include <GSPAT_SolverV2.h>
#if _ADVANCED_SOLVERS
#include <GSPAT_SolverBEM.h>
#include <GSPAT_SolverTS.h>
#endif 
#include <AsierInho_V2.h>
#include <OpenCLSolverImpl_Interoperability.h>
#include <pthread.h>
#include <deque>

struct ThreadEngine_SyncData {
	cl_uint numGeometries =32;
	float targetUPS=10000;
	unsigned char newDivider = 0; // Set to a specific value when needs changing. Reset to zero once divider updated.
	bool phaseOnly = true;
	AsierInho_V2::AsierInhoBoard_V2* driver;
	GSPAT::Solver* solver;
	pthread_mutex_t mutex_solution_queue;
	pthread_mutex_t solution_available;
	std::deque<GSPAT::Solution*> solutions;
	pthread_t readerThread, writerThread;
	cl_uint runningThreads;
	float lastMissedDeadline_writer, lastMissedDeadline_reader;
	OpenMPD::IVisualRenderer* renderer = 0;
	bool running = true;
	bool hardwareSync = true;
} ;

void* engineWritter(void* arg);
void* engineReader(void* arg);

#endif


