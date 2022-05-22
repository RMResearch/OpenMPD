#include <PBD_Engine_Prerequisites.h>
#include <PBD_Engine_CWrapper.h>
#include <stdio.h>

void PrintMessage(const char* msg) {
	printf("%s\n", msg);
}

int main(void) {
		//0. Init
        PBDEngine_CWrapper_RegisterPrintFuncs(PrintMessage, PrintMessage, PrintMessage);
        PBDEngine_CWrapper_Initialize();
        PBDEngine_CWrapper_SetupEngine(2000000);
        //1. Setup scene        
        PBD_PrimitiveManager_Handler primitiveManagerHandler = PBDEngine_CWrapper_StartEngine(10240, 32, 26, 24);
        float p1[]=  { 0.02f, 0, 0.1f, 1 }, p2[]= { -0.02f, 0, 0.1f, 1}, a1[]= { 1 } ;
        cl_uint pos1 = PBDEngine_CWrapper_createPositionsDescriptor(primitiveManagerHandler, p1,1);
        cl_uint pos2 = PBDEngine_CWrapper_createPositionsDescriptor(primitiveManagerHandler, p2,1);
        cl_uint amp1 = PBDEngine_CWrapper_createAmplitudesDescriptor(primitiveManagerHandler, a1, 1);
        cl_uint pri1 = PBDEngine_CWrapper_declarePrimitive(primitiveManagerHandler, pos1, amp1);
        cl_uint pri2 = PBDEngine_CWrapper_declarePrimitive(primitiveManagerHandler, pos2, amp1);
        PBDEngine_CWrapper_commitUpdates(primitiveManagerHandler);
        PBDEngine_CWrapper_setPrimitiveEnabled(primitiveManagerHandler, pri1, true);
        PBDEngine_CWrapper_setPrimitiveEnabled(primitiveManagerHandler, pri2, true);
        PBDEngine_CWrapper_commitUpdates(primitiveManagerHandler);
		//2. Run
		float mat [] = {			1 , 0, 0, 0,
                                    0 , 1, 0, 0,
                                    0 , 0, 1, 0.0f,
                                    0 , 0, 0, 1,

                                    1 , 0, 0, 0,
                                    0 , 1, 0, 0,
                                    0 , 0, 1, 0.0f,
                                    0 , 0, 0, 1,
            };
            cl_uint targets[] = { pri1, pri2 };
            PBDEngine_CWrapper_update_HighLevel(primitiveManagerHandler, targets, 2, mat, mat);
			char c;
			scanf("%c", &c);
		//3. End
		PBDEngine_CWrapper_releasePrimitive(primitiveManagerHandler, pri1);
        PBDEngine_CWrapper_releasePrimitive(primitiveManagerHandler, pri2);
        PBDEngine_CWrapper_releasePositionsDescriptor(primitiveManagerHandler, pos1);
        PBDEngine_CWrapper_releasePositionsDescriptor(primitiveManagerHandler, pos2);
        PBDEngine_CWrapper_releaseAmplitudesDescriptor(primitiveManagerHandler, amp1);
        PBDEngine_CWrapper_commitUpdates(primitiveManagerHandler);
        PBDEngine_CWrapper_StopEngine();
        PBDEngine_CWrapper_RegisterPrintFuncs(NULL, NULL, NULL);
}