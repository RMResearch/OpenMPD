using UnityEngine;
using System.Collections;
using System;

public class Primitive_SoundExample : PrimitiveGameObject
{
    [Header("Parameters")]
    //public Vector3 vector3 = new Vector3();
    public float freq = 200;
    [ShowOnly] public int numSamples2 = 0;

    public override void ConfigureDescriptors()
    {
        //float freq = 200;
        numSamples2 = (int)(10000 / freq);
        float[] amplitudes2 = new float[numSamples2];
        for (int s = 0; s < numSamples2; s++)
            amplitudes2[s] = 10000 + (float)(5000 * Math.Cos((2 * Math.PI * s) / numSamples2));

        Amplitudes_Descriptor singingHold = new Amplitudes_Descriptor(amplitudes2);
        SetDescriptors(OpenMPD_ContextManager .Instance().GetDefaultPositionsDescriptor(), singingHold.amplitudesDescriptorID);
    }
}
