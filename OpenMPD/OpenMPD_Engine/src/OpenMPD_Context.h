#ifndef OPENMPD_CONTEXT
#define OPENMPD_CONTEXT
#include <include/OpenMPD.h>
#include <src\PrimitiveDescriptors.h>
#include "GSPAT_Solver_Prerequisites.h"
#include <OpenCLSolverImpl_Interoperability.h>
#include "Helper\OpenCLUtilityFunctions.h"
#include "src\PrimitiveUpdatesListener.h"
#include <map> 
#include <list>
#include <Windows.h>
#include <pthread.h>
#include <sstream>

namespace OpenMPD {
	class OpenMPD_Context : public IPrimitiveUpdater {

		/**Allows us to generate unique primitive IDs.*/
		cl_uint primitiveID_seed;
		int posIndex;
		/** The manager works on a dual buffer, containing two copies of the primitives defined in the system.
			These pointers contain the currently active version used to render, and 
			the version with changes which will become active when "commit" is called.			
		*/
		Primitive*  current, *modified;
		cl_mem primitives;
		/** The manager keeps a queue of descriptor updates, to apply them as current descriptors are 
			executed. If no other descriptors are in the queue, the manager loops the current descriptor*/
		struct _DescriptorUpdate { cl_uint descriptor; cl_uint startPos; };
		std::list<_DescriptorUpdate> curPosDescUpdates[MAX_PRIMITIVES], modPosDescUpdates[MAX_PRIMITIVES];
		std::list<_DescriptorUpdate> curAmpDescUpdates[MAX_PRIMITIVES], modAmpDescUpdates[MAX_PRIMITIVES];
		std::list<_DescriptorUpdate> curColDescUpdates[MAX_PRIMITIVES], modColDescUpdates[MAX_PRIMITIVES];

		struct OpenCL_ExecutionContext* context;
		pthread_mutex_t mutex; 
		/** Low level descriptors are stored in a single data base.
			Descriptors should not change while they are being used to render a Primitive (continuity cannot be assured).
			Only descriptors not currently under use should change (i.e. not assigned to an active Primitive). 
			Thus, these do not need to be double buffered*/
		std::map < cl_uint, PositionsDescriptor* > positionsDescriptors;
		std::map < cl_uint, AmplitudesDescriptor* > amplitudesDescriptors;
		std::map < cl_uint, ColoursDescriptor* > coloursDescriptors;

		/** Contains a definition of the Primitives to render, according to current state. 
			This definition remains constant until a new "commit" command is received. 
			This struct avoids unnecessary recomputation between "commits" and can be directly issued to OpenCL.*/
		struct PrimitiveTargets primitiveTargets;

		/**
			This map keeps track of the primitives that will become active (declared and enabled) once the next "commit" command arrives.
			It is used to produce the (constant) PrimitiveTargets used for rendering during "commit".
		*/
		std::map<cl_uint, bool> modifiedPrimitiveTargets;
		
		std::list<OpenMPD::PrimitiveUpdatesListener*> listeners;
		
		OpenMPD::IVisualRenderer* glRenderer = NULL;
		//Singleton related methods:
		OpenMPD_Context() {
			;
		}

	public: 
		/**Returns a Singleton to the current class. This interface exposes all functionality.*/
		static OpenMPD_Context& instance() {
			static OpenMPD_Context _instance;
			return _instance;
		}
		
		/**Returns a Singleton to the current class, to be used by clients.
		   This interface allows management of the Primitives and Descriptors, but does not provide control over
		   low level rendering aspects. */
		static IPrimitiveUpdater& primitiveUpdaterInstance() {
			return ((IPrimitiveUpdater&)instance());
		}
		
		void initialize(size_t memorySizeInFloats,OpenCL_ExecutionContext * context, IVisualRenderer* renderer=0) {
			if (current != NULL) {
				printWarning_OpenMPD("PrimitiveManager: Manager was already initialised. Commnad ignored.\n");
				return;
			}
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
			BufferManager::initialize(context, memorySizeInFloats);	
			pthread_mutex_init(&(mutex), NULL);
			primitiveID_seed = 1;
			////2. Create OpenCL buffers:
			cl_int err;
			this->context = context;
			primitives= clCreateBuffer(context->context, CL_MEM_READ_ONLY,  MAX_PRIMITIVES*sizeof(struct Primitive), NULL, &err);
			//2. Initialize Render_GL subsystem if necessary:
			if (renderer)
				glRenderer = renderer;

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
			} while (modified[curID-1].primitiveID != OpenMPD::INVALID_PRIMITIVE_ID && numAttempts++ <= MAX_PRIMITIVES);
			//Return the ID:
			if (numAttempts <= MAX_PRIMITIVES)
				return curID;
			//Otherwise, we are out of IDs...
			printWarning_OpenMPD("PrimitiveManager: Could not allocate a new primitive ID. Number of primitives declared is too high. ");
			return OpenMPD::INVALID_PRIMITIVE_ID;
		}

		/** Creates a new Positions descriptor, using the data provided to initialize it. 
			It will return an invalid descriptor (BufferManager::INVALID_ID) if no memory is 
			available to define the new descriptor.	*/
		virtual cl_uint createPositionsDescriptor(float* positions, int numPosSamples) { 
			PositionsDescriptor* pd = new PositionsDescriptor(positions, numPosSamples);
			positionsDescriptors[pd->getID()] = pd;
			std::stringstream msg;
			msg << "Primitive Manager: Positions descriptor " << pd->getID() << " created ("<<numPosSamples<<" samples).\n";
			printMessage_OpenMPD(msg.str().c_str());
			if(glRenderer)
				glRenderer->createVisualBufferFromPositions(pd->getID(), numPosSamples, positions);
			return pd->getID();
		}

		
		/** Creates a new Amplitudes descriptor, using the data provided to initialize it. 
			It will return an invalid descriptor (BufferManager::INVALID_ID) if no memory is 
			available to define the new descriptor.	*/
		virtual cl_uint createAmplitudesDescriptor(float* amplitudes, int numPosSamples){ 
			AmplitudesDescriptor* ad = new AmplitudesDescriptor(amplitudes, numPosSamples);
			amplitudesDescriptors[ad->getID()] = ad;
			std::stringstream msg;
			msg << "Primitive Manager: Amplitudes descriptor " << ad->getID() << " created ("<<numPosSamples<<" samples).\n";
			printMessage_OpenMPD(msg.str().c_str());
			return ad->getID();
		}

		/** Creates a new Colour descriptor, using the data provided to initialize it. 
			It will return an invalid descriptor (BufferManager::INVALID_ID) if no memory is 
			available to define the new descriptor.	*/
		virtual cl_uint createColoursDescriptor(float* coloursRGBA, int numColorSamples) { 
			ColoursDescriptor* cd = new ColoursDescriptor(coloursRGBA, numColorSamples);
			coloursDescriptors[cd->getID()] = cd;
			std::stringstream msg;
			msg << "Primitive Manager: Colours descriptor " << cd->getID() << " created ("<<numColorSamples<<" samples).\n";
			printMessage_OpenMPD(msg.str().c_str());
			if(glRenderer)
				glRenderer->createVisualBufferFromColours(cd->getID(), numColorSamples, coloursRGBA);
			return cd->getID();
		}

		void printSceneInfo() {
				std::ostringstream msg;
				msg << "PrimitiveManager::printSceneInfo:\n";
				msg << "Current Primitives:\n";
				for (int p = 0; p < MAX_PRIMITIVES; p++) {
					if (modified[p].primitiveID != OpenMPD::INVALID_PRIMITIVE_ID)
						msg << " Prim_" << modified[p].primitiveID << "; PosID_" << modified[p].curPosDescriptorID << "; AmpID_" << modified[p].curAmpDescriptorID << "\n";
				}
				msg << "Currend Descriptors:\n";
				msg << " Positions:";
				std::map<cl_uint, PositionsDescriptor*>::iterator itPos=positionsDescriptors.begin();
				for (; itPos != positionsDescriptors.end(); itPos++)
					msg << "P_" << itPos->second->getID() << "; ";
				msg << "\n Amplitudes:";				
				std::map<cl_uint, AmplitudesDescriptor*>::iterator itAmp = amplitudesDescriptors.begin();
				for (; itAmp != amplitudesDescriptors.end(); itAmp++)
					msg << "A_" << itAmp->second->getID() << "; ";
				printMessage_OpenMPD(msg.str().c_str());
		
		}

		/** Declares a new Primitive, using the positions and amplitudes descriptors provides (Colours are optional. Call updatePrimitive_Colours, if required).
			Primitives are initially disabled, so clients must explicitly call "setEnabled(true)" to start
			rendering it (this will actually start when a "commit" is called).
			Rendering of this primitive will start at the sample number indicated in "firstPosIndex" and "firstAmpIndex".
			It can fail if no more primitives can be declared (MAX_PRIMITIVES) or if the descriptors described do not exist,
			returning OpenMPD::INVALID_PRIMITIVE to indicate error. */
		virtual cl_uint declarePrimitive(cl_uint posDescriptor, cl_uint ampDescriptor, cl_uint firstPosIndex=0, cl_uint firstAmpIndex=0) { 
			cl_uint primitiveID = getPrimitiveID();
			if (primitiveID <= OpenMPD::INVALID_PRIMITIVE_ID || primitiveID > OpenMPD::MAX_PRIMITIVES) {
				printWarning_OpenMPD("PrimitiveManager: Cannot declare a new primitive (is limit exceeded?).\n");
				return OpenMPD::INVALID_PRIMITIVE_ID;
			}
			if (positionsDescriptors.find(posDescriptor) == positionsDescriptors.end()) {
				printWarning_OpenMPD("PrimitiveManager: Cannot declare primitive (Positions descriptor does not exist).\n");
				return OpenMPD::INVALID_PRIMITIVE_ID;
			}
			if (amplitudesDescriptors.find(ampDescriptor) == amplitudesDescriptors.end()) {
				printWarning_OpenMPD("PrimitiveManager: Cannot declare primitive (Amplitudes descriptor does not exist).\n");
				return OpenMPD::INVALID_PRIMITIVE_ID;
			}
			//Declaring:
			Primitive p;
			p.primitiveID = primitiveID;
			p.curPosDescriptorID = p.nextPosDescriptorID = posDescriptor;
			p.curAmpDescriptorID = p.nextAmpDescriptorID = ampDescriptor;
			p.curColDescriptorID = p.nextColDescriptorID = OpenMPD::INVALID_BUFFER_ID;
			p.curPosIndex = p.curStartPosIndex = p.nextStartPosIndex = firstPosIndex;
			p.curAmpIndex = p.curStartAmpIndex = p.nextStartAmpIndex = firstAmpIndex;
			p.curColIndex = p.curStartColIndex = p.nextStartColIndex = 0;
			
			
			std::stringstream msg;
			msg << "Primitive Manager: Primitive " << primitiveID << " declared-> PosDesc["<<posDescriptor<<"], AmpDesc["<<ampDescriptor<<"] \n";
			printMessage_OpenMPD(msg.str().c_str());	
			
			pthread_mutex_lock(&(mutex));
			modified[primitiveID - 1] = p;
			pthread_mutex_unlock(&(mutex));
			printSceneInfo();
			return primitiveID; 
		}
		
		/** Updates the matrices of a set of "num_primitives" primitives stored in array "primitives".
			The matrices are stored consecutively in arrays "mStarts" and "mEnds", and these will be immediately used for rendering.*/
		virtual void update_HighLevel(cl_uint* primitives, cl_uint num_primitives, float* mStarts, float* mEnds/*,  GSPAT::MatrixAlignment ma*/) { 
			for (cl_uint i = 0; i < num_primitives; i++) {
				//Update current				
				if (  current[primitives[i]-1].primitiveID > OpenMPD::INVALID_PRIMITIVE_ID
				   && current[primitives[i]-1].primitiveID <= OpenMPD::MAX_PRIMITIVES)
				{
					pthread_mutex_lock(&(mutex));
					//memcpy(&(current[primitives[i]-1].mStart[0]),&(mStarts[16 * i]), 16*sizeof(cl_float));
					memcpy(&(current[primitives[i]-1].mEnd[0]),&(mEnds[16 * i]), 16*sizeof(cl_float));					
					//memcpy(&(modified[primitives[i]-1].mStart[0]),&(mStarts[16 * i]), 16*sizeof(cl_float));					
					memcpy(&(modified[primitives[i]-1].mEnd[0]),&(mEnds[16 * i]), 16*sizeof(cl_float));										
					pthread_mutex_unlock(&(mutex));	

				}else {
					char msg[512];
					sprintf_s(msg, "PrimitiveManager: Updating undeclared primitive %d-> Ignoring command.\n", primitives[i]);
					printWarning_OpenMPD(msg);
					//printSceneInfo();
				}
			}
			// Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->primitiveUpdate_HighLevel(primitives, num_primitives, mStarts, mEnds);
						
			return ; 
		}
		
		/** Updates the positions to use to render a given primitive to those specified in the new "PositionsDescriptor". 
			The rendering engine will swap to the new descriptor once the current descriptor is finished one full iteration 
			(i.e. it arrives to the last sample of the current descriptor). 
			Thus, the starting position of the new descriptor (3D position at "nextFirstPosIndex") should match that of the old descriptor (to ensure continuity).
			The command will be ignored if the primitive or the descriptor do not exist.
		*/
		virtual void updatePrimitive_Positions(cl_uint primitiveID, cl_uint nextPosDescriptor, cl_uint nextFirstPosIndex) { 
			//1. Check if valid:
			if (primitiveID <= OpenMPD::INVALID_PRIMITIVE_ID ||primitiveID > OpenMPD::MAX_PRIMITIVES) {
				printWarning_OpenMPD("PrimitiveManager: Cannot enable/disable Primitive (Invalid ID).\n");				
				return ;
			}
			if (modified[primitiveID - 1].primitiveID == OpenMPD::INVALID_PRIMITIVE_ID
				|| positionsDescriptors.find(nextPosDescriptor)==positionsDescriptors.end()) {
					printWarning_OpenMPD("PrimitiveManager: Cannot update primitive's Positions Descriptor (either primitive or descriptor do not exist)-> Ignoring command.\n");	
			}
			//2. Update directly or enqueue
			pthread_mutex_lock(&(mutex));				
			//a. if it is currently looping, update straigthaway:
			if (modified[primitiveID - 1].nextPosDescriptorID == modified[primitiveID - 1].curPosDescriptorID) {
				modified[primitiveID - 1].nextPosDescriptorID = nextPosDescriptor;
				modified[primitiveID - 1].nextStartPosIndex = nextFirstPosIndex;
			}
			else {//b. Add to queue:
				_DescriptorUpdate d = { nextPosDescriptor, nextFirstPosIndex };
				modPosDescUpdates[primitiveID - 1].push_back(d);
			}
			pthread_mutex_unlock(&(mutex));		
			//3. Write to console (debug)
			std::stringstream msg;
			msg << "Primitive Manager: Primitive " << primitiveID << "-> PositionsDescriptor["<<nextPosDescriptor<<"], start: "<<nextFirstPosIndex<<" \n";
			printMessage_OpenMPD(msg.str().c_str());	
			//4.  Notify listeners:
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
			//1. Check if valid
			if (primitiveID <= OpenMPD::INVALID_PRIMITIVE_ID ||primitiveID > OpenMPD::MAX_PRIMITIVES) {
				printWarning_OpenMPD("PrimitiveManager: Cannot enable/disable Primitive (Invalid ID).\n");				
				return ;
			}
			if (modified[primitiveID - 1].primitiveID == OpenMPD::INVALID_PRIMITIVE_ID
				|| amplitudesDescriptors.find(nextAmpDescriptor)==amplitudesDescriptors.end()) {
					printWarning_OpenMPD("PrimitiveManager: Cannot update primitive's Amplitudes Descriptor (either primitive or descriptor do not exist)-> Ignoring command.\n");	
			}
			//2. Add or enqueue
			pthread_mutex_lock(&(mutex));
			//a. if it is currently looping, update straigthaway:
			if (modified[primitiveID - 1].nextAmpDescriptorID == modified[primitiveID - 1].curAmpDescriptorID) {
				modified[primitiveID - 1].nextAmpDescriptorID = nextAmpDescriptor;
				modified[primitiveID - 1].nextStartAmpIndex = nextFirstAmpIndex;
			}
			else {//b. Add to queue:
				_DescriptorUpdate d = { nextAmpDescriptor, nextFirstAmpIndex };
				modAmpDescUpdates[primitiveID - 1].push_back(d);
			}
			pthread_mutex_unlock(&(mutex));	
			//3. Print to console (debug)
			std::stringstream msg;
			msg << "Primitive Manager: Primitive " << primitiveID << "-> AmplitudeDescriptor["<<nextAmpDescriptor<<"], start: "<<nextFirstAmpIndex<<" \n";
			printMessage_OpenMPD(msg.str().c_str());	
			//4. Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->primitiveUpdate_LowLevel(primitiveID,modified[primitiveID-1]);
			
			return ; 
		}

		/** Updates the colours to use to render a given primitive to those specified in the new "ColoursDescriptor". 
			The rendering engine will swap to the new descriptor once the current descriptor is finished one full iteration 
			(i.e. it arrives to the last sample of the current descriptor). 
			Thus, the starting position of the new descriptor (3D position at "nextFirstPosIndex") should match that of the old descriptor (to ensure continuity).
			The command will be ignored if the primitive or the descriptor do not exist.
		*/
		virtual void updatePrimitive_Colours(cl_uint primitiveID, cl_uint nextColDescriptor, cl_uint nextFirstColIndex) { 
			//1. Check if valid:
			if (primitiveID <= OpenMPD::INVALID_PRIMITIVE_ID ||primitiveID > OpenMPD::MAX_PRIMITIVES) {
				printWarning_OpenMPD("OpenMPD_Context: Cannot enable/disable Primitive (Invalid ID).\n");				
				return ;
			}
			if (modified[primitiveID - 1].primitiveID == OpenMPD::INVALID_PRIMITIVE_ID
				|| coloursDescriptors.find(nextColDescriptor)==coloursDescriptors.end()) {
					printWarning_OpenMPD("OpenMPD_Context: Cannot update primitive's Colours Descriptor (either primitive or descriptor do not exist)-> Ignoring command.\n");	
			}
			//2. Update directly or enqueue
			pthread_mutex_lock(&(mutex));				
			//a. If it is unassigned (could be, they are optional)
			if (modified[primitiveID - 1].curColDescriptorID == INVALID_BUFFER_ID) {
				modified[primitiveID - 1].curColDescriptorID = modified[primitiveID - 1].nextColDescriptorID = nextColDescriptor;
				modified[primitiveID - 1].nextStartColIndex = nextFirstColIndex;
			
			}
			else if (modified[primitiveID - 1].nextColDescriptorID == modified[primitiveID - 1].curColDescriptorID) {
				//b. if it is currently looping, update straigthaway (once current finishes):
				modified[primitiveID - 1].nextColDescriptorID = nextColDescriptor;
				modified[primitiveID - 1].nextStartColIndex = nextFirstColIndex;
			}
			else {//c. Add to queue:
				_DescriptorUpdate d = { nextColDescriptor, nextFirstColIndex };
				modColDescUpdates[primitiveID - 1].push_back(d);
			}
			pthread_mutex_unlock(&(mutex));		
			//3. Write to console (debug)
			std::stringstream msg;
			msg << "Primitive Manager: Primitive " << primitiveID << "-> ColoursDescriptor["<<nextColDescriptor<<"], start: "<<nextFirstColIndex<<" \n";
			printMessage_OpenMPD(msg.str().c_str());	
			//4.  Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->primitiveUpdate_LowLevel(primitiveID,modified[primitiveID-1]);
			return ; 
		}
		/** Applies all the changes specified by the prior commands. This allows clients to setup the new state 
		(i.e. declare, enable/disable Primitives, Descriptors), and have them apply in an orderly fashion.
		Internally, double buffers get swapped (changes had been stored in "modified" database) and consistency between buffers is restored.*/
		virtual void commitUpdates(){ 
			std::stringstream msg;
			printMessage_OpenMPD("Primitive Manager: Commit message received!");
			msg<<"Primitive Manager: Commiting changes-> Current target Primitives";

			//Data in the primitives cannot be updated during rendering (getCurrentPrimitives-postRender).
			//This mutex prevents such access.
			pthread_mutex_lock(&(mutex));
			//1. Copy main primitive descriptions
			memcpy(current, modified, MAX_PRIMITIVES * sizeof(Primitive));
			BufferManager::instance().commitPendingCopyEvents();
			//2. Update pending descriptor updates (append to "current" and empty lists from "modified" version):
			for (int p = 0; p < MAX_PRIMITIVES; p++) {
				if (modAmpDescUpdates[p].size() > 0) {
					curAmpDescUpdates[p].splice(curAmpDescUpdates[p].end(), modAmpDescUpdates[p]);
					modAmpDescUpdates[p].clear();
				}
				if (modPosDescUpdates[p].size() > 0) {
					curPosDescUpdates[p].splice(curPosDescUpdates[p].end(), modPosDescUpdates[p]);
					modPosDescUpdates[p].clear();
				}
				if (modColDescUpdates[p].size() > 0) {
					curColDescUpdates[p].splice(curColDescUpdates[p].end(), modColDescUpdates[p]);
					modColDescUpdates[p].clear();
				}
			}
			//3.Update rendering targets (OpenCL const struct):
			primitiveTargets.numPrimitivesBeingRendered = (cl_uint)modifiedPrimitiveTargets.size();
			msg << primitiveTargets.numPrimitivesBeingRendered << " [";
			std::map<cl_uint, bool>::iterator it = modifiedPrimitiveTargets.begin();
			for (cl_uint p = 0; p < primitiveTargets.numPrimitivesBeingRendered; p++, it++) {
				primitiveTargets.primitiveIDs[p] = it->first;
				msg << primitiveTargets.primitiveIDs[p] ;
			}
			pthread_mutex_unlock(&(mutex));
			msg << "]\n";
			printMessage_OpenMPD(msg.str().c_str());	
			//4. Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->commit();

		}

		inline struct PrimitiveTargets& getPrimitiveTargets() {
			return this->primitiveTargets;
		}

		/**
			Returns current state of primitives. Locks database until postRender is called.
		*/
		inline cl_event getCurrentPrimitives(cl_mem* allPrimitives) {
			pthread_mutex_lock(&(mutex));//No commits are allowed between this method and end of render (postRender)
			cl_event event;
			clEnqueueWriteBuffer(context->queue, primitives, CL_FALSE, 0, OpenMPD::MAX_PRIMITIVES * sizeof(Primitive), current, 0, NULL, &event);
			*allPrimitives = primitives;
			return event;
		}

		inline void getCurrentPrimitives(Primitive** c, Primitive** m ) {
			*c = current;
			*m = modified;
		}
		
		// retrieve the current position index being rendered
		inline int getCurrentPosIndex() {
			return posIndex;
		}
		
		/**
			Notifies the manager that numSamples have just been rendered, which is used to update the 
			indexes of position, colour and amplitude indexes to use next
		*/
		inline void postRender(int numSamples) {
			//No commits were allowed between primitives are acquired (getCurrentPrimitives) and end of render. We allow commits now.
			pthread_mutex_unlock(&(mutex));
			BufferManager& bm = BufferManager::instance();
			//We only update primitives being rendered.
			for (cl_uint p = 0; p< primitiveTargets.numPrimitivesBeingRendered; p++) {
				cl_uint primitiveIndex = primitiveTargets.primitiveIDs[p]-1;//Remember primitive IDS [1..MAX_PRIMITIVES], indexes [0..MAX_PRIMITIVES-1]
				Primitive& c = current[primitiveIndex];
				Primitive& m = modified[primitiveIndex];
				pthread_mutex_lock(&(mutex));				
				
				cl_int curPosDescriptorSize = bm.getBufferDescriptor(c.curPosDescriptorID).numSamplesFloat / 4;
				cl_int curAmpDescriptorSize = bm.getBufferDescriptor(c.curAmpDescriptorID).numSamplesFloat ;
				cl_int nextPosDescriptorSize = bm.getBufferDescriptor(c.nextPosDescriptorID).numSamplesFloat / 4;
				cl_int nextAmpDescriptorSize = bm.getBufferDescriptor(c.nextAmpDescriptorID).numSamplesFloat ;
				
				{	//Update Positions
					cl_int curEndPosIndex_nonCyclic = c.curStartPosIndex + curPosDescriptorSize;
					cl_int curPos_nonCyclic = c.curPosIndex + (c.curPosIndex < c.curStartPosIndex)*curPosDescriptorSize;
					bool nextPosCycle = ((curPos_nonCyclic < curEndPosIndex_nonCyclic) && (curPos_nonCyclic + numSamples) >= curEndPosIndex_nonCyclic);
					cl_int posSamplesRemainingNextCycle = (curPos_nonCyclic + numSamples - c.curStartPosIndex) - curPosDescriptorSize;
					c.curPosIndex = m.curPosIndex =
						(1 - nextPosCycle)*((c.curPosIndex + numSamples) % curPosDescriptorSize)
						+ (nextPosCycle)*((c.nextStartPosIndex + posSamplesRemainingNextCycle) % nextPosDescriptorSize);
					c.curPosDescriptorID = m.curPosDescriptorID = (1 - nextPosCycle)*c.curPosDescriptorID + nextPosCycle*c.nextPosDescriptorID;
					c.curStartPosIndex = m.curStartPosIndex = (1 - nextPosCycle)*c.curStartPosIndex + nextPosCycle*c.nextStartPosIndex;
					//Check update queues and pick next descriptor:
					//printf("Pos(%d): start = %d; end=%d; cur = %d; %s\n", c.curPosDescriptorID, c.curStartPosIndex, curEndPosIndex_nonCyclic, c.curPosIndex, (nextPosCycle ? "TRUE" : "FALSE"));
					/*if(nextPosCycle)
						printWarning_OpenMPD("PrimitiveManager: Synchronized position transition.\n");
*/
					if (nextPosCycle && curPosDescUpdates[primitiveIndex].size() != 0) {
						_DescriptorUpdate& du = curPosDescUpdates[primitiveIndex].front();
						c.nextPosDescriptorID = m.nextPosDescriptorID = du.descriptor;
						c.nextStartPosIndex = m.nextStartPosIndex = du.startPos;
						curPosDescUpdates[primitiveIndex].pop_front();
					}
				}	

				{	//Update Amplitudes:
					cl_int curEndAmpIndex_nonCyclic = c.curStartAmpIndex + curAmpDescriptorSize;
					cl_int curAmp_nonCyclic = c.curAmpIndex + (c.curAmpIndex < c.curStartAmpIndex)*curAmpDescriptorSize;
					bool nextAmpCycle = ((curAmp_nonCyclic < curEndAmpIndex_nonCyclic) && (curAmp_nonCyclic + numSamples)  >= curEndAmpIndex_nonCyclic);
					cl_int ampSamplesRemainingNextCycle = (c.curAmpIndex + numSamples - c.curStartAmpIndex)-curAmpDescriptorSize;
					
					c.curAmpIndex = m.curAmpIndex =
						(1 - nextAmpCycle)*((c.curAmpIndex + numSamples) % curAmpDescriptorSize)
						+ (nextAmpCycle)*((c.nextStartAmpIndex + ampSamplesRemainingNextCycle) % nextAmpDescriptorSize);						
					c.curAmpDescriptorID = m.curAmpDescriptorID = 
						(1 - nextAmpCycle)*(c.curAmpDescriptorID) 
						+(nextAmpCycle)*(c.nextAmpDescriptorID);
					c.curStartAmpIndex =
						(1 - nextAmpCycle)*c.curStartAmpIndex
						+ (nextAmpCycle)*c.nextStartAmpIndex;	
					//Check update queues:
					//printf("Amp(%d): start = %d; end=%d; cur = %d; %s\n", c.curAmpDescriptorID, c.curStartAmpIndex, curEndAmpIndex_nonCyclic, c.curAmpIndex, (nextAmpCycle ? "TRUE" : "FALSE"));
					/*if(nextAmpCycle && curAmpDescriptorSize>4)
						printWarning_OpenMPD("PrimitiveManager: Synchronized amplitude transition.\n");*/

					if (nextAmpCycle && curAmpDescUpdates[primitiveIndex].size() != 0) {
						_DescriptorUpdate& du = curAmpDescUpdates[primitiveIndex].front();
						c.nextAmpDescriptorID = m.nextAmpDescriptorID = du.descriptor;
						c.nextStartAmpIndex = m.nextStartAmpIndex = du.startPos;
						curAmpDescUpdates[primitiveIndex].pop_front();
					}
				}
				// Update Colours
				if(c.curColDescriptorID != OpenMPD::INVALID_BUFFER_ID){	
					//Update Colours (if the primitive uses them):
					cl_int curColDescriptorSize = bm.getBufferDescriptor(c.curColDescriptorID).numSamplesFloat / 4;
					cl_int nextColDescriptorSize = bm.getBufferDescriptor(c.nextColDescriptorID).numSamplesFloat / 4;

					cl_int curEndColIndex_nonCyclic = c.curStartColIndex + curColDescriptorSize;
					cl_int curCol_nonCyclic = c.curColIndex + (c.curColIndex < c.curStartColIndex)*curColDescriptorSize;
					bool nextColCycle = ((curCol_nonCyclic < curEndColIndex_nonCyclic) && (curCol_nonCyclic + numSamples)  >= curEndColIndex_nonCyclic);
					cl_int ampSamplesRemainingNextCycle = (c.curColIndex + numSamples - c.curStartColIndex)-curColDescriptorSize;
					c.curColIndex = m.curColIndex =
						(1 - nextColCycle)*((c.curColIndex + numSamples) % curColDescriptorSize)
						+ (nextColCycle)*((c.nextStartColIndex + ampSamplesRemainingNextCycle) % nextColDescriptorSize);						
					c.curColDescriptorID = m.curColDescriptorID = 
						(1 - nextColCycle)*(c.curColDescriptorID) 
						+(nextColCycle)*(c.nextColDescriptorID);
					c.curStartColIndex =
						(1 - nextColCycle)*c.curStartColIndex
						+ (nextColCycle)*c.nextStartColIndex;	
					//Check update queues:
					if (nextColCycle && curColDescUpdates[primitiveIndex].size() != 0) {
						_DescriptorUpdate& du = curColDescUpdates[primitiveIndex].front();
						c.nextColDescriptorID = m.nextColDescriptorID = du.descriptor;
						c.nextStartColIndex = m.nextStartColIndex = du.startPos;
						curColDescUpdates[primitiveIndex].pop_front();
					}
				}				
				//The steps in these matrices have already been taken. We can update now
				//Add a matrixUpdated flag per primitive (mutex-protected).
				//If updated-> do nothing(it had already been updated) and set flag to false 
				// else apply this update:
				memcpy(c.mStart, c.mEnd, 16 * sizeof(cl_float));
				memcpy(m.mStart, m.mEnd, 16 * sizeof(cl_float));
				pthread_mutex_unlock(&(mutex));				
				//printf("Prim%d(pos%d, desc %d, cur%d,  next %d)  ;", c.primitiveID, c.curPosIndex, c.curPosDescriptorID,c.curStartPosIndex, c.nextStartPosIndex);				
			}
			pthread_mutex_lock(&(mutex));	
			if (glRenderer) 				
				glRenderer->updateTargets(this->primitiveTargets, current);
			pthread_mutex_unlock(&(mutex));	
			
			//printf("\n");
		}
		/** Deletes an existing Positions descriptor, so that memory can be used for other purposes. 
		     Client is responsible for only deleting descriptors that are not in use.*/
		virtual bool releasePositionsDescriptor(cl_uint pd) {
			std::map<cl_uint, PositionsDescriptor*>::iterator it = positionsDescriptors.find(pd);
			if (it == positionsDescriptors.end()) {
				printWarning_OpenMPD("PrimitiveManager: Cannot release Positions Descriptor (does not exist).\n");				
				return false;
			}
			delete it->second;
			positionsDescriptors.erase(it);
			if (glRenderer)
				glRenderer->releaseVisualBuffer(pd);
			std::stringstream msg;
			msg << "Primitive Manager: Positions descriptor " << pd << " released.\n";
			printMessage_OpenMPD(msg.str().c_str());	
			return true;
		}
		
		/** Deletes an existing Amplitudess descriptor, so that memory can be used for other purposes. 
		     Client is responsible for only deleting descriptors that are not in use.*/
		virtual bool releaseAmplitudesDescriptor(cl_uint ad) {
			std::map<cl_uint, AmplitudesDescriptor*>::iterator it = amplitudesDescriptors.find(ad);
			if (it == amplitudesDescriptors.end()) {
				printWarning_OpenMPD("PrimitiveManager: Cannot release Amplitudes Descriptor (does not exist).\n");				
				return false;
			}
			delete it->second;
			amplitudesDescriptors.erase(it);
			std::stringstream msg;
			msg << "Primitive Manager: Amplitudes descriptor " << ad << " released.\n";
			printMessage_OpenMPD(msg.str().c_str());	
			return true;	
		};

		/** Deletes an existing Coulour descriptor, so that memory can be used for other purposes. 
		     Client is responsible for only deleting descriptors that are not in use.*/
		virtual bool releaseColoursDescriptor(cl_uint cd) {
			std::map<cl_uint, ColoursDescriptor*>::iterator it = coloursDescriptors.find(cd);
			if (it == coloursDescriptors.end()) {
				printWarning_OpenMPD("OpenMPD_Context: Cannot release Positions Descriptor (does not exist).\n");				
				return false;
			}
			delete it->second;
			coloursDescriptors.erase(it);
			if (glRenderer)
				glRenderer->releaseVisualBuffer(cd);
			std::stringstream msg;
			msg << "Primitive Manager: Colours descriptor " << cd << " released.\n";
			printMessage_OpenMPD(msg.str().c_str());	
			return true;
		}

		/** Enables/disables existing primitives. Only enabled primitives will be rendered, but changes are not 
			applied until "commit" is invoked.		
		*/
		virtual bool setPrimitiveEnabled(cl_uint p, bool enabled=true) {
			if (p <= OpenMPD::INVALID_PRIMITIVE_ID || p > OpenMPD::MAX_PRIMITIVES) {
				printWarning_OpenMPD("PrimitiveManager: Cannot enable/disable Primitive (Invalid ID).\n");				
				return false;
			}

			pthread_mutex_lock(&(mutex));
			if (modified[p - 1].primitiveID == OpenMPD::INVALID_PRIMITIVE_ID) {
				printWarning_OpenMPD("PrimitiveManager: Cannot enable/disable Primitive (does not exist).\n");				
				return false;
			}
			//Update the primitive targets (CPU database):
			std::map<cl_uint, bool>::iterator it = modifiedPrimitiveTargets.find(p);
			if (!enabled && it != modifiedPrimitiveTargets.end())
				modifiedPrimitiveTargets.erase(it);
			if(enabled)
				modifiedPrimitiveTargets[p] = true;
			pthread_mutex_unlock(&(mutex));
			std::stringstream msg;
			msg << "Primitive Manager: Primitive " << p << (enabled? " enabled.\n": "disabled.\n");
			printMessage_OpenMPD(msg.str().c_str());	
			return true;
		};

		/** Destroys an existing primitive. Primitives should not be destroyed if they are currently being rendered.*/
		virtual bool releasePrimitive(cl_uint p) {
			//Check Primitive
			if (p <= OpenMPD::INVALID_PRIMITIVE_ID ||p > OpenMPD::MAX_PRIMITIVES) {
				printWarning_OpenMPD("PrimitiveManager: Cannot enable/disable Primitive (Invalid ID).\n");				
				return false;
			}
			if (modified[p - 1].primitiveID == OpenMPD::INVALID_PRIMITIVE_ID) {
				printWarning_OpenMPD("PrimitiveManager: Cannot release Primitive (does not exist).\n");				
				return false;
			}
			//Disable (not renderable any more) and destroy
			setPrimitiveEnabled(p,false);
			modified[p - 1].primitiveID = OpenMPD::INVALID_PRIMITIVE_ID;
			std::stringstream msg;
			msg << "Primitive Manager: Primitive " << p << "released.\n";
			printMessage_OpenMPD(msg.str().c_str());	
			// Notify listeners:
			std::list<PrimitiveUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->primitiveReleased(p);
			return true;
		}

		void addListener(PrimitiveUpdatesListener* l) {
			listeners.push_back(l);
		}
		void removeListener(PrimitiveUpdatesListener* l) {
			//Searh for it using an iterator:
			std::list<PrimitiveUpdatesListener*>::iterator it = listeners.begin();
			for (; it != listeners.end(); it++)
				if (*it == l) {
					listeners.erase(it);	//Remove it			
					return;
				}
		}

		void releaseAllResources() {
			if (current == NULL)
				return;
			//Relese OpenCL	resources
			clReleaseMemObject(primitives);
			pthread_mutex_destroy(&(mutex));
			_aligned_free(current); current = NULL;
			_aligned_free(modified); modified = NULL;
			//Release all primitives and descriptors
			this->primitiveTargets.numPrimitivesBeingRendered = 0;
			std::map<cl_uint, PositionsDescriptor*>::iterator it;
			for (it = positionsDescriptors.begin(); it != positionsDescriptors.end(); it++)
				delete it->second;
			this->positionsDescriptors.clear();
			std::map<cl_uint, AmplitudesDescriptor*>::iterator it2;
			for (it2 = amplitudesDescriptors.begin(); it2 != amplitudesDescriptors.end(); it2++)
				delete it2->second;
			this->amplitudesDescriptors.clear();
			std::map<cl_uint, ColoursDescriptor*>::iterator it3;
			for (it3 = coloursDescriptors.begin(); it3 != coloursDescriptors.end(); it3++)
				delete it3->second;
			this->coloursDescriptors.clear();
			BufferManager::destroy();
			//Remove listeners
			this->listeners.clear();
			this->modifiedPrimitiveTargets.clear();
			this->primitiveID_seed = 1;
			//The renderer is generated by an external component, so it is not our responsibility to delete it. 
			if (glRenderer) {
				delete glRenderer; glRenderer = NULL;
			}
			printMessage_OpenMPD("Primitive Manager: All primitives, descriptors and buffers released.\n");	

		}
	};
};

#endif