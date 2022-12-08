#ifndef _OPEN_MPD
#define _OPEN_MPD
#include <OpenMPD_Prerequisites.h>

namespace OpenMPD {
	class _OPEN_MPD_ENGINE_Export IPrimitiveUpdater {
	public:
		//1. Defining low-level descriptors
		virtual cl_uint createPositionsDescriptor(float* positions, int numPosSamples) = 0;
		virtual cl_uint createAmplitudesDescriptor(float* amplitudes, int numPosSamples) = 0;
		virtual cl_uint createColoursDescriptor(float* amplitudes, int numPosSamples) = 0;
		//2. Defining primitives (based on descriptors)
		virtual cl_uint declarePrimitive(cl_uint posDescriptor, cl_uint ampDescriptor, cl_uint firstPosIndex = 0, cl_uint firstAmpIndex = 0) = 0;

		//3. Runtime
		virtual bool setPrimitiveEnabled(cl_uint p, bool enabled) = 0;
		virtual void update_HighLevel(cl_uint* primitives, cl_uint num_primitives, float* mStarts, float* mEnds/*,  GSPAT::MatrixAlignment ma*/) = 0;
		virtual void updatePrimitive_Positions(cl_uint primitiveID, cl_uint nextPosDescriptor, cl_uint nextFirstPosIndex) = 0;
		virtual void updatePrimitive_Amplitudes(cl_uint primitiveID, cl_uint nextAmpDescriptor, cl_uint nextFirstAmpIndex) = 0;
		virtual void updatePrimitive_Colours(cl_uint primitiveID, cl_uint nextAmpDescriptor, cl_uint nextFirstAmpIndex) = 0;
		virtual void commitUpdates() = 0;

		//4. Releasing resources: 
		virtual bool releasePositionsDescriptor(cl_uint pd) = 0;
		virtual bool releaseAmplitudesDescriptor(cl_uint ad) = 0;
		virtual bool releaseColoursDescriptor(cl_uint cd) = 0;
		virtual bool releasePrimitive(cl_uint pd) = 0;
	};
	class _OPEN_MPD_ENGINE_Export IVisualRenderer {
		public:
		virtual void createVisualBufferFromPositions(cl_uint bufferID, size_t numSamples, float* positions)=0;
		virtual void createVisualBufferFromColours(cl_uint bufferID, size_t numSamples, float* positions)=0;
		virtual void releaseVisualBuffer(cl_uint bufferID)=0;
		virtual void updateTargets(OpenMPD::PrimitiveTargets& currentTargets, OpenMPD::Primitive currentPrimitives[OpenMPD::MAX_PRIMITIVES])=0;
		virtual void render(float* P, float* V)=0;
		virtual void setPointSize(int size)=0;
		virtual void setRenderOffsetInSamples(int offset)=0;
		virtual ~IVisualRenderer() {;}
	};
	/**Initialize: Initializes the DLL, but the engine is still not setup.*/
	_OPEN_MPD_ENGINE_Export bool Initialize();

	/** RegisterPrintFuncs: Registers the callback functions that the engine will use to send its notifications, warnings and errors. */
	_OPEN_MPD_ENGINE_Export void RegisterPrintFuncs(void(*p_Message)(const char*), void(*p_Warning)(const char*), void(*p_Error)(const char*));

	/**SetupEngine: Creates all the internal structures and solvers (GSPAT, AsierInho) to prepare the engine to run.
		- memorySizeInBytes indicates the amount of GPU memory that can be used to store position and amplitude descriptors (e.g. low level definitions of our primitives).
			  Please note that there are some fixed data structures that are not included in this amount (so the actual memory used will be a bit larger)

	The engine is still not running (boards not connected, IDs, FPS not configured).
	The method returns/creates the IPrimitiveUpdater (Singleton), which the client can use to control the MPD experience.
	This IPrimitiveUpdater will be common in all subsequent calls to RunEngine and can be used even while the engine is not running
	to declare possition/amplitude buffers, primitives, etc.
	*/
	_OPEN_MPD_ENGINE_Export void SetupEngine(size_t memorySizeInBytes, GSPAT_SOLVER v, OpenMPD::IVisualRenderer* renderer= 0);
	/**StartEngine: Configures the required internal structures (GSPAT) and creates the threads required for the engine to run.
			- FPSDivider: Determines the rate at which updates are sent to the board (FPS = 40000/FPSDivider).
			- numParallelGeometries: Number of solutions computed in parallel. Please note that, combined with the above, this determines the number of run cycle of the engine.
			For instance, if computing 32 geometries in parallel, the engine will need to run at 320 cyples per second to meet 10240 UPS.
			For a more responsive engine, you can use 16 geometries and the engine will ruan at 640 cycles per second (e.g. good for a fast tracking system).
			- topBoardID and bottomBoardID: determine the device IDs to be used.
			- forceSync: Forces the updating thread (e.g. Unity, using the IPrimitiveUpdater) to remain in sync with the wirter thread.
			  Each 'update_HighLevel' called (e.g. Unity) will match a cycle in the writer thread (N geometries sent to GPU).
	The method returns the IPrimitiveUpdater, which the client can use to control the MPD experience.
	If the system is already setup and running, it still returns the current active Primitive Manager.
	If initialization fails, the method will return NULL.
	*/
<<<<<<< HEAD
	_OPEN_MPD_ENGINE_Export IPrimitiveUpdater* StartEngine_TopBottom(cl_uchar FPSDivider, cl_uint numParallelGeometries, cl_uint topBoardID, cl_uint bottomBoardID, bool forceSync = true);
	_OPEN_MPD_ENGINE_Export IPrimitiveUpdater* StartEngine(cl_uchar FPSDivider, cl_uint numParallelGeometries, cl_uint numBoards, cl_uint* boardIDs, float* boardLocationsM4x4, bool forceSync = true);
=======
	_OPEN_MPD_ENGINE_Export IPrimitiveUpdater* StartEngine(cl_uchar FPSDivider, cl_uint numParallelGeometries, cl_uint topBoardID, cl_uint bottomBoardID, bool forceSync = true);
	
	_OPEN_MPD_ENGINE_Export IPrimitiveUpdater* StartEngineSingleBoard(cl_uchar FPS_Divider, cl_uint numParallelGeometries, cl_uint boardID, float* matToWorld, bool forceSync = true);
>>>>>>> 86b6a178ad1200efbee71af63115e85ae3ee9c89

	_OPEN_MPD_ENGINE_Export void updateBoardSeparation(float distance);

	/**	setupFPS_Divider: Adjusts the global rendering speed of the engine, by setting a "divider".
		Dividers are used to ensure hardware controlled framerates, using 40KHz as the base frequency:
				- Divider 1--> Update boards at 40KHz
				- Divider 2--> Update boards at 20KHz
				- Divider 3--> Update boards at 13.33KHz
				- Divider 4--> Update at 10KHz (most common)
	*/
	_OPEN_MPD_ENGINE_Export void setupFPS_Divider(unsigned char FPSDivider);

	/**setupPhaseOnly: Adjusts the solver mode to use only phases or to solve for variable amplitudes .*/
	_OPEN_MPD_ENGINE_Export void setupPhaseOnly(bool usePhaseOnly);

	/**setupHardwareSync: Configures the engine to work with devices having hardware synchronization of phase updates.*/
	_OPEN_MPD_ENGINE_Export void setupHardwareSync(bool useHardwareSync);
	/**
		StopEngine: Stops all threads related to the rendering engine, and turns all transducers off.
		Execution can be resumed by calling RunEngine (but the configuration used can be changed).
		Please note that the existing definitions (Primitives, Buffers) will remain to be usable.
	*/
	_OPEN_MPD_ENGINE_Export bool StopEngine();
	/**
		Release: Deallocates all resources related to the engine (IPrimitiveUpdater).
	*/
	_OPEN_MPD_ENGINE_Export bool Release();
	/**
		getCurrentPosIndex: Retrives the current index beeing rendered from the primitive manager class
	*/
	_OPEN_MPD_ENGINE_Export int GetCurrentPosIndex();

	//These methods are used by the implementation classes to print messages using the callbacks provided by the client. 
	void printMessage_OpenMPD(const char* msg);
	void printError_OpenMPD(const char* msg);
	void printWarning_OpenMPD(const char* msg);
}
#endif