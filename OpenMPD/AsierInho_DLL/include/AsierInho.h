#ifndef _ASIER_INHO
#define _ASIER_INHO
#include <AsierInho_Prerequisites.h>
/**
This project contains a simple driver to communicate with out Acoustophoretic board (our PAT).
This provides direct control over each transducer’s phase and amplitude, as well as access to
our MATD mode (high speed computation of single levitation traps, with amplitude and illumination control).
The module is encapsulated into a DLL, which you can use through interfaces in either C++ or C. 
This namespace contains a pure virtual interface (AsierInhoBoard), which cleints can use to control the boards.
It also contains an abstract factory, to allow creation of board controllers (AsierInhoBoards)
without exposing implementation details; as well as other utility functions for the DLL to print
notification, warning and error messages. 
*/
namespace AsierInho {
	class _AsierInho_Export AsierInhoBoard {
	protected:
		/**
			AsierInho's cannot be created directly, but you should use the method:
				AsierInho::createAsierInho(BoardType boardType, int bottomBoardID, int topBoardID)
			This allows separating interface from implementation and should allow modifications to the DLL without breaking clients.
		*/
		AsierInhoBoard() { ; }

	public:
		/**
			Virtual destructor (required so that derived classes are deleted properly)
		*/
		virtual ~AsierInhoBoard() { ; }

		//PUBLIC METHODS USED BY CLIENTS:

		/**
			Connects to the boards, using the board type and ID specified.
			ID corresponds to the index we labeled on each board.
			Returns NULL if connection failed.
		*/
		virtual bool connect(BoardType boardType, int bottomBoardID, int topBoardID = 0) = 0;
		/**
			The transducer boards are "logically" arranged in a side by side conficuration (16 transducers for (line1, bottom), 16 transd for (line1, top); the next 16 for (line2,bottom); the next 16 for (line2, top), etc...
			HOWEVER, the way the transducers are arranged in the hardware is different (the are arranged in block of 8, conneted to the same shift register)
			Besides, each transducer has an inherent (and different) phase delay.

			This method allows you to read the mapping of the transducers and their respective phase delays, which is used by GS_PAT to create the messages to send to the board.
			If you are not using GS-PAT, you do not need to read these values.
		*/
		/*virtual void readAdjustments(int* transducerIds, int* phaseAdjust) = 0;*/
		/**
			DEBUG VERSION: Reads the parameters required to use the boards (transducer positions, phase corrections, etc). 
			This method allows you to read the information required by GS_PAT to compute solutions, and create the messages to send to the board.			
			NOTE: Asumes the second board is on top of the first one (23.8cm)
		*/
		virtual void readParameters(float* transducerPositions, int* transducerIds, int* phaseAdjust, float* amplitudeAdjust, int* numDiscreteLevels)=0;

		/**
			change the mode of the AsierInho i.e. Normal mode (updates every phases and amplitudes) or MATD mode
		*/
		virtual void modeReset(int mode = NORMAL) = 0;

		//METHODS TO USE IN MODE "NORMAL": 
		/**
			The method sends these messages to the boards (512B for bottom board, 512B for top)xnumMessages. These are already properly formattted (e.g. first byte in each package is set to 128 + phase[0]).
			NOTE: The GPU solver (GS-PAT) directly discretises and formats the messages, so clients using GS-PAT only need to call this method
			(but not methods to discretise, etc.).
		*/
		virtual void updateMessage(unsigned char* message) = 0;

		/**
			This method turns the transducers off (so that the board does not heat up/die misserably)
			The board is still connected, so it can later be used again (e.g. create new traps)
		*/
		virtual void turnTransducersOn() = 0;
		virtual void turnTransducersOff() = 0;
		/**
			This method disconnects the board, closing the ports.
		*/
		virtual void disconnect() = 0;

		//METHODS TO USE IN MODE "MATD"

		virtual float boardHeight() = 0;
		/**
			In the MATD mode, hologram calculation is done in the FPGA.
			This method updates the board using position of the trap (the origin is the centre of the bottom board and it's in meter),
			colour of the LED illumination [0,255], apmlitude [0,1] and phase [0,2pi) of the point, and a flag indicating twin trap or focusing.
			It creates a levitation trap when TwinFlag = true, otherwise a focusing point.
		*/
		virtual void updateFrame(vec3 position, vec3 colour = vec3(0, 0, 0), float amplitude = 1.0f, float phase = 0.0f, bool twinTrap = true) = 0;
		/**
			To achieve 40kHz update rate, we need to send multiple (~50) frames at the same time to increase the communication speed.
			The FPGA can handle up to 250 frames at once.
		*/
		virtual void updateMultipleFrames(int numFramesPerUpdate, vec3 *positions, vec3 *colours = nullptr, float *amplitudes = nullptr, float *phases = nullptr, bool *twinFlags = nullptr) = 0;


		//METHODS TO USE IF COMPUTING SOUNDFIELDS IN YOUR OWN:
		/**
			This board uses discrete representations for the phases.
			That is, while our pahses can be any real number (we usually think of them in the range [0..2PI)),
			the board only uses 128 discrete values.
			This method computes the set of discretized phases from an input set of (real) phases.
			The number of elements in the array corresponds to the number of transducers in AsierInho.
			NOTE: This is done authomatically by the GPU solver (GS-PAT), so this method should only be called for
			clients computing sound fields on their own
		*/
		virtual void discretizePhases(float* phases, unsigned char* discretePhases) = 0;
		virtual unsigned char _discretizePhase(float phase) = 0;
		/**
			This board uses discrete representations for the amplitudees.
			That is, while our amplitudes can be any real number (we usually think of them in the range [0..1)),
			the board only uses 64 discrete values (half the phase resolution).
			This method computes the set of discretized amplitudes from an input set of (real) amplitudes.
			The number of elements in the array corresponds to the number of transducers in AsierInho.
			NOTE: This is done authomatically by the GPU solver (GS-PAT), so this method should only be called for
			clients computing sound fields on their own
		*/
		virtual void discretizeAmplitudes(float* amplitudes, unsigned char* discreteAmplitudes) = 0;
		virtual unsigned char _discretizeAmplitude(float amplitude) = 0;

		/**
			Using variable amplitudes (different to 1) effectively shifts the phases. This method must be called once
			phases and amplitudes have been discretised to avoid this shifting.
			NOTE: This is done authomatically by the GPU solver (GS-PAT), so this method should only be called for
			clients computing sound fields on their own
		*/
		virtual void correctPhasesShift(unsigned char* discretePhases, unsigned char* discreteAmplitudes) = 0;

		/**
			This method updates AsierInho with the discretized phases specified as an argument.
			Thus, the method will simply send these discretized phases to the board.
			NOTE: This is done authomatically by the GPU solver (GS-PAT), so this method should only be called for
			clients computing sound fields on their own
		*/
		virtual void updateDiscretePhases(unsigned char* discretePhases) = 0;
		virtual void updateDiscretePhasesAndAmplitudes(unsigned char* discretePhases, unsigned char* discreteAmplitudes) = 0;
	};
	
	_AsierInho_Export AsierInhoBoard* createAsierInho();

	void printMessage(const char*);
	void printError(const char*);
	void printWarning(const char*);

	_AsierInho_Export void RegisterPrintFuncs(void(*p_Message)(const char*), void(*p_Warning)(const char*), void(*p_Error)(const char*));
};


#endif
