#ifndef _GL_RENDERER
#define _GL_RENDERER
#define HAVE_STRUCT_TIMESPEC
#include <OpenMPD_Engine/include/OpenMPD.h>
#include <windows.h>
#include <pthread.h> 
#include <gl3w\gl3w.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
class GL_Renderer : public OpenMPD::IVisualRenderer{
	pthread_mutex_t mutex;
	size_t renderWindowSizeInSamples, numSamplesOffset;
	size_t curPointSize;
	//Shared OpenGL context:
	HDC device;
	HGLRC glContextData;
	GLuint positionBuffers[OpenMPD::MAX_BUFFERS];
	GLuint colourBuffers[OpenMPD::MAX_BUFFERS];
	size_t numFloatSamplesPerBuffer[OpenMPD::MAX_BUFFERS];
	//Description of current triangle strips to render:	
	struct MPD_RenderTarget {
		GLuint colourBuffer;
		GLuint positionBuffer;
		size_t c_Start, p_Start, numSamplesToRender;
		float M[16];
	} targets [OpenMPD::MAX_PRIMITIVES];

	struct OpenMPD::PrimitiveTargets currentTargets;
	
public:
	GL_Renderer(HDC device, HGLRC glContextData,int renderWindowSizeInSamples=320, int numSamplesOffset=0, void (*p)(char*)=NULL);
	virtual void createVisualBufferFromPositions(cl_uint bufferID, size_t numSamples, float* positions);
	virtual void releaseVisualBuffer(cl_uint bufferID);
	virtual void createVisualBufferFromColours(cl_uint bufferID, size_t numSamples, float* positions);
	virtual void updateTargets(OpenMPD::PrimitiveTargets& currentTargets, OpenMPD::Primitive currentPrimitives[OpenMPD::MAX_PRIMITIVES]);
	virtual void render(float* P, float* V);
	virtual void setPointSize(int size);
	virtual void setRenderOffsetInSamples(int offset);
	virtual ~GL_Renderer();
private: 
	//Drawing functionality: 
	GLuint m_Program;
	GLint m_UniformModelMatrix;
	GLint m_UniformViewMatrix;
	GLint m_UniformProjMatrix;
	GLint vertexPosition_modelspaceID;
	GLint vertexColour_modelspaceID;
	void (*_print)(char*);

	void initOpenGLResources();
	void renderStrip(float* M, float* V, float* P, GLuint pBuffer, GLuint cBuffer, size_t pStart, size_t c_Start, size_t numSamples);
	void print(char * msg) {
		if (_print)_print(msg);
	}
};

#endif 
