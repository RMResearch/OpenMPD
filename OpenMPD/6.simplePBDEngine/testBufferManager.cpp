#include "./GSPAT_RenderingEngine/src/BufferManager.h"
#include "./GSPAT_RenderingEngine/src/PrimitiveDescriptors.h"
#include "./GSPAT_RenderingEngine/src/PrimitiveManager.h"
#include <GSPAT_Solver.h>
#include <AsierInho.h>
#include <OpenCLSolverImpl_Interoperability.h>


void printMessage_PBDEngine(const char* msg) { printf("%s\n", msg); }
void printError_PBDEngine(const char* msg){ printf("%s\n", msg); }
void printWarning_PBDEngine(const char* msg){ printf("%s\n", msg); }


GSPAT::Solver* initGSPAT() {
//Create driver and connect to it
	AsierInho::RegisterPrintFuncs(printMessage_PBDEngine, printMessage_PBDEngine, printMessage_PBDEngine);	
	AsierInho::AsierInhoBoard* driver= AsierInho::createAsierInho();
	//Create solver:
	GSPAT::RegisterPrintFuncs(printMessage_PBDEngine, printMessage_PBDEngine, printMessage_PBDEngine);
	GSPAT::Solver* solver = GSPAT::createSolver(32, 16);
	//while(!driver->connect(AsierInho::BensDesign, 3, 6))
	//	printf("Failed to connect to board.");
	driver->connect(AsierInho::BensDesign, 3, 6);
	int mappings[512], phaseDelays[512];
	driver->readAdjustments(mappings, phaseDelays);
	solver->setBoardConfig(mappings, phaseDelays);
	//Program: Create a trap and move it with the keyboard
	float curPos[] = { 0,0,0.1f , 1}, amplitude =1;
	unsigned char* msgTop, *msgBottom;
	float m1[] = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
	//a. create a solution
	GSPAT::Solution* solution = solver->createSolution(1,1,true, curPos, &amplitude,m1,m1);
	solver->compute(solution); 
	solution->finalMessages(&msgTop, &msgBottom);
	driver->updateMessage(msgBottom, msgTop, 1);
	solver->releaseSolution(solution);
	return solver;
}
void main() {
	GSPAT::Solver* solver = initGSPAT();
	struct OpenCL_ExecutionContext * context = (struct OpenCL_ExecutionContext *)solver->getSolverContext();
	PBDEngine::PrimitiveManager::instance().initialize(context, 2000000);//2Mfloats (64MB?)
	printf("Size of float: %zd\n", sizeof(cl_float));
	float p1_data[] = { 1,1,1,1	,2,2,2,2, 3,3,3,3,	4,4,4,4 };
	float p2_data[] = { 5,5,5,5,	6,6,6,6,	7,7,7,7,	8,8,8,8 };
	float a1_data[] = { 1.0f,1.1f, 1.2f, 1.3f }, a2_data[] = { 2.0f, 2.1f, 2.2f };
	PBDEngine::IPrimitiveUpdater& pm=PBDEngine::PrimitiveManager::primitiveUpdaterInstance();
	//Create descriptors
	cl_uint pos1=pm.createPositionsDescriptor(p1_data, 4);
	cl_uint pos2=pm.createPositionsDescriptor(p2_data, 4);
	cl_uint amp1=pm.createAmplitudesDescriptor(a1_data, 4);
	cl_uint amp2=pm.createAmplitudesDescriptor(a2_data, 3);
	//Create Primitives
	cl_uint pri1 = pm.declarePrimitive(pos1, amp1);
	cl_uint pri2 = pm.declarePrimitive(pos2, amp2);
	pm.commitUpdates();
	pm.setPrimitiveEnabled(pri1,true);
	pm.setPrimitiveEnabled(pri2,true);
	pm.commitUpdates();
	cl_uint pri_IDs[]={ pri1,pri2 };
	float mStarts[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
	float mEnds[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
	
	//Test external input uploading:
	//1. Create kernel:
	{
		
		//Configure input
		PBDEngine::PrimitiveManager& pm2=PBDEngine::PrimitiveManager::instance();
		cl_mem primitives, memory, bufferDescriptors;
		cl_uint numGeometries = 2;
		cl_uint numPoints=2;
		//Create program
		cl_program program;
		OpenCLUtilityFunctions::createProgramFromFile("PBDEngine.cl", context->context, context->device, &program);
		cl_int err;
		cl_kernel fillDataFromPrimitives = clCreateKernel(program, "fillDataFromPrimitives2", &err);
		if (err < 0) {
			printWarning_PBDEngine("Couldn't create kernel\n");
		}
		//Create output buffers
		cl_mem positions_CL, amplitudes_CL, mStarts_CL, mEnds_CL, initialGuess_CL;
		positions_CL = clCreateBuffer(context->context, CL_MEM_READ_WRITE, numGeometries*numPoints*4*sizeof(cl_float), NULL, NULL);
		amplitudes_CL = clCreateBuffer(context->context, CL_MEM_READ_WRITE, numGeometries*numPoints*sizeof(cl_float), NULL, NULL);
		mStarts_CL = clCreateBuffer(context->context, CL_MEM_READ_WRITE, numPoints*16*sizeof(cl_float), NULL, NULL);
		mEnds_CL = clCreateBuffer(context->context, CL_MEM_READ_WRITE, numPoints*16*sizeof(cl_float), NULL, NULL);
		initialGuess_CL= clCreateBuffer(context->context, CL_MEM_READ_WRITE, numPoints*2*sizeof(cl_float), NULL, NULL);
		//Configure parameters (kernel args)
		while (true) {
			for (int i = 0; i < 32; i++)mEnds[i]++;
			pm.update_HighLevel(pri_IDs, 2, mStarts, mEnds,GSPAT::MatrixAlignment::ColumnMajorAlignment);
			pm.commitUpdates();


			numPoints = pm2.getCurrentPrimitives(&primitives);
			PBDEngine::PrimitiveTargets& targets = pm2.getPrimitiveTargets();
			std::vector<cl_event> *committedEvents;
			PBDEngine::BufferManager::instance().getBufferDescriptors(&memory, &bufferDescriptors, &committedEvents);
		
			//size_t aux = sizeof(struct PBDEngine::PrimitiveTargets);
			err |= clSetKernelArg(fillDataFromPrimitives, 0, sizeof(PBDEngine::PrimitiveTargets), &(targets));
			err |= clSetKernelArg(fillDataFromPrimitives, 1, sizeof(cl_mem), &(primitives));
			err |= clSetKernelArg(fillDataFromPrimitives, 2, sizeof(cl_mem), &(bufferDescriptors));
			err |= clSetKernelArg(fillDataFromPrimitives, 3, sizeof(cl_mem), &(memory));
			err |= clSetKernelArg(fillDataFromPrimitives, 4, sizeof(cl_mem), &(positions_CL));
			err |= clSetKernelArg(fillDataFromPrimitives, 5, sizeof(cl_mem), &(amplitudes_CL));
			err |= clSetKernelArg(fillDataFromPrimitives, 6, sizeof(cl_mem), &(initialGuess_CL));
			err |= clSetKernelArg(fillDataFromPrimitives, 7, sizeof(cl_mem), &(mStarts_CL));
			err |= clSetKernelArg(fillDataFromPrimitives, 8, sizeof(cl_mem), &(mEnds_CL));
			if (err < 0) { printMessage_PBDEngine("computeFandB::Couldn't set kernel argument 0"); return; }
			//Run kernel:
			size_t global_size[2] = { numPoints, numGeometries };
			size_t local_size[2] = { numPoints, numGeometries };
			cl_event finishedUpload;
			if(committedEvents && committedEvents->size()!=0)
				err |= clEnqueueNDRangeKernel(context->queue, fillDataFromPrimitives, 2, NULL, global_size, local_size,  committedEvents->size(), committedEvents->data(), &(finishedUpload));
			else 
				err |= clEnqueueNDRangeKernel(context->queue, fillDataFromPrimitives, 2, NULL, global_size, local_size, 0, NULL, &(finishedUpload));
						
			pm2.postRender(numGeometries);
			PBDEngine::BufferManager::instance().releasePendingCommitedCopies(committedEvents);
			if (err < 0) { 
				printMessage_PBDEngine("computeFandB:: Couldn't enqueue the kernel");
				return; }

			//Read positions
			float positions[128], amplitudes[32], mStartsOut[32], mEndsOut[32], initialGuess[4];
			err = clEnqueueReadBuffer(context->queue, positions_CL, CL_TRUE, 0, numPoints*numGeometries * 4 * sizeof(float), &(positions), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, amplitudes_CL, CL_TRUE, 0, numPoints*numGeometries * sizeof(float), &(amplitudes), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, initialGuess_CL, CL_TRUE, 0, numPoints * 2 * sizeof(float), &(initialGuess), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, mStarts_CL, CL_TRUE, 0, numPoints * 16 * sizeof(float), &(mStartsOut), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, mEnds_CL, CL_TRUE, 0, numPoints * 16 * sizeof(float), &(mEndsOut), 1, &(finishedUpload), NULL);
			struct PBDEngine::Primitive allPrims[32];
			PBDEngine::BufferDescriptor allBufferDesc[256];
			err = clEnqueueReadBuffer(context->queue, primitives, CL_TRUE, 0, 32 * sizeof(struct PBDEngine::Primitive), &(allPrims), 1, &(finishedUpload), NULL);
			err = clEnqueueReadBuffer(context->queue, bufferDescriptors, CL_TRUE, 0, 256 * sizeof(struct PBDEngine::BufferDescriptor), &(allBufferDesc), 1, &(finishedUpload), NULL);
			for (int i = 0; i < numPoints*numGeometries; i++)
				printf("P %.1f; G %.1f; NP %.1f; Pri %.1f;\n", positions[4 * i], positions[4 * i + 1], positions[4 * i + 2], positions[4 * i + 3]);
			printf("Done\n");
		}
	}




}
/*int main() {
	PBDEngine::BufferManager::initialise(1000);
	PBDEngine::BufferManager& bufferManager = PBDEngine::BufferManager::instance();
	
	PBDEngine::Buffer &b1 = bufferManager.createBuffer(200);
	PBDEngine::Buffer &b2 = bufferManager.createBuffer(200);
	PBDEngine::Buffer &b3 = bufferManager.createBuffer(200);
	bufferManager.releaseBuffer(b2);
	bufferManager.releaseBuffer(b3);
	//PBDEngine::Buffer &b4=bufferManager.createBuffer(800);

	PBDEngine::PositionsDescriptor p(NULL, 200);
	bufferManager.releaseBuffer(b2);//Does nothing (already destroyed)
	PBDEngine::PositionsDescriptor p2(NULL, 50);//Returns invallid buffer (no space)
	bufferManager.releaseBuffer(b1);//Empties
	PBDEngine::PositionsDescriptor p3(NULL, 40);//Returns valid buffer.
	PBDEngine::AmplitudesDescriptor a3(NULL, 40);//Returns valid buffer.
	PBDEngine::PrimitiveManager::instance();
}*/

