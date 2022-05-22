#include <include/PBD_Engine.h>
#include "ThreadEngine_SyncData.h"
#include <src/EngineThreads.h>
#include <Helper\TimeFunctions.h>
#include <sstream>
//namespace PBD_Engine {

	void updateGeometries( ThreadEngine_SyncData* threadEngine_SyncData, unsigned char* message) {
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
		//let's check the current time and wait untill next update is due
		//gettimeofday(&curTime, 0x0);
		do {
			gettimeofday(&curTime, 0x0);
			timeSinceLastUpdate = computeTimeElapsedInMilis(prevTime, curTime);
		} while (timeSinceLastUpdate < 0.99f*timePerUpdateInMilis && timeSinceLastUpdate>0);
		//DEBUG (Check final message obtained)
		{
			/*std::ostringstream msg;
			msg << "Sent message BOTTOM:\n ";
			for (int i = 0; i < 512; i++) msg << (unsigned int)messagesBottom[i]<<", ";
			msg << "\n\nSent message TOP:\n ";
			for (int i = 0; i < 512; i++) msg << (unsigned int)messagesTop[i]<<", ";
			msg << "\n\n";
			PBDEngine::printMessage_PBDEngine(msg.str().c_str());*/

		}

		//Send phases and update time
		threadEngine_SyncData->driver->updateMessage(message);
		prevTime = curTime;
		//Plot performance (should be 1s)
		numSolutions--;
		if (numSolutions <= 0) {
			timePerUpdateInMilis =1000.0f/(threadEngine_SyncData->targetUPS);
			numSolutions = threadEngine_SyncData->targetUPS;
			//DEBUG: Print performance
			static char logPerformance[512];
			sprintf_s(logPerformance, "READ: Time Per computation = %f ms; UPS: %d\n",  computeTimeElapsedInMilis(prevResetTime, curTime)/threadEngine_SyncData->targetUPS, threadEngine_SyncData->targetUPS);
			PBDEngine::printMessage_PBDEngine(logPerformance);			
			prevResetTime = curTime;
		}
	}

	void* engineReader(void* arg) { 
		PBDEngine::printMessage_PBDEngine("PBDEngine: Initialising writer thread.\n");
		ThreadEngine_SyncData* threadEngine_SyncData = (ThreadEngine_SyncData*)arg;
		GSPAT::Solver* solver = threadEngine_SyncData->solver;

		PBDEngine::printMessage_PBDEngine("PBDEngine: Reader thread running \n");
		while (threadEngine_SyncData->running) {
			//0. Check the Queue:
			pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
			while (threadEngine_SyncData->running && threadEngine_SyncData->solutions.size() == 0) {
				//1. There is nothing to be done... wait until some solution is produced.
				pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
				pthread_mutex_lock(&(threadEngine_SyncData->solution_available));
				pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
			}
			//1. We got access! 
			if(threadEngine_SyncData->running){
				//2. Let's get the first solution in the queue (and unlock, so others can keep adding/removing)
				GSPAT::Solution* curSolution = threadEngine_SyncData->solutions[0];
				threadEngine_SyncData->solutions.pop_front();
				pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
				// read the final phases and discretise. 
				unsigned char* msg=NULL;
				curSolution->finalMessages(&msg);
				for (cl_uint g = 0; g < threadEngine_SyncData->numGeometries; g ++)
					updateGeometries(threadEngine_SyncData,&(msg[1024 * g]));
				//DEBUG: Compare intermediate steps (phases from each board are still side-by-side): 
				{
					//float* phases = curSolution->finalArrayPhases();//loads and stored them into the solution
					//std::ostringstream msg;

					//for (int g = 0; g < curSolution->geometries(); g++) {
					//	msg << "\nPhases "<<g<<" out of "<<curSolution->geometries()<<":\n ";
					//	//for (int i = 0; i < 512*curSolution->geometries(); i++) 
					//	for (int y = 0; y < 16; y++)
					//		for (int x = 0; x < 16; x++)
					//			msg << phases[32 * y + x] << ", ";
					//	msg << "\n\n";
					//	for (int y = 0; y < 16; y++)
					//		for (int x = 16; x < 32; x++)
					//			msg << phases[32 * y + x] << ", ";

					//	PBDEngine::printMessage_PBDEngine(msg.str().c_str());
					//}
				}
				solver->releaseSolution(curSolution);
			}
			else {
				pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));			
			}
		}
		//Reduce number of running threads before exiting:
		pthread_mutex_lock(&(threadEngine_SyncData->mutex_solution_queue));
		threadEngine_SyncData->runningThreads--;
		pthread_mutex_unlock(&(threadEngine_SyncData->mutex_solution_queue));
		PBDEngine::printMessage_PBDEngine("PBDEngine: Reader thread finished\n");

		return NULL;
	}
//};