#ifndef PBD_PRIMITIVE_MANAGER
#define PBD_PRIMITIVE_MANAGER
#include "GSPAT_Solver_Prerequisites.h"
#include <OpenCLSolverImpl_Interoperability.h>
#include "src\OpenCLUtilityFunctions.h"
#include "GSPAT_RenderingEngine\src\PrimitiveUpdatesListener.h"
#include <map> 
#include <list>
#include <Windows.h>
#include <pthread.h>

namespace PBDEngine {
	class IPrimitiveUpdater{
	public:
		//1. Defining low-level descriptors
		virtual cl_uint createPositionsDescriptor(float* positions, int numPosSamples) = 0;
		virtual cl_uint createAmplitudesDescriptor(float* amplitudes, int numPosSamples) = 0;

		//2. Defining primitives (based on descriptors)
		virtual cl_uint declarePrimitive(cl_uint posDescriptor, cl_uint ampDescriptor, cl_uint firstPosIndex=0, cl_uint firstAmpIndex=0) = 0;
		
		//3. Runtime
		virtual bool setPrimitiveEnabled(cl_uint p, bool enabled) = 0;
		virtual void update_HighLevel(cl_uint* primitives, cl_uint num_primitives, float* mStarts, float* mEnds,  GSPAT::MatrixAlignment ma) = 0;
		virtual void updatePrimitive_Positions(cl_uint primitiveID, cl_uint nextPosDescriptor, cl_uint nextFirstPosIndex) = 0;
		virtual void updatePrimitive_Amplitudes(cl_uint primitiveID, cl_uint nextAmpDescriptor, cl_uint nextFirstAmpIndex) = 0;
		virtual void commitUpdates()=0;

		//4. Releasing resources: 
		virtual bool releasePositionsDescriptor(cl_uint pd) = 0;
		virtual bool releaseAmplitudesDescriptor(cl_uint ad) = 0;
		virtual bool releasePrimitive(cl_uint p) = 0;
	};
	class PrimitiveManager: public IPrimitiveUpdater {

		/**Allows us to generate unique primitive IDs.*/
		cl_uint primitiveID_seed;
		
		/** The manager works on a dual buffer, containing two copies of the primitives defined in the system.
			These pointers contain the currently active version used to render, and 
			the version with changes which will become active when "commit" is called.			
		*/
		Primitive*  current, *modified;
		pthread_mutex_t mutex; 
		/** Low level descriptors are stored in a single data base.
			Descriptors should not change while they are being used to render a Primitive (continuity cannot be assured).
			Only descriptors not currently under use should change (i.e. not assigned to an active Primitive). 
			Thus, these do not need to be double buffered*/
		std::map < cl_uint, PositionsDescriptor* > positionsDescriptors;
		std::map < cl_uint, AmplitudesDescriptor* > amplitudesDescriptors;

		/** Contains a definition of the Primitives to render, according to current state. 
			This definition remains constant until a new "commit" command is received. 
			This struct avoids unnecessary recomputation between "commits" and can be directly issued to OpenCL.*/
		struct PrimitiveTargets primitiveTargets;

		/**
			This map keeps track of the primitives that will become active (declared and enabled) once the next "commit" command arrives.
			It is used to produce the (constant) PrimitiveTargets used for rendering during "commit".
		*/
		std::map<cl_uint, bool> modifiedPrimitiveTargets;
		
		std::list<PBDEngine::PrimitiveUpdatesListener*> listeners;

		//TODO: Move responsibility to Consistency Manager
		//OpenCL low level resources:
		OpenCL_ExecutionContext* context;
		/** Buffer containing the current primitive targets to render (a struct PrimitiveTargets)*/
		cl_mem primitiveTargets_CLBuffer;
		/** Buffer containing all primitives*/
		cl_mem allPrimitives_CLBuffer[2]; 
		size_t currentCLBUfferIndex;
		//END TODO
		//Singleton related methods:
		PrimitiveManager() : primitiveID_seed(1) { 
			pthread_mutex_init(&(mutex), NULL);
		}

	public: 
		/**Returns a Singleton to the current class. This interface exposes all functionality.*/
		static PrimitiveManager& instance() {
			static PrimitiveManager _instance; 
			return _instance;
		}
		
		/**Returns a Singleton to the current class, to be used by clients.
		   This interface allows management of the Primitives and Descriptors, but does not provide control over
		   low level rendering aspects. */
		static IPrimitiveUpdater& primitiveUpdaterInstance() {
			return ((IPrimitiveUpdater&)instance());
		}
		
		void initialize(struct OpenCL_ExecutionContext* context, size_t memorySizeInFloats) {
			//1. Create memory aligned (PAGE) buffers for the primitive descriptors
			SYSTEM_INFO systemInfo;
			GetSystemInfo(&systemInfo);
			//cout << "Page Size Is: " << systemInfo.dwPageSize;
			current =(Primitive*) _aligned_malloc(MAX_PRIMITIVES*sizeof(Primitive), systemInfo.dwPageSize); //new Primitive[MAX_PRIMITIVES];
			modified = (Primitive*)_aligned_malloc(MAX_PRIMITIVES*sizeof(Primitive), systemInfo.dwPageSize);//new Primitive[MAX_PRIMITIVES];
			Primitive empty;
			for (int p = 0; p < MAX_PRIMITIVES; p++) {
				current[p]= empty;
				modified[p]= empty;
			}
			//2. Create OpenCL buffers:
			cl_int err;
			this->context = context;
			//0. Create buffers required:
			BufferManager::initialize(context, memorySizeInFloats);			
			primitiveTargets_CLBuffer= clCreateBuffer(context->context, CL_MEM_READ_WRITE,  sizeof(struct PrimitiveTargets), NULL, NULL);
			currentCLBUfferIndex = 0;
			allPrimitives_CLBuffer[0]= clCreateBuffer(context->context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,  MAX_PRIMITIVES*sizeof(struct Primitive), current, &err);
			allPrimitives_CLBuffer[1]= clCreateBuffer(context->context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,  MAX_PRIMITIVES*sizeof(struct Primitive), modified, &err);
			current = (Primitive*)clEnqueueMapBuffer(context->queue, allPrimitives_CLBuffer[currentCLBUfferIndex], CL_TRUE, CL_MAP_WRITE, 0, MAX_PRIMITIVES * sizeof(struct Primitive), 0, NULL, NULL, &err);
			modified = (Primitive*)clEnqueueMapBuffer(context->queue, allPrimitives_CLBuffer[1-currentCLBUfferIndex], CL_TRUE, CL_MAP_WRITE, 0, MAX_PRIMITIVES * sizeof(struct Primitive), 0, NULL, NULL, &err);


			/*allPrimitives_CLBuffer[0]= clCreateBuffer(context->context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,  MAX_PRIMITIVES*sizeof(struct Primitive), NULL, NULL);
			allPrimitives_CLBuffer[1]= clCreateBuffer(context->context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,  MAX_PRIMITIVES*sizeof(struct Primitive), NULL, NULL);
			
			current = (Primitive*)clEnqueueMapBuffer(context->queue, allPrimitives_CLBuffer[currentCLBUfferIndex], CL_TRUE, CL_MAP_WRITE, 0, MAX_PRIMITIVES * sizeof(struct Primitive), 0, NULL, NULL, &err);
			modified = (Primitive*)(clEnqueueMapBuffer(context->queue, allPrimitives_CLBuffer[1-currentCLBUfferIndex], CL_TRUE, CL_MAP_WRITE, 0, MAX_PRIMITIVES * sizeof(struct Primitive), 0, NULL, NULL, &err));*/
		}

		/**	Generate a unique and valid Primitive identificator. 
			Returns INVALID_PRIMITIVE if too many primitives have been defined already (MAX_PRIMITIVES). */
		cl_uint getPrimitiveID() {
			cl_uint curID ;
			cl_uint numAttempts = 0;
			//Try to find a valid ID for the primitive
			do {
				curID = primitiveID_seed;
				primitiveID_seed = (primitiveID_seed%MAX_PRIMITIVES)+1;//Valid primitives will go from 1..32
			} while (modified[curID-1].primitiveID !=PBDEngine::INVALID_PRIMITIVE_ID && numAttempts++ <= MAX_PRIMITIVES);
			//Return the ID:
			if (numAttempts <= MAX_PRIMITIVES)
				return curID;
			//Otherwise, we are out of IDs...
			printWarning_PBDEngine("PrimitiveManager: Could not allocate a new primitive ID. Number of primitives declared is too high. ");
			return PBDEngine::INVALID_PRIMITIVE_ID;
		}

		/** Creates a new Positions descriptor, using the data provided to initialize it. 
			It will return an invalid descriptor (BufferManager::INVALID_ID) if no memory is 
			available to define the new descriptor.	*/
		virtual cl_uint createPositionsDescriptor(float* positions, int numPosSamples) { 
			PositionsDescriptor* pd = new PositionsDescriptor(positions, numPosSamples);
			positionsDescriptors[pd->getID()] = pd;
			return pd->getID();
		}

		
		/** Creates a new Amplitudes descriptor, using the data provided to initialize it. 
			It will return an invalid descriptor (BufferManager::INVALID_ID) if no memory is 
			available to define the new descriptor.	*/
		virtual cl_uint createAmplitudesDescriptor(float* amplitudes, int numPosSamples){ 
			AmplitudesDescriptor* ad = new AmplitudesDescriptor(amplitudes, numPosSamples);
			amplitudesDescriptors[ad->getID()] = ad;
			return ad->getID();
		}

		/** Declares a new Primitive, using the positions and amplitudes descriptors provides.
			Primitives are initially disabled, so clients must explicitly call "setEnabled(true)" to start
			rendering it (this will actually start when a "commit" is called).
			Rendering of this primitive will start at the sample number indicated in "firstPosIndex" and "firstAmpIndex".
			It can fail if no more primitives can be declared (MAX_PRIMITIVES) or if the descriptors described do not exist,
			returning PBDEngine::INVALID_PRIMITIVE to indicate error. */
		virtual cl_uint declarePrimitive(cl_uint posDescriptor, cl_uint ampDescriptor, cl_uint firstPosIndex=0, cl_uint firstAmpIndex=0) { 
			cl_uint primitiveID = getPrimitiveID();
			if (primitiveID == PBDEngine::INVALID_PRIMITIVE_ID) {
				printWarning_PBDEngine("PrimitiveManager: Cannot declare a new primitive (is limit exceeded?).\n");
				return PBDEngine::INVALID_PRIMITIVE_ID;
			}
			if (positionsDescriptors.find(posDescriptor) == positionsDescriptors.end()) {
				printWarning_PBDEngine("PrimitiveManager: Cannot declare primitive (Positions descriptor does not exist).\n");
				return PBDEngine::INVALID_PRIMITIVE_ID;
			}
			if (amplitudesDescriptors.find(ampDescriptor) == amplitudesDescriptors.end()) {
				printWarning_PBDEngine("PrimitiveManager: Cannot declare primitive (Amplitudes descriptor does not exist).\n");
				return PBDEngine::INVALID_PRIMITIVE_ID;
			}
			//Declaring:
			pthread_mutex_lock(&(mutex));
			current[primitiveID - 1].primitiveID = primitiveID;
			current[primitiveID-1].curPosDescriptorID = current[primitiveID-1].nextPosDescriptorID = posDescriptor;
			current[primitiveID-1].curAmpDescriptorID = current[primitiveID-1].nextAmpDescriptorID = ampDescriptor;
			current[primitiveID-1].curPosIndex = current[primitiveID-1].curStartPosIndex = current[primitiveID-1].nextStartPosIndex = firstPosIndex;
			current[primitiveID-1].curAmpIndex = current[primitiveID-1].curStartAmpIndex = current[primitiveID-1].nextStartAmpIndex = firstAmpIndex;
			modified[primitiveID-1]=current[primitiveID-1];
			pthread_mutex_unlock(&(mutex));

			return primitiveID; 
		}
		
		/** Updates the matrices of a set of "num_primitives" primitives stored in array "primitives".
			The matrices are stored concequtively in arrays "mStarts" and "mEnds", and these will be immediately used for rendering.*/
		virtual void update_HighLevel(cl_uint* primitives, cl_uint num_primitives, float* mStarts, float* mEnds,  GSPAT::MatrixAlignment ma) { 
			for (cl_uint i = 0; i < num_primitives; i++) {
				//Update current				
				if (current[primitives[i]-1].primitiveID!=PBDEngine::INVALID_PRIMITIVE_ID) {
					pthread_mutex_lock(&(mutex));
					memcpy(&(current[primitives[i]-1].mStart[0]),&(mStarts[16 * i]), 16*sizeof(cl_float));
					memcpy(&(current[primitives[i]-1].mEnd[0]),&(mEnds[16 * i]), 16*sizeof(cl_float));					
					memcpy(&(modified[primitives[i]-1].mStart[0]),&(mStarts[16 * i]), 16*sizeof(cl_float));					
					memcpy(&(modified[primitives[i]-1].mEnd[0]),&(mEnds[16 * i]), 16*sizeof(cl_float));										
					pthread_mutex_unlock(&(mutex));								
				}else {
					printWarning_PBDEngine("PrimitiveManager: Updating undeclared primitive-> Ignoring command.\n");
				}
				// Notify listeners:
				std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
				for (; l != listeners.end(); l++)
					(*l)->primitiveUpdate_HighLevel(primitives, num_primitives, mStarts, mEnds);
			}
			return ; 
		}
		
		/** Updates the positions to use to render a given primitive to those specified in the new "PositionsDescriptor". 
			The rendering engine will swap to the new descriptor once the current descriptor is finished one full iteration 
			(i.e. it arrives to the last sample of the current descriptor). 
			Thus, the starting position of the new descriptor (3D position at "nextFirstPosIndex") should match that of the old descriptor (to ensure continuity).
			The command will be ignored if the primitive or the descriptor do not exist.
		*/
		virtual void updatePrimitive_Positions(cl_uint primitiveID, cl_uint nextPosDescriptor, cl_uint nextFirstPosIndex) { 
			if (modified[primitiveID - 1].primitiveID == PBDEngine::INVALID_PRIMITIVE_ID
				|| positionsDescriptors.find(nextPosDescriptor)==positionsDescriptors.end()) {
					printWarning_PBDEngine("PrimitiveManager: Cannot update primitive's Positions Descriptor (either primitive or descriptor do not exist)-> Ignoring command.\n");	
			}
			pthread_mutex_lock(&(mutex));				
			modified[primitiveID-1].nextPosDescriptorID = nextPosDescriptor; 
			modified[primitiveID-1].nextStartPosIndex = nextFirstPosIndex;
			pthread_mutex_unlock(&(mutex));				
		
			// Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->primitiveUpdate_LowLevel(primitiveID,modified[primitiveID-1]);
			return ; 
		}
		
		/** Updates the maplitudes to use to render a given primitive to those specified in the new "AmplitudesDescriptor". 
			The rendering engine will swap to the new descriptor once the current descriptor is finished one full iteration 
			(i.e. it arrives to the final sample of the current descriptor). 
			The command will be ignored if the primitive or the descriptor do not exist.
		*/
		virtual void updatePrimitive_Amplitudes(cl_uint primitiveID, cl_uint nextAmpDescriptor, cl_uint nextFirstAmpIndex) { 
			if (modified[primitiveID - 1].primitiveID == PBDEngine::INVALID_PRIMITIVE_ID
				|| amplitudesDescriptors.find(nextAmpDescriptor)==amplitudesDescriptors.end()) {
					printWarning_PBDEngine("PrimitiveManager: Cannot update primitive's Amplitudes Descriptor (either primitive or descriptor do not exist)-> Ignoring command.\n");	
			}
			pthread_mutex_lock(&(mutex));							
			modified[primitiveID-1].nextAmpDescriptorID = nextAmpDescriptor; 
			modified[primitiveID-1].nextStartAmpIndex = nextFirstAmpIndex;
			pthread_mutex_unlock(&(mutex));				

			// Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->primitiveUpdate_LowLevel(primitiveID,modified[primitiveID-1]);
			
			return ; 
		}
		
		/** Applies all the changes specified by the prior commands. This allows clients to setup the new state 
		(i.e. declare, enable/disable Primitives, Descriptors), and have them apply in an orderly fashion.
		Internally, double buffers get swapped (changes had been stored in "modified" database) and consistency between buffers is restored.*/
		virtual void commitUpdates(){ 
			
			//TODO: Make Thread safe
			//wait
			pthread_mutex_lock(&(mutex));
			Primitive*swap = current; 
			current = modified;
			modified = swap;
			currentCLBUfferIndex = 1 - currentCLBUfferIndex;
			memcpy(modified, current, MAX_PRIMITIVES * sizeof(Primitive));//Restere consistency
			pthread_mutex_unlock(&(mutex));
			//Update rendering targets (OpenCL const struct):
			primitiveTargets.numPrimitivesBeingRendered = (cl_uint)modifiedPrimitiveTargets.size();
			std::map<cl_uint, bool>::iterator it = modifiedPrimitiveTargets.begin();
			for (cl_uint p = 0; p < primitiveTargets.numPrimitivesBeingRendered; p++, it++)
				primitiveTargets.primitiveIDs[p] = it->first;
			BufferManager::instance().commitPendingCopyEvents();
			//release
			// Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->commit();

		}

		inline struct PrimitiveTargets& getPrimitiveTargets() {
			return this->primitiveTargets;
		}
		inline int getCurrentPrimitives(cl_mem* allPrimitives) {
			//DEBUG: We wait until writing ends... low performance
			/*cl_event dataCopied;
			cl_int err = clEnqueueWriteBuffer(context->queue, this->primitiveTargets_CLBuffer, CL_TRUE, 0, sizeof(struct PrimitiveTargets), &(this->primitiveTargets), 0, NULL, NULL);
			//err |= clEnqueueWriteBuffer(context->queue, this->allPrimitives_CLBuffer, CL_TRUE, 0, MAX_PRIMITIVES*sizeof(struct Primitive), current, 0, NULL, NULL);
			
			if (err != 0) {
				printWarning_PBDEngine("PrimitiveManager: Could not copy current primitive state to OpenCL buffers.");			
				return 0;
			}
			*primitiveTargets = this->primitiveTargets_CLBuffer;*/
			pthread_mutex_lock(&(mutex));
			clEnqueueUnmapMemObject(context->queue, allPrimitives_CLBuffer[currentCLBUfferIndex], current, 0, NULL, NULL);
			clEnqueueUnmapMemObject(context->queue, allPrimitives_CLBuffer[1-currentCLBUfferIndex], modified, 0, NULL, NULL);
			pthread_mutex_unlock(&(mutex));
			
			*allPrimitives = this->allPrimitives_CLBuffer[currentCLBUfferIndex];
			return this->primitiveTargets.numPrimitivesBeingRendered;
		}

		inline void getCurrentPrimitives(Primitive** c, Primitive** m ) {
			*c = current;
			*m = modified;
		}
		/**
			Notifies the manager that numSamples have just been rendered, which is used to update the 
			indexes of position and amplitude indexes to use next
		*/
		inline void postRender(int numSamples) {
			BufferManager& bm = BufferManager::instance();
			//We only update primitives being rendered.
			for (cl_uint p = 0; p< primitiveTargets.numPrimitivesBeingRendered; p++) {
				cl_uint primitiveIndex = primitiveTargets.primitiveIDs[p]-1;//Remember primitive IDS [1..MAX_PRIMITIVES], indexes [0..MAX_PRIMITIVES-1]
				Primitive& c = current[primitiveIndex];
				Primitive& m = current[primitiveIndex];
				pthread_mutex_lock(&(mutex));				
				
				cl_int curPosDescriptorSize = bm.getBufferDescriptor(c.curPosDescriptorID).numSamplesFloat / 4;
				cl_int curAmpDescriptorSize = bm.getBufferDescriptor(c.curAmpDescriptorID).numSamplesFloat ;
				cl_int nextPosDescriptorSize = bm.getBufferDescriptor(c.nextPosDescriptorID).numSamplesFloat / 4;
				cl_int nextAmpDescriptorSize = bm.getBufferDescriptor(c.nextAmpDescriptorID).numSamplesFloat ;
				//Update position indexes and ID to use				
				c.curPosIndex = m.curPosIndex = (c.curPosIndex + numSamples < curPosDescriptorSize?
					c.curPosIndex + numSamples :
					(c.nextStartPosIndex + c.curPosIndex + numSamples - curPosDescriptorSize) % nextPosDescriptorSize);
				c.curPosDescriptorID = m.curPosDescriptorID = (c.curPosIndex + numSamples < curPosDescriptorSize ?
					c.curPosDescriptorID : c.nextPosDescriptorID);
				//Amplitudes:
				c.curAmpIndex = m.curAmpIndex= (c.curAmpIndex + numSamples< curAmpDescriptorSize ?
					c.curAmpIndex + numSamples : 
					(c.nextStartAmpIndex+c.curAmpIndex + numSamples - curAmpDescriptorSize) % nextAmpDescriptorSize);
				c.curAmpDescriptorID =m.curAmpDescriptorID = (c.curAmpIndex + numSamples < curAmpDescriptorSize ?
					c.curAmpDescriptorID : c.nextAmpDescriptorID);
				//The steps in these matrices have already been taken. We can update now
				memcpy(c.mStart, c.mEnd, 16 * sizeof(cl_float));
				memcpy(m.mStart, m.mEnd, 16 * sizeof(cl_float));
				pthread_mutex_unlock(&(mutex));				

				
			}
			cl_int err;
			pthread_mutex_lock(&(mutex));

			current = (Primitive*)clEnqueueMapBuffer(context->queue, allPrimitives_CLBuffer[currentCLBUfferIndex], CL_TRUE, CL_MAP_WRITE, 0, MAX_PRIMITIVES * sizeof(struct Primitive), 0, NULL, NULL, &err);
			modified = (Primitive*)clEnqueueMapBuffer(context->queue, allPrimitives_CLBuffer[1-currentCLBUfferIndex], CL_TRUE, CL_MAP_WRITE, 0, MAX_PRIMITIVES * sizeof(struct Primitive), 0, NULL, NULL, &err);
			pthread_mutex_unlock(&(mutex));

		}
		/** Deletes an existing Positions descriptor, so that memory can be used for other purposes. 
		     Client is responsible for only deleting descriptors that are not in use.*/
		virtual bool releasePositionsDescriptor(cl_uint pd) {
			std::map<cl_uint, PositionsDescriptor*>::iterator it = positionsDescriptors.find(pd);
			if (it == positionsDescriptors.end()) {
				printWarning_PBDEngine("PrimitiveManager: Cannot release Positions Descriptor (does not exist).\n");				
				return false;
			}
			delete it->second;
			positionsDescriptors.erase(it);
			return true;
		}
		
		/** Deletes an existing Amplitudess descriptor, so that memory can be used for other purposes. 
		     Client is responsible for only deleting descriptors that are not in use.*/
		virtual bool releaseAmplitudesDescriptor(cl_uint ad) {
			std::map<cl_uint, AmplitudesDescriptor*>::iterator it = amplitudesDescriptors.find(ad);
			if (it == amplitudesDescriptors.end()) {
				printWarning_PBDEngine("PrimitiveManager: Cannot releaase Amplitudes Descriptor (does not exist).\n");				
				return false;
			}
			delete it->second;
			amplitudesDescriptors.erase(it);
			return true;	
		};

		/** Enables/disables existing primitives. Only enabled primitives will be rendered, but changes are not 
			applied until "commit" is invoked.		
		*/
		virtual bool setPrimitiveEnabled(cl_uint p, bool enabled=true) {
			if (modified[p - 1].primitiveID == PBDEngine::INVALID_PRIMITIVE_ID) {
				printWarning_PBDEngine("PrimitiveManager: Cannot enable/disable Primitive (does not exist).\n");				
				return false;
			}
			//Update the primitive targets (CPU database):
			std::map<cl_uint, bool>::iterator it = modifiedPrimitiveTargets.find(p);
			if (!enabled && it != modifiedPrimitiveTargets.end())
				modifiedPrimitiveTargets.erase(it);
			if(enabled)
				modifiedPrimitiveTargets[p] = true;
		};

		/** Destroys an existing primitive. Primitives should not be destroyed if they are currently being rendered.*/
		virtual bool releasePrimitive(cl_uint p) {
			//Check Primitive
			if (modified[p - 1].primitiveID == PBDEngine::INVALID_PRIMITIVE_ID) {
				printWarning_PBDEngine("PrimitiveManager: Cannot release Primitive (does not exist).\n");				
				return false;
			}
			//Disable (not renderable any more) and destroy
			setPrimitiveEnabled(false);
			modified[p - 1].primitiveID = PBDEngine::INVALID_PRIMITIVE_ID;
			// Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->primitiveReleased(p);
		}

		void addListener(PrimitiveUpdatesListener* l) {
			listeners.push_back(l);
		}
		void removeListener(PrimitiveUpdatesListener* l) {
			//Searh for it using an iterator:
			std::list<PrimitiveUpdatesListener*>::iterator it = listeners.begin();
			for (; it != listeners.end(); it++)
				if (*it == l)
					listeners.erase(it);	//Remove it			
		}
	};
};

#endif