#include "GL_Renderer.h"
#include <stdio.h>

char consoleLine[256];

GL_Renderer::GL_Renderer(HDC device, HGLRC glContextData,int renderWindowSizeInSamples, int numSamplesOffset, void(*p)(char *))
	:device(device)
	, renderWindowSizeInSamples(renderWindowSizeInSamples)
	, numSamplesOffset(numSamplesOffset)
	, glContextData(glContextData)
	, curPointSize(5)
	, _print(p)
{
	pthread_mutex_init(&mutex, NULL);
	memset(positionBuffers, 0, OpenMPD::MAX_BUFFERS * sizeof(GLuint));
	memset(colourBuffers, 0, OpenMPD::MAX_BUFFERS * sizeof(GLuint));
	memset(numFloatSamplesPerBuffer, 0, OpenMPD::MAX_BUFFERS * sizeof(size_t));
	currentTargets.numPrimitivesBeingRendered = 0;
	initOpenGLResources();
}
GL_Renderer::~GL_Renderer() {
	print("GL_Renderer::~GL_Renderer()");
	//if(wglGetCurrentContext()!=glContextData)
	//	wglMakeCurrent(device, glContextData);
	//for (int b = 0; b != OpenMPD::MAX_BUFFERS; b++)
	//	if (positionBuffers[b] != 0) {
	//		sprintf_s(consoleLine, "Releasing Position Buffer ID %d", b + 1); print(consoleLine);
	//		positionBuffers[b] = 0;

			//glDeleteBuffers(1, &(positionBuffers[b]));
	//	}
}
void GL_Renderer::createVisualBufferFromPositions(cl_uint bufferID,size_t numSamples, float * positions)
{
	print("GL_Renderer::createVisualBuffer()");
	if(wglGetCurrentContext()!=glContextData)
		wglMakeCurrent(device, glContextData);
	//1.Adapt data (We copy the descriptor, plus the renderWindowSizeInSamples. Any cyclic range will fit in this new array)
	size_t samplesAllocated=(numSamples+renderWindowSizeInSamples);
	numFloatSamplesPerBuffer[bufferID - 1] = 4 * numSamples;	
	float* tempBuffer = new float[4*samplesAllocated];
	//Copy the initial descriptor
	memcpy(tempBuffer, positions, numSamples * (4 * sizeof(float)));
	//Fill the extra space (renderWindowSizeInSamples) with copies from the descriptor
	size_t numSamplesRemaining =(size_t) renderWindowSizeInSamples;
	size_t curOffset = numSamples * (4);//Used as a pointer inside the buffer (where we need to write)
	while (numSamplesRemaining>0)
		if (numSamplesRemaining > numSamples) {
			//The hole descriptor fits...make a completer copy
			memcpy(&(tempBuffer[curOffset]), positions, numSamples *(4 * sizeof(float)));
			numSamplesRemaining -= numSamples;
			curOffset += 4 * numSamples;
		}
		else {
			//Partial copy
			memcpy(&(tempBuffer[curOffset]), positions, numSamplesRemaining *(4 * sizeof(float)));
			numSamplesRemaining -= numSamplesRemaining;//Zero...end of loop
			curOffset += 4 * numSamples;
		}
	//2.Allocate buffer:
	glGenBuffers(1, &(positionBuffers[bufferID-1]));		
	//sprintf(consoleLine, "GL_Renderer: OpenGL Buffer ID: %p created\n", positionBuffers[bufferID-1]);
	//print(consoleLine);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffers[bufferID-1]);
	glBufferData(GL_ARRAY_BUFFER, samplesAllocated* (4*sizeof(GLfloat)), tempBuffer, GL_DYNAMIC_DRAW);
	delete tempBuffer;
	glFinish();

}

void GL_Renderer::releaseVisualBuffer(cl_uint bufferID) {
	print("GL_Renderer::releaseVisualBuffer()");

	if (positionBuffers[bufferID-1] != 0) {
		glDeleteBuffers(1, &(positionBuffers[bufferID]));
		positionBuffers[bufferID-1] = 0;	
		numFloatSamplesPerBuffer[bufferID - 1] = 0;
	}
}

void GL_Renderer::createVisualBufferFromColours(cl_uint bufferID,size_t numSamples, float * colours)
{
	print("GL_Renderer::createVisualBuffer()");
	if(wglGetCurrentContext()!=glContextData)
		wglMakeCurrent(device, glContextData);
	//1.Adapt data (We copy the descriptor, plus the renderWindowSizeInSamples. Any cyclic range will fit in this new array)
	size_t samplesAllocated=(numSamples+renderWindowSizeInSamples);
	numFloatSamplesPerBuffer[bufferID - 1] = 4 * numSamples;	
	float* tempBuffer = new float[4*samplesAllocated];
	//Copy the initial descriptor
	memcpy(tempBuffer, colours, numSamples * (4 * sizeof(float)));
	//Fill the extra space (renderWindowSizeInSamples) with copies from the descriptor
	size_t numSamplesRemaining =(size_t) renderWindowSizeInSamples;
	size_t curOffset = numSamples * (4);//Used as a pointer inside the buffer (where we need to write)
	while (numSamplesRemaining>0)
		if (numSamplesRemaining > numSamples) {
			//The hole descriptor fits...make a completer copy
			memcpy(&(tempBuffer[curOffset]), colours, numSamples *(4 * sizeof(float)));
			numSamplesRemaining -= numSamples;
			curOffset += 4 * numSamples;
		}
		else {
			//Partial copy
			memcpy(&(tempBuffer[curOffset]), colours, numSamplesRemaining *(4 * sizeof(float)));
			numSamplesRemaining -= numSamplesRemaining;//Zero...end of loop
			curOffset += 4 * numSamples;
		}
	//2.Allocate buffer:
	glGenBuffers(1, &(colourBuffers[bufferID-1]));		
	//sprintf(consoleLine, "GL_Renderer: OpenGL Buffer ID: %p created\n", positionBuffers[bufferID-1]);
	//print(consoleLine);
	glBindBuffer(GL_ARRAY_BUFFER, colourBuffers[bufferID-1]);
	glBufferData(GL_ARRAY_BUFFER, samplesAllocated* (4*sizeof(GLfloat)), tempBuffer, GL_DYNAMIC_DRAW);
	delete tempBuffer;
	glFinish();
}

void GL_Renderer::updateTargets(OpenMPD::PrimitiveTargets & currentTargets, OpenMPD::Primitive currentPrimitives[OpenMPD::MAX_PRIMITIVES])
{
	print("GL_Renderer::updateTargets()");
	pthread_mutex_lock(&mutex);
	this->currentTargets = currentTargets;
	for (cl_uint p = 0; p < currentTargets.numPrimitivesBeingRendered; p++) {
		//1. ACCESS selected Primitive and RenderTarget
		cl_uint primitiveID = currentTargets.primitiveIDs[p];
		struct MPD_RenderTarget& renderTarget = targets[primitiveID - 1];
		OpenMPD::Primitive& primitive = currentPrimitives [primitiveID - 1];
		//2. UPDATE RenderTarget according to Primitive:
		renderTarget.positionBuffer = primitive.curPosDescriptorID;
		renderTarget.colourBuffer = primitive.curColDescriptorID;
		renderTarget.numSamplesToRender = renderWindowSizeInSamples;
		memcpy(renderTarget.M, primitive.mEnd, 16 * sizeof(float));
		
		//3.Compute slice of the posBuffer that we need to render (assuming GL_LINE_STRIP OR GL_POINTS).
		{
			size_t numPositionsInBuffer = numFloatSamplesPerBuffer[primitive.curPosDescriptorID - 1] / 4;
			size_t startSample, endSample;

			//Rendering reveals only a section... WE CAN SELECT THIS SECTION FROM OUR (CYCLE+ numSamplesOffset) BUFFERS
			if (primitive.curPosIndex + numSamplesOffset >= 0) {
				startSample = primitive.curPosIndex + numSamplesOffset;
				endSample = startSample + renderWindowSizeInSamples;
			}
			else {
				startSample = numPositionsInBuffer + primitive.curPosIndex + numSamplesOffset;
				endSample = startSample + renderWindowSizeInSamples;
			}
			sprintf_s(consoleLine, "P%d (%lld - %lld);\t ", primitiveID, startSample, endSample); print(consoleLine);
			renderTarget.p_Start = startSample;
			renderTarget.numSamplesToRender = renderWindowSizeInSamples;
		}
		//4.Compute slice of the colBuffer that we need to render (assuming GL_LINE_STRIP).
		{
			size_t numColoursInBuffer = numFloatSamplesPerBuffer[primitive.curPosDescriptorID - 1] / 4;
			size_t startSample, endSample;

			//Rendering reveals only a section... WE CAN SELECT THIS SECTION FROM OUR (CYCLE+ numSamplesOffset) BUFFERS
			if (primitive.curColIndex + numSamplesOffset >= 0) {
				startSample = primitive.curColIndex + numSamplesOffset;
				endSample = startSample + renderWindowSizeInSamples;
			}
			else {
				startSample = numColoursInBuffer + primitive.curColIndex + numSamplesOffset;
				endSample = startSample + renderWindowSizeInSamples;
			}
			sprintf_s(consoleLine, "P%d (%lld - %lld);\t ", primitiveID, startSample, endSample); print(consoleLine);
			renderTarget.c_Start = startSample;			
		}
	}
	print("\n");
	pthread_mutex_unlock(&mutex);
}


const char* vertex = "#version 150\n"																\
	"in highp vec4 pos;\n"										\
	"in lowp vec4 color;\n"										\
	"\n"															\
	" out lowp vec4 ocolor;\n"									\
	"\n"															\
	"uniform highp mat4 modelMatrix;\n"								\
	"uniform highp mat4 viewMatrix;\n"								\
	"uniform highp mat4 projMatrix;\n"								\
	"\n"															\
	"void main()\n"													\
	"{\n"															\
	"	gl_Position = (projMatrix * viewMatrix * modelMatrix) * pos;\n"	\
	"	ocolor = color;\n"											\
	"}\n";

const char* fragment = "#version 150\n"				\
	"out lowp vec4 fragColor;\n"					\
	"in lowp vec4 ocolor;\n"						\
	"\n"											\
	"void main()\n"									\
	"{\n"											\
	"	fragColor = ocolor;\n"						\
	"}\n";

void GL_Renderer::initOpenGLResources()
{
	print("GL_Renderer::initOpenGLResources()");
	gl3wInit();
	//wglMakeCurrent(device, glContext);
	//print("GL_Renderer::initOpenGLResources(): glContext made current.");
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertex, NULL);
	glCompileShader(vertexShader);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragment, NULL);
	glCompileShader(fragmentShader);
	// Link shaders into a program and find uniform locations
	m_Program = glCreateProgram();
	glBindAttribLocation(m_Program, 0, "pos");
	glBindAttribLocation(m_Program, 1, "color");
	glAttachShader(m_Program, vertexShader);
	glAttachShader(m_Program, fragmentShader);
	glBindFragDataLocation(m_Program, 0, "fragColor");
	glLinkProgram(m_Program);
	GLint status = 0;
		glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
	m_UniformModelMatrix = glGetUniformLocation(m_Program, "modelMatrix");
	m_UniformViewMatrix = glGetUniformLocation(m_Program, "viewMatrix");
	m_UniformProjMatrix = glGetUniformLocation(m_Program, "projMatrix");
	vertexPosition_modelspaceID = glGetAttribLocation(m_Program, "pos");
	vertexColour_modelspaceID = glGetAttribLocation(m_Program, "color");
	print("GL_Renderer::initOpenGLResources(): shaders compiled without errors.");
	sprintf_s(consoleLine, "m_Program: %d [ M: %d; V: %d; P: %d ]", m_Program, m_UniformModelMatrix, m_UniformViewMatrix, m_UniformProjMatrix); print(consoleLine);
}

void GL_Renderer::render(float* P, float* V)
{
	pthread_mutex_lock(&mutex);
	print("GL_Renderer::render()");
	for (cl_uint p = 0; p < currentTargets.numPrimitivesBeingRendered; p++) {
		//Read primitive ID:
		cl_uint primitiveID = currentTargets.primitiveIDs[p];
		//Check if it has a colour ID (i.e., needs rendering)
		if (targets[primitiveID - 1].colourBuffer == 0)
			continue;
		//Read data and render
		GLuint pBuffer = positionBuffers[targets[primitiveID - 1].positionBuffer-1];
		GLuint cBuffer = colourBuffers[targets[primitiveID - 1].colourBuffer-1];
		size_t pStart = targets[primitiveID - 1].p_Start;
		size_t cStart = targets[primitiveID - 1].c_Start;
		size_t numSamples = targets[primitiveID - 1].numSamplesToRender;
		float* M = targets[primitiveID - 1].M;
		float M_traspose[] = { M[0],	M[4],	M[8],	M[12],
								M[1],	M[5],	M[9],	M[13],
								M[2],	M[6],	M[10],	M[14],
								M[3],	M[7],	M[11],	M[15] };
		//Render:
		renderStrip(M_traspose, V, P, pBuffer, cBuffer, pStart, cStart, numSamples);
	}
	pthread_mutex_unlock(&mutex);
}

void GL_Renderer::setPointSize(int size)
{
	curPointSize = size;
}

void GL_Renderer::setRenderOffsetInSamples(int offset)
{
	this->numSamplesOffset = offset;
}

void GL_Renderer::renderStrip(float* M, float* V, float* P, GLuint pBuffer, GLuint cBuffer, size_t p_Start, size_t c_Start, size_t numSamples) {
	print("GL_Renderer::renderStrip()");
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	if (pBuffer == 0) {
		print("GL_Renderer::renderStrip() ERROR-> Attempting to draw uninitialized buffer.");
		return;
	}
	// Setup shader program to use, and the matrices
	glUseProgram(m_Program);
	glUniformMatrix4fv(m_UniformModelMatrix, 1, GL_FALSE, M);
	glUniformMatrix4fv(m_UniformViewMatrix, 1, GL_FALSE, V);
	glUniformMatrix4fv(m_UniformProjMatrix, 1, GL_FALSE, P);
	//Config the positions buffer
	glEnableVertexAttribArray(vertexPosition_modelspaceID);
	glBindBuffer(GL_ARRAY_BUFFER, pBuffer);
	glPointSize(1.0f*curPointSize);
	glVertexAttribPointer(
		vertexPosition_modelspaceID,  // The attribute we want to configure
		4,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)(4*p_Start*sizeof(float))       // array buffer offset
	); 
	//Config the colours buffer
	glEnableVertexAttribArray(vertexColour_modelspaceID);
	glBindBuffer(GL_ARRAY_BUFFER, cBuffer);
	glPointSize(1.0f*curPointSize);
	glVertexAttribPointer(
		vertexColour_modelspaceID,		// The attribute we want to configure
		4,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)(4*c_Start*sizeof(float))       // array buffer offset
	); 
	// Draw
	glDrawArrays(GL_POINTS, 0,(GLsizei)numSamples);
	print("GL: End render triangles.");
}


