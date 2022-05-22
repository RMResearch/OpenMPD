#include <include/OpenMPD.h>
#include "ThreadEngine_SyncData.h"
#include <src/EngineThreads.h>
#include <Helper\TimeFunctions.h>
#include <sstream>
#include <Windows.h>
//namespace OpenMPD {

void updateBoards_SW_Sync( ThreadEngine_SyncData* threadEngine_SyncData, unsigned char* message,int numGeometries,  bool& firstTime) {
		/*static struct timeval prevTime, curTime, prevResetTime;
		static int numSolutions = 0;
		if (firstTime) {
			gettimeofday(&prevTime, 0x0);
			prevResetTime = prevTime;
			threadEngine_SyncData->lastMissedDeadline_reader = prevTime.tv_sec + prevTime.tv_usec / 1000000.0f;
			firstTime = false;
		}
		//let's check the current time and wait untill next update is due
		struct timeval cycleDuration, timeDeadline, timeRemaining ;
		float timePerUpdateInMilis = 1000.0f / threadEngine_SyncData->targetUPS; //this needs to change every time we change dynamicTargetUPS
		createTimeval(&cycleDuration, timePerUpdateInMilis);
		timeval_add(&timeDeadline, prevTime, cycleDuration);
		//gettimeofday(&curTime, 0x0);
		int tRemaining_usec;
		do {
			gettimeofday(&curTime, 0x0);			
			tRemaining_usec = timeval_subtract(&timeRemaining, timeDeadline, curTime);				
		} while (tRemaining_usec>0);
		
		//Send phases and update time
		threadEngine_SyncData->driver->updateMessage(message);
		//If we fall behind by more than 0.1s (100K usec), we reset:
		if (tRemaining_usec < -100000) {
			prevTime = curTime;
			threadEngine_SyncData->lastMissedDeadline_reader = curTime.tv_sec + curTime.tv_usec / 1000000.0f;
			//printf("Missed deadline\n");
		}
		else //This is the usual case. 
			prevTime = timeDeadline;		
		numSolutions++;
		//Plot performance (once per second)		
		float timeSinceLastFPSUpdate = computeTimeElapsedInMilis(prevResetTime, curTime);
		static float timePerFPSUpdate=5000;
		if (timeSinceLastFPSUpdate>= timePerFPSUpdate) {
			//DEBUG: Print performance
# ifdef _TIME_PROFILING
			static bool doProfile = false;
			if(doProfile)
				threadEngine_SyncData->driver->_profileTimes();
			doProfile = !doProfile;
# endif			
			
			static char logPerformance[512];
			sprintf_s(logPerformance, "READ: Time Per computation = %f ms; UPS: %f Target UPS: %d; \n", timeSinceLastFPSUpdate / numSolutions, 1000 * numSolutions / timeSinceLastFPSUpdate, threadEngine_SyncData->targetUPS);
			OpenMPD::printMessage_OpenMPD(logPerformance);			
			static char logMissedDeadlines[512];
			sprintf_s(logPerformance, "\t Time since missed deadline: Reader = %f; Writer = %f\n", (curTime.tv_sec+curTime.tv_usec/1000000.0f)-threadEngine_SyncData->lastMissedDeadline_reader, (curTime.tv_sec + curTime.tv_usec / 1000000.0f) - threadEngine_SyncData->lastMissedDeadline_writer);
			OpenMPD::printMessage_OpenMPD(logPerformance);

			
			numSolutions = 0;			
			prevResetTime = curTime;
		}*/
	
	#pragma region Evaluation
	/*
	static std::string data = "";
	static double reportPeriod = 1;// 0.1 seconds
	static double reportPeriodToRec = 0.02f;// 0.1 seconds
	static int numSolutions = (int)(reportPeriod * threadEngine_SyncData->targetUPS);
	static int numSolutionsToRec = (int)(reportPeriodToRec * threadEngine_SyncData->targetUPS);
	static struct timeval prevTime, curTime, prevResetTime;
	static float timeSinceLastUpdate;
	//static int counter = 0;

	if (firstTime)
		gettimeofday(&prevTime, 0x0);
	//Send messages and update time
	threadEngine_SyncData->driver->updateMessages(message, numGeometries);
	gettimeofday(&curTime, 0x0);
	prevTime = curTime;

	//Plot performance (During last 5 second)
	numSolutions -= numGeometries;
	numSolutionsToRec -= numGeometries;

	if (numSolutionsToRec <= 0) {
		//Log performance
		float timeSinceLastFPSUpdate = computeTimeElapsed(prevResetTime, curTime);
		//counter++;
		//delta_t,Actual UPS,

		if (numSolutions <= 0) {
			data += std::to_string(timeSinceLastFPSUpdate) + "," + std::to_string((reportPeriodToRec*threadEngine_SyncData->targetUPS - numSolutionsToRec) / timeSinceLastFPSUpdate) + ",";
			numSolutionsToRec = (int)(reportPeriodToRec * threadEngine_SyncData->targetUPS);
			prevResetTime = curTime;

			static char logPerformance[2048];
			//sprintf_s(logPerformance, "Engine Report (last %f seconds): Actual UPS: %f, Target UPS: %f\n", timeSinceLastFPSUpdate, (reportPeriod*threadEngine_SyncData->targetUPS - numSolutions) / timeSinceLastFPSUpdate, threadEngine_SyncData->targetUPS);
			sprintf_s(logPerformance, data.c_str());
			OpenMPD::printMessage_OpenMPD(logPerformance);

			//Reset profiling variables (time, num messages to report).
			numSolutions = (int)(reportPeriod *threadEngine_SyncData->targetUPS);
			//prevResetTime = curTime;
			data = "";
			//counter = 0;
		}
		else {
			data += std::to_string(timeSinceLastFPSUpdate) + "," + std::to_string((reportPeriodToRec*threadEngine_SyncData->targetUPS - numSolutionsToRec) / timeSinceLastFPSUpdate) + "," + "\n";
			numSolutionsToRec = (int)(reportPeriodToRec * threadEngine_SyncData->targetUPS);
			prevResetTime = curTime;
			//counter++;
		}
	}
	*/
	#pragma endregion

		#pragma region 5 seconds report
		static int reportPeriod = 5;
		static int numSolutions = (int)( reportPeriod * threadEngine_SyncData->targetUPS);
		static struct timeval prevTime, curTime, prevResetTime;
		static float timeSinceLastUpdate;
		//0. Initialize timers
		if (firstTime) {
			gettimeofday(&prevTime, 0x0);
			prevResetTime = prevTime;
			firstTime = false;
		}
		//1. We send messages one at a time (when due time arrives):
		for (int g = 0; g < numGeometries; g++) {
			//TODO: Provide a general solution (arbitrary number of boards)
			size_t msgSize = 512;
			size_t numBoards = 2;
			//A. let's check the current time. Wait untill next update is due
			float timePerUpdateInMilis = 1000.0f / threadEngine_SyncData->targetUPS; //this needs to change every time we change dynamicTargetUPS
			gettimeofday(&curTime, 0x0);
			/*do {
				gettimeofday(&curTime, 0x0);
				timeSinceLastUpdate = computeTimeElapsedInMilis(prevTime, curTime);
			} while (timeSinceLastUpdate < 0.95f*timePerUpdateInMilis && timeSinceLastUpdate>0);*/
			//Send message and update time
			unsigned char** board_messages = new unsigned char*[numBoards];
			for (int b = 0; b < numBoards; b++)
				board_messages[b] = &(message[numGeometries*msgSize*b + g*msgSize]);
			threadEngine_SyncData->driver->updateMessagePerBoard(board_messages);
			delete board_messages;
			prevTime = curTime;
			//Plot performance (During last 5 second)
			numSolutions -= 1;
			if (numSolutions <= 0) {
				//Log performance
				float timeSinceLastFPSUpdate = computeTimeElapsed(prevResetTime, curTime);
				static char logPerformance[512];
				sprintf_s(logPerformance, "Engine Report (last %f seconds/SW Sync): Actual UPS: %f, Target UPS: %f\n", timeSinceLastFPSUpdate, (reportPeriod*threadEngine_SyncData->targetUPS - numSolutions) / timeSinceLastFPSUpdate, threadEngine_SyncData->targetUPS);
				OpenMPD::printMessage_OpenMPD(logPerformance);
				//Reset profiling variables (time, num messages to report).
				numSolutions = (int)(reportPeriod *threadEngine_SyncData->targetUPS);
				prevResetTime = curTime;
			}
		}
		
		#pragma endregion
	}

	void updateBoards_HW_Sync( ThreadEngine_SyncData* threadEngine_SyncData, unsigned char* message,int numGeometries,  bool& firstTime) {
		static int reportPeriod = 5;
		static int numSolutions = (int)( reportPeriod * threadEngine_SyncData->targetUPS);
		static struct timeval curTime, prevResetTime;
		static float timeSinceLastUpdate;
		//Initialize timers
		if (firstTime) {
			gettimeofday(&prevResetTime, 0x0);
			firstTime = false;
		}
		//Send messages and update time
		gettimeofday(&curTime, 0x0);
		threadEngine_SyncData->driver->updateMessages(message, numGeometries);
		numSolutions -= numGeometries;
		//Plot performance (During last 5 second)
		if (numSolutions <= 0) {			
			//Log performance
			float timeSinceLastFPSUpdate = computeTimeElapsed(prevResetTime, curTime);
			static char logPerformance[512];
			sprintf_s(logPerformance, "Engine Report (last %f seconds/HW Sync): Actual UPS: %f, Target UPS: %f\n", timeSinceLastFPSUpdate, (reportPeriod*threadEngine_SyncData->targetUPS - numSolutions) / timeSinceLastFPSUpdate, threadEngine_SyncData->targetUPS);
			OpenMPD::printMessage_OpenMPD(logPerformance);
			//Reset profiling variables (time, num messages to report).
			numSolutions = (int)( reportPeriod *threadEngine_SyncData->targetUPS);
			prevResetTime=curTime;
		}		
	}
	
	void* engineReader(void* arg) { 
		OpenMPD::printMessage_OpenMPD("OpenMPD: Initialising writer thread.\n");
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		ThreadEngine_SyncData* threadEngine_SyncData = (ThreadEngine_SyncData*)arg;
		GSPAT::Solver* solver = threadEngine_SyncData->solver;
		OpenMPD::printMessage_OpenMPD("OpenMPD: Reader thread running \n");

		bool firstTime = true;
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
				//3. Read the messages (already discretised). 
				unsigned char* msg=NULL;
				curSolution->finalMessages(&msg);
				//4. Send as needed (depends on HR sync availability)
				if(threadEngine_SyncData->hardwareSync)
					updateBoards_HW_Sync(threadEngine_SyncData,msg, curSolution->geometries(), firstTime);
				else 
					updateBoards_SW_Sync(threadEngine_SyncData,msg, curSolution->geometries(), firstTime);
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
		OpenMPD::printMessage_OpenMPD("OpenMPD: Reader thread finished\n");

		return NULL;
	}
//};