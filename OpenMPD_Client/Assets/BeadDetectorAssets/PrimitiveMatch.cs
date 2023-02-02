using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


namespace Assets.Helper
{
#if true
    class PrimitiveMatch
    {
        public Vector3 initialPosition;
        public float startMoveTime;
        //Previous state of the Primitive (we save, so that we can restore once finished) 
        public Vector3 prev_position;           //Where it was. We only change its position, so we do not need to save the whole matrix (we will not change it)    
        public float prev_moveStepSize;         //How quickly the primitive moved
        public uint prev_positionDescriptor;    //Descriptors that it used.
        public uint prev_amplitudeDescriptor;

        public static PrimitiveMatch[] generateMatch(Vector3[] particlePositions, Primitive[] primitivesToMatch/*, GameObject[] fixedPosDesc = null*/)
        {
            //0. Create return array:
            PrimitiveMatch[] result = new PrimitiveMatch[primitivesToMatch.Length];
            //1. Save primitive states:
            for (int p = 0; p < result.Length; p++)
            {
                result[p] = new PrimitiveMatch();
                Primitive primitive = primitivesToMatch[p];
                result[p].prev_moveStepSize = primitive.maxStepInMeters;
                result[p].prev_position = primitivesToMatch[p].transform.position;
                result[p].prev_positionDescriptor = primitive.GetPositionsDescriptorID();
                result[p].prev_amplitudeDescriptor = primitive.GetAmplitudesDescriptorID();
            }
            //2. MATCH: Currently SUPER STUPID-> First particle seen matched to first primitive in array
            // We need something more clever, but this is just to test. 
            for (int p = 0; p < result.Length; p++)
            {
                result[p].initialPosition = particlePositions[p];
            }
            return result;
        }
    }
#endif
}

