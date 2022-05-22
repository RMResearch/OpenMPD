#include <src/Synchronize/ForceUPS_Sync.h>
#include <stdio.h>
ForceUPS_Sync::ForceUPS_Sync() {
	pthread_mutex_init(&waitForUnityUpdate,NULL);
	pthread_mutex_init(&waitForEngineWriterUpdate,NULL);
	pthread_mutex_lock(&waitForEngineWriterUpdate);
	enabled = true;
}

void ForceUPS_Sync::primitiveUpdate_HighLevel(cl_uint* primitiveIDs, cl_uint numPrimitives, float* mStart, float* mEnds) {
	if (enabled)
	{
		//printf("ForceUPS_Sync::primitiveUpdate_HighLevel\n" );
		pthread_mutex_unlock(&waitForUnityUpdate);
		pthread_mutex_lock(&waitForEngineWriterUpdate);
	}
}

void ForceUPS_Sync::startCycle() {
	if (enabled) {
		//printf("ForceUPS_Sync::startCycle\n" );
		pthread_mutex_lock(&waitForUnityUpdate);
	}
}
void ForceUPS_Sync::waitNextCycle() {
	if (enabled) {
		//printf("ForceUPS_Sync::waitNextCycle\n");
		pthread_mutex_unlock(&waitForEngineWriterUpdate);
	}
}

void ForceUPS_Sync::disable() {
	enabled = false;
	//Make sure we are not holding anyone
	pthread_mutex_unlock(&waitForEngineWriterUpdate);
	pthread_mutex_unlock(&waitForUnityUpdate);
}

ForceUPS_Sync::~ForceUPS_Sync() {
	pthread_mutex_destroy(&waitForEngineWriterUpdate);
	pthread_mutex_destroy(&waitForUnityUpdate);
}
	