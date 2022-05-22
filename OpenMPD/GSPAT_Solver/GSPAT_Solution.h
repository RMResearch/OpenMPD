#ifndef _GSPAT_SOLUTION
#define _GSPAT_SOLUTION
#include <GSPAT_Solver_Prerequisites.h>
namespace GSPAT {
	class _GSPAT_Export Solution {
		friend class Solver;
	public:
		virtual void lockEvents()=0;
		virtual int geometries()=0;
		virtual int points() = 0;
		/**	Copies the solution's data buffers, so that external clients can upload data there directly. 
			Client must ensure that enough memory is available in the (output) array provided and must also know how
			to decode these buffers (e.g. cast to OpenCL buffers,if the underlying implemenation class uses OpenCL).  
			If NULL is provided as an argument, it returns the number of buffers required.	*/
		virtual int dataBuffers(void* pBuffers) = 0;
		/** Sets the events that will notify the solution that data uploading is finished. 
			The event provided must match the requirements of the implementation class (e.g. OpenCL event).*/
		virtual void dataExternallyUploadedEvent(void* pExternalEvents, int numEvents) = 0;
		/**
			Computes the messages to be sent to the boards and stores them in "out".
			This is all you need to use, if you are using GS_PAT. 
		*/
		virtual void finalMessages(unsigned char** out)=0;
		
		/**
			Returns the final array describing the phases of each transducer.
			This method is used for debugging purposes, and for backwards compatibility (e.g. clients that
			did not use the discretization in the GPU solver). 
			Please note that while the Levitation signature has been applied, no other discretization 
			steps are included (transducers mapping, phase adjustments).
		*/
		virtual float* finalArrayPhases()=0;
		/**
			Returns the final array describing the amplitudes of each transducer. This method is used
			for debugging purposes, and for backwards compatibility (e.g. clients that did not use the
			discretization in the GPU solver). 
			Please note that no discretization steps are included (transducers mapping, phase adjustments).
		*/
		virtual float* finalArrayAmplitudes()=0;
		/**
			Returns the final array describing the complex field (Re/Im for each transducer).
			NO LEVITATION SIGNATURE APPLIED-> So it is just a focusing hologram. 
			This method is retained for debugging purposes and can be used to visualize the fields. 
		*/
		virtual float* finalHologramReIm()=0;

		
		//Methods to read results from intermediate steps (mostly used for debugging)
		virtual void readPropagators(float* singlePointFields, float* singlePointFieldsNormalised, int numGeometries = 1)=0;
		virtual void readMatrixR(float* R, int numGeometries = 1)=0;
		virtual void readTargetPointsReIm(float* targetPoints, int numGeometries = 1)=0;
		virtual void releaseEvents() = 0;
	protected:
		/**
			Solutions should never be created directly by clients. This constructor prevents 
			such behaviour, and solutions will only be created using the factory method in our solvers. 
		*/
		Solution() { ; }
		/**
			The dstructor is protected, so that Solutions cannot be deleted by clients.
			Instead, destruction will always be managed by the solver that created the Solution.
			The destructor is virtual, so that derived classes are destroyed correctly.
		*/
		virtual ~Solution() { ; }
	};
};
#endif
