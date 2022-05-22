#include "RenderResources.h"

cl_mem positions_CL, colours_CL;
static const size_t numSamples = 320, pointsPerSample = 2, floatsPerPoint = 3;
void declareBuffers(cl_context context, cl_command_queue  queue) {
	float positions[numSamples* pointsPerSample*floatsPerPoint];
	float colours[numSamples* pointsPerSample*floatsPerPoint];

	for (int s = 0; s < numSamples; s++) {
		float pointA[] = { -1 + (2.0f*s) / numSamples, 0.1f, 0 };
		float pointB[] = { -1 + (2.0f*s) / numSamples, -0.1f, 0 };
		float colour[] = { (s / 10) / 32.0f, 0, 0 };
		positions[s*pointsPerSample*floatsPerPoint] = pointA[0];
		positions[s*pointsPerSample*floatsPerPoint+1] = pointA[1];
		positions[s*pointsPerSample*floatsPerPoint+2] = pointA[2];
		positions[(s*pointsPerSample+1)*floatsPerPoint] = pointB[0];
		positions[(s*pointsPerSample+1)*floatsPerPoint+1] = pointB[1];
		positions[(s*pointsPerSample+1)*floatsPerPoint+2] = pointB[2];

		colours[s*pointsPerSample*floatsPerPoint] = colour[0];
		colours[s*pointsPerSample*floatsPerPoint+1] = colour[1];
		colours[s*pointsPerSample*floatsPerPoint+2] = colour[2];
		colours[(s*pointsPerSample+1)*floatsPerPoint] = colour[0];
		colours[(s*pointsPerSample+1)*floatsPerPoint+1] = colour[1];
		colours[(s*pointsPerSample+1)*floatsPerPoint+2] = colour[2];
	}
	int error;
	positions_CL= clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, numSamples* pointsPerSample*floatsPerPoint* sizeof(float), positions, &error);	
	colours_CL= clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, numSamples* pointsPerSample*floatsPerPoint* sizeof(float), colours, &error);
	/* //READ THEM (Chekc they are correct)
	float positions2[numSamples* pointsPerSample*floatsPerPoint];
	float colours2[numSamples* pointsPerSample*floatsPerPoint];
	clEnqueueReadBuffer(queue, positions_CL, CL_TRUE, 0, numSamples* pointsPerSample*floatsPerPoint * sizeof(float), positions2, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, colours_CL, CL_TRUE, 0, numSamples* pointsPerSample*floatsPerPoint * sizeof(float), colours2, 0, NULL, NULL);

	for (int i = 0; i < 10; i++)
		positions[i] += 0;

	*/
}
size_t getBuffers(cl_mem* vertexBuffer, cl_mem* colourBuffer) {
	*vertexBuffer = positions_CL;
	*colourBuffer = colours_CL;
	return numSamples* pointsPerSample*floatsPerPoint;
}