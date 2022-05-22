#ifndef _GSPAT_SOLVER
#define _GSPAT_SOLVER
#include <GSPAT_Solver_Prerequisites.h>
#include <GSPAT_Solution.h>
#include <glm/gtc/matrix_transform.hpp>		//GLM (matrices, vectors)
#include <vector>

namespace GSPAT {
	
	class _GSPAT_Export Solver {
	public:
		virtual ~Solver() { ; }

		/**
			Returns a data structure containing data related to the solver's execution context.
			Such context is dependent on the underlying implementation class, and the client must know how to parse it. 
			This enables the interoperability required to allow data to be uploaded to solutions externally.
		*/
		virtual void* getSolverContext() = 0;
		/**
			Configures the parameters required to allow the solver to discretise and produce the final messages to send to the board.
			(SEE EXPLANATION ABOUT THESE PARAMETERS IN METHOD:  Asierinho.h::readAdjustments())
		*/
		virtual void setBoardConfig(float* transducerPositions, int* transducerToPINMap, int* phaseAdjust, float* amplitudeAdjust, int numDiscreteLevels = 128) = 0;

		/**
			Returns a solution with the appropriate configuration, which the client can then use to compute the required transducer activation.
			- numPoints: Points in each geometry
			- numGeometries: Number of geometries to be computed in parallel
			- phaseOnly: Specifies if the solver should solve only phases (A=1, for all transducers) or phase and amplitude.
			- positions: buffer describing the positions of each point in each geometry. Each point must be described in homogeneous coordinates (x,y,z,1).
			   The buffer contains all "numPoints" points for Geometry 0, then "numPoints" points for Geometry 1, etc.
			- amplitudes: Specifies the amplitude of each point in each geometry. If "phaseOnly"==true, this can be in a homogemenous range (i.e. [0,1]).
			   In phase and amplitude is used, the target amplitudes must be described in Pascals.
			- mStart: Transformation matrix describing the position/orientation of each of the numPoints for the first geometry
			- mEnd: Transformation matrix describing the position/orientation of each of the numPoints for the last geometry
			NOTE: The solver only needs on matrix per point (NOT PER POINT x GEOMETRY)-> It interpolates intermediate matrices on its own.

		*/
		virtual Solution* createSolution(int numPoints, int numGeometries, bool phaseOnly, float* positions, float*amplitudes, float* matStarts, float* matEnds, GSPAT::MatrixAlignment a= GSPAT::MatrixAlignment::ColumnMajorAlignment) = 0;
		
		/**
			Returns a solution with the appropriate configuration, which the client can then use to compute the required transducer activation.
			- numPoints: Points in each geometry
			- numGeometries: Number of geometries to be computed in parallel
			- phaseOnly: Specifies if the solver should solve only phases (A=1, for all transducers) or phase and amplitude.
			Input data is uploaded to internal buffers externally (see methods Solution::dataBuffers, Solution::dataExternallyUploadedEvent); 

		*/
		virtual Solution* createSolutionExternalData(int numPoints, int numGeometries, bool phaseOnly, GSPAT::MatrixAlignment a)=0;
		
		/**
			Returns a solution turning transducers off.
		*/
		virtual Solution* createTransducersOffSolution()=0;
		/**
			Returns a solution setting a new FPS "divider" value (and turning transducers off).
			Dividers are used to ensure hardware controlled framerates, using 40KHz as the base frequency:
			    - Divider 1--> Update boards at 40KHz
				- Divider 2--> Update boards at 20KHz
				- Divider 3--> Update boards at 13.33KHz
				- Divider 4--> Update at 10KHz (most common)
		*/
		virtual Solution* createNewDividerSolution(unsigned char FPSdivider) = 0;
		/**
			Sends a solution to be computed by the GPU.
			This is an asynchronous method, returning immediately after the commands are delivered to the GPU.
			The client must then read the results using the methods in Solution (e.g. Solution::finalMessages())
			, but this will lock the client thread untill the GPU computation is finished.
			In the case data is provided externally (see createSolutionExternalData), the client should have followed these steps:
				- Retrieved buffers to store data to  (Solution::dataBuffers)
				- Triggered copying data to there buffers
				- Provide the event/s that notifies that data is ready to the Solution (see Solution::dataExternallyUploadedEvent).
				- Call GSPAT_Solver::compute(Solution* s).

		*/
		virtual void compute(Solution* solution) = 0;

		/**
			Releases the solution so that the solver can use its resources (e.g. GPU buffers) in the future.
			The client MUST ALWAYS release the solution once it has "compute"d and read the solution
			(i.e. the solver has a limited number of solutions that it needs to reuse.
			This is like a library, not a book shop:
				RETURN THE BOOK ONCE YOU FINISHED IT OR YOU WILL NOT GET ANY MORE BOOKS!!!
		*/
		virtual void releaseSolution(Solution* solution) = 0;
		/**
			Sends a solution to be computed by the GPU, and its usage is similar to "compute".
			However, the solver uses the pointHolograms (e.g. matrices F and B*) provided by the client.
			This is used for debugging purposes only
		*/
		//virtual void computeWithExternalPropagators(float* pointHologram, float* normalizedPointHologram, Solution* solution) = 0;
protected:
	/**
		This method allows a solver to destroy a given solution. Subclasses of Solver will be 
		able to create the adequate types (subclasses) of Solution that fit their pipeline. 
		However, given the declaration of Solution's destructor (protected), they would still not
		be able to delete them directly. 
		Subclasses of Solver will be able to delete their solutions by calling this method (allow 
		deleting derived Solutions). However, external clients of the DLL will not be able to delete
		them. This enforces Solutions being managed always through their solver.
	*/
	void destroySolution(Solution* s) {
		delete s;
	}
	};	
};
#endif