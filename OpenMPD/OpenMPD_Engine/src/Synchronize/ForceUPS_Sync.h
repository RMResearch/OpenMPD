#pragma once
#include <src/PrimitiveUpdatesListener.h>
#include <src/EngineThreads.h>
#include <pthread.h>
/**
	This class keeps the synchronization between the Rendering thread (external client, such as Unity) and the Writer thread. 
	This tries to enforce a 1:1 match between their running cycles, which is great to ensure continuity of motion (transformation matrices in primitives). 
*/
class ForceUPS_Sync : public OpenMPD::PrimitiveUpdatesListener, public EngineWriterListener {
	pthread_mutex_t waitForUnityUpdate;
	pthread_mutex_t waitForEngineWriterUpdate;
	bool enabled;
public:
	ForceUPS_Sync();
	/**Locks the calling thread on a mutex until the next update is ready to be sent for computation (i.e. primitives data ready, computation might/might not have started yet) in the EngineWriterThread.
		It then returns control to the calling thread (e.g. Unity). 
	*/ 
	virtual void primitiveUpdate_HighLevel(cl_uint* primitiveIDs, cl_uint numPrimitives, float* mStart, float* mEnds);
	/** This method is invoked once primitive is sent to the GPU. 
		Transfer completion can be captured with the OpenCL argument 'event'.
		This can be used to unlock the client thread (locked in primitiveUpdate_HighLevel).	     
	*/
	
	virtual void startCycle();
	virtual void waitNextCycle();
	void disable();
	~ForceUPS_Sync();
	//Methods inherited from PrimitiveUpdatesListener (NO FUNCTIONALITY)
	virtual void primitiveDeclared(cl_uint primitiveID, cl_uint posDesc, cl_uint ampDesc, cl_uint startPosIndex, cl_uint endPosIndex) { ; }
	virtual void primitiveUpdate_LowLevel(cl_uint primitiveID, OpenMPD::Primitive& p) { ; }
	virtual void commit() { ; }
	virtual void primitiveReleased(cl_uint primitiveID) { ; }

};