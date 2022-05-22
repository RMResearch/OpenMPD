#ifndef OpenMPD_BUFFER_MANAGER
#define OpenMPD_BUFFER_MANAGER
#include "./OpenMPD_Engine/include/OpenMPD_Prerequisites.h"
#include "./OpenMPD_Engine/src/Buffer.h"
#include "./OpenMPD_Engine/src/BufferUpdatesListener.h"
#include <OpenCLSolverImpl_Interoperability.h>
#include <map>
#include <list>
#include <Windows.h>
#include <vector>
#include <sstream>

namespace OpenMPD {

	class BufferManager
	{
	private:
		static BufferManager* _instance;
		cl_uint ID_seed;
		//TODO: MOVE TO CONSISTENCY MANAGER
		OpenCL_ExecutionContext* context;
		cl_mem buffer_CL;
		cl_mem bufferDescriptors_CL;
		/**The manager keeps all pending operations in a buffer, until "commit" is called*/
		std::vector<cl_event>* currentPendingCopies; 
		/**The operations accumulated are than copied to pendingCommittedCopies, which must be completed
		before the render manager can compute any more solutions.*/
		std::vector<cl_event>* pendingCommittedCopies;

		//END TODO
		size_t totalMemory;
		std::map<cl_uint,Buffer*> usedGaps, emptyGaps;
		BufferDescriptor* bufferDescriptors;
		bool bufferDescriptorsGPUDirty;
		Buffer errorBuffer;
		std::list<OpenMPD::BufferUpdatesListener*> listeners;
		static cl_uint instanceCounter;
		/**Private constructor. This type of objects can only be constructed calling instance().*/
		BufferManager(struct OpenCL_ExecutionContext* context,size_t size) :totalMemory(size),context(context), ID_seed(1), errorBuffer(0,0,INVALID_BUFFER_ID) { 
			setupInternalMemory();
			currentPendingCopies = new std::vector<cl_event>; currentPendingCopies->reserve(MAX_BUFFERS);
			pendingCommittedCopies = new std::vector<cl_event>; pendingCommittedCopies->reserve(MAX_BUFFERS);
			instanceCounter++;
		}
		~BufferManager() {
			releaseAllResources();
			instanceCounter--;
		}
	protected: 
		virtual void setupInternalMemory() {
			Buffer* wholeMemory = new Buffer(0, (cl_uint)totalMemory, getValidID());
			emptyGaps[wholeMemory->getID()]=wholeMemory; 
			//Create page aligned array for the descriptors:
			SYSTEM_INFO systemInfo;
			GetSystemInfo(&systemInfo);
			bufferDescriptors =(BufferDescriptor*) _aligned_malloc(MAX_BUFFERS*sizeof(BufferDescriptor), systemInfo.dwPageSize); //new Primitive[MAX_PRIMITIVES];
			memset(bufferDescriptors, 0, MAX_BUFFERS * sizeof(BufferDescriptor));

			// OpenCL initialization
			buffer_CL= clCreateBuffer(context->context, CL_MEM_READ_ONLY,  totalMemory * sizeof(float), NULL, NULL);
			//bufferDescriptors_CL = clCreateBuffer(context->context, CL_MEM_USE_HOST_PTR |CL_MEM_READ_ONLY,  MAX_BUFFERS * sizeof(BufferDescriptor), bufferDescriptors, NULL);
			bufferDescriptors_CL = clCreateBuffer(context->context, CL_MEM_READ_ONLY,  MAX_BUFFERS * sizeof(BufferDescriptor), NULL, NULL);//Requires manual writing on commit
			bufferDescriptorsGPUDirty = true;
		}
		void releaseAllResources() {
			if (currentPendingCopies == NULL )
				return;
			//Release all pending events:
			releasePendingCommitedCopies(currentPendingCopies); currentPendingCopies = NULL;
			releasePendingCommitedCopies(pendingCommittedCopies); pendingCommittedCopies = NULL;
			//Release OpenCL buffers:
			clReleaseMemObject(this->bufferDescriptors_CL);
			clReleaseMemObject(this->buffer_CL);
			//Release main memory:
			_aligned_free( this->bufferDescriptors); bufferDescriptors = NULL;
			std::map<cl_uint, Buffer*>::iterator it;
			for (it = emptyGaps.begin(); it != emptyGaps.end(); it++)
				delete it->second;
			for (it = usedGaps.begin(); it != usedGaps.end(); it++)
				delete it->second;	
			this->listeners.clear();
		}
	public: 
		//METHODS RELATED TO SINGLETON
		cl_uint getValidID() {
			cl_uint ID;
			//Find an unused ID.
			cl_uint numAttempts = MAX_BUFFERS;
			do {
				ID = ID_seed;
				ID_seed = (ID_seed)%MAX_BUFFERS + 1;
			} while (usedGaps.find(ID) != usedGaps.end() && numAttempts-- >= 0);
			if(numAttempts>=0)
				return ID;
			return INVALID_BUFFER_ID;
		}

		static bool initialize(struct OpenCL_ExecutionContext* context, size_t memorySizeInFloats) {
			if (_instance != NULL) {
				printWarning_OpenMPD("BufferManager had already been initialized. Command ignored");
				return false;
			}
			_instance = new BufferManager(context, memorySizeInFloats);
			return true;
		}

		static bool destroy() {
			if (_instance == NULL) {
				printWarning_OpenMPD("BufferManager had already been destroyed. Command ignored");
				return false;
			}
			delete _instance;
			_instance = NULL;
			return true;
		}
		static bool isInitialised() {
			return _instance != NULL;
		}
		static BufferManager& instance() {
			if (!isInitialised())
				printError_OpenMPD("BufferManager not initialized. Cannot return instance.");
			return *_instance;  			
		}
		//FUNCTIONALITY
		Buffer& createBuffer(cl_uint dataSize) {
			//0. Make sure memory stays aligned to (at least) 4 floats (GPU words are 4 floats wide)
			cl_uint size = dataSize + (dataSize % 4 == 0 ? 0 : 4 - (dataSize % 4));
			//1. Traverse all empty gaps
			std::map<cl_uint, Buffer*>::iterator itGaps;
			for (itGaps = emptyGaps.begin(); itGaps != emptyGaps.end(); itGaps++) {
				//1.1. If gap is large enough
				if (itGaps->second->size() >= size) {
					//Split into two (to be used and whatever remains), and destroy.
					Buffer *used, *remain;
					itGaps->second->split(size, &used, &remain);
					//Remove previous gap from list of gaps
					delete itGaps->second;					
					emptyGaps.erase(itGaps);
					//Add alocated buffer to used buffers
					usedGaps[used->getID()] = used;
					bufferDescriptors[used->getID() - 1].start = used->getStart();
					bufferDescriptors[used->getID() - 1].numSamplesFloat= dataSize;
					bufferDescriptorsGPUDirty = true;
					//Add remaining space to empty gaps (unless gap is empty)
					if (remain->size() > 0)
						emptyGaps[remain->getID()] = remain;
					else
						delete remain;
					//Notify listeners:
					std::list<BufferUpdatesListener*>::iterator l = listeners.begin();
					for (; l != listeners.end(); l++)
						(*l)->bufferCreated(used->getID(), used->getStart(), used->getEnd());
					//Return succesfully:	
					std::stringstream msg;
					msg << "BufferManager: Created buffer " << used->getID() <<" ["<<size<< " floats]\n";
					printMessage_OpenMPD(msg.str().c_str());
					return *used;
				}
			}
			//2. If no gaps available, return error.
			return errorBuffer;
		}
		/** Returns true if the buffer corresponds to a currently used gap in memory.*/
		inline bool isValid(cl_uint id) {			
			return (usedGaps.find(id)!= usedGaps.end());
		}
		/**
			Fills the buffer 'id' with the data provided (enough data to fill the whole buffer must be provided).
			This is triggered as an asynchronous operation, which must be completed by the time a render operation is 
			performed after client call "commit".
		*/
		bool fillData(cl_uint id, float* data) {
			if (!isValid(id)) {
				printWarning_OpenMPD("BufferManager: Cannot fill that buffer (does not exist). Command ignored");
				return false;
			}
			Buffer* b = usedGaps.find(id)->second;
			//TODO: Move this responsibility to ConsistencyManager 
			//Copy all data in the OpenCL buffer (Only register event if *notifyEvent!=NULL). 
			cl_event dataCopied;
			cl_int err = clEnqueueWriteBuffer(context->queue, buffer_CL, CL_FALSE, b->getStart()*sizeof(cl_float), b->size()* sizeof(cl_float), data, 0, NULL, &dataCopied);
			if (err != 0) {
				printWarning_OpenMPD("BufferManager: Error filling buffer.");
				return false;
			}
			std::stringstream msg;
			msg << "BufferManager: Filling buffer " << id << " with " << b->size() << " floats\n";
			printMessage_OpenMPD(msg.str().c_str());
			this->currentPendingCopies->push_back(dataCopied);
			//END TODO:
			//Notify listeners:
			std::list<BufferUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->bufferFilled(id,data,b->size());
			//Notify success		
			return true;
		}

		bool releaseBuffer(OpenMPD::Buffer& b) {
			return releaseBuffer(b.getID());
		}

		bool releaseBuffer(cl_uint id){
			//1. Remove buffer from used buffers.
			//1.1. Check b is a valid buffer (must be in used buffers). 
			if (!isValid(id))
				return false;
			//1.2. Remove it from used buffers
			std::map<cl_uint, Buffer*>::iterator it=usedGaps.find(id);
			Buffer* buffer = it->second;
			usedGaps.erase(it);
			//2. Add the buffer to the list of available gaps (join to existing gaps if possible). 
			_releaseAndMerge(buffer);

			std::stringstream msg;
			msg << "BufferManager: Releasing buffer " << id << "\n";
			printMessage_OpenMPD(msg.str().c_str());
			
			//3. Notify listeners:
			std::list<BufferUpdatesListener*>::iterator l = listeners.begin();
			for (; l != listeners.end(); l++)
				(*l)->bufferReleased(id);

			return true;		
		}

		/*inline void getBufferDescriptors(BufferDescriptor** descriptors, cl_uint* size) {
			*descriptors = &(bufferDescriptors[0]);
			*size = MAX_BUFFERS;
		}*/

		/**
			This method must be called when the client issues a "commit" event.
			All events related to pending copy/fillData operations are stored at a temporary buffer
			and returned to the rendering engine, so that it waits for data transfers to be finished
			before computing any more solutions. 		*/
		void commitPendingCopyEvents() {
			if (bufferDescriptorsGPUDirty) {
				cl_event bufferDescriptorsUpdated;
				cl_int err = clEnqueueWriteBuffer(context->queue, bufferDescriptors_CL, CL_FALSE, 0, MAX_BUFFERS * sizeof(BufferDescriptor), bufferDescriptors, 0, NULL, &bufferDescriptorsUpdated);
				currentPendingCopies->push_back(bufferDescriptorsUpdated);
				bufferDescriptorsGPUDirty = false;
			}

			if (pendingCommittedCopies->size() != 0) {
				pendingCommittedCopies->insert(pendingCommittedCopies->end(), currentPendingCopies->begin(), currentPendingCopies->end());
				printWarning_OpenMPD("Buffer events for commited operations seem to have not been attended. ");
			}
			else {
				pendingCommittedCopies = currentPendingCopies;
			}
			currentPendingCopies = new std::vector<cl_event>;
			currentPendingCopies->reserve(MAX_BUFFERS);
		}

		/**
			This method is called by the rendering engine each time a solution is to be computed.
			It provides access to the OpenCL buffers managing our memory layout (buffers and buffersDescriptors)
			and a vector with the transfers that need to be completed before rendeering starts (pendingCommittedCopies).
			Please note that most of these transfers should be complete already (they have been issued asynchronously by the client).
			The vector of events must be released after rendering by calling releasePendingCommitedCopies. 
		*/
		inline bool getBufferDescriptors(cl_mem* buffers, cl_mem* buffersDescriptors, std::vector<cl_event>** pendingCommittedCopies) {
			*buffersDescriptors =bufferDescriptors_CL;
			*buffers = this->buffer_CL;
			*pendingCommittedCopies = this->pendingCommittedCopies;
			this->pendingCommittedCopies = new std::vector<cl_event>;
			return true;
		}

		inline BufferDescriptor& getBufferDescriptor(cl_uint id) {
			return bufferDescriptors[id-1];//IDs go from 1..MAX_BUFFERS, but indexes go from 0..MAX_BUFFERS-1
		}
		/**
			Releases the vector of pending operations received by the rendering engine during getBuffersDescriptors.
		*/
		void releasePendingCommitedCopies(std::vector<cl_event>* pendingCommittedCopies) {
			for (int e = 0; e < pendingCommittedCopies->size(); e++)
				clReleaseEvent(pendingCommittedCopies->operator[](e));
			delete pendingCommittedCopies;			
		}
		void addListener(BufferUpdatesListener* l) {
			listeners.push_back(l);
		}
		void removeListener(BufferUpdatesListener* l) {
			//Searh for it using an iterator:
			std::list<BufferUpdatesListener*>::iterator it = listeners.begin();
			for (; it != listeners.end(); it++)
				if (*it == l)
					listeners.erase(it);	//Remove it			
		}


private: 
		/**
			Releases a valid buffer, merging it to any adjacent gaps.
			If merging to a gap is possible, it does this and continues to iterate to check if the 
			new (merged) gap can be merged to others (iterative implementation of a recursive method).
		*/
		void _releaseAndMerge(Buffer* buffer) {
			bool merged;
			//1. Merge with all possible gaps.
			do {
				merged = false;
				//1.Traverse all empty gaps
				std::map<cl_uint, Buffer*>::iterator it; 
				for (it = emptyGaps.begin(); it != emptyGaps.end(); it++) {
					//1.1. If gap b is adjacent to any existing gap
					if (it->second->adjacent(*buffer)) {
						//a. Merge gaps
						Buffer *oldGap=it->second, *mergedGap;
						mergedGap = Buffer::merge(buffer, oldGap);
						//b. Remove previous (smaller) gaps
						emptyGaps.erase(it);
						delete oldGap;
						delete buffer;
						//c. Check if new (merged) gap can be merged to others (next iteration).
						buffer = mergedGap;
						merged = true;
						break;
					}
				}
			} while (merged);
			//2. Add resulting to list of empty gaps.
			emptyGaps[buffer->getID()] = buffer;
		}

	};
};
#endif
