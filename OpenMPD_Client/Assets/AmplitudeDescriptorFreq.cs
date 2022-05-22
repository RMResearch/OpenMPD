using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using MyBox;

public class AmplitudeDescriptorFreq : AmplitudeDescriptorAsset
{
    [Header("Parameters")]
    //public Vector3 vector3 = new Vector3();
    public float modulationFreq = 200;
    public float amp = 5000;
    public int offsetAmp = 10000;
    
    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    [ShowOnly] public int numSamples = 0;

    [HideInInspector]
    public float[] amplitudes;

    [ButtonMethod]
    private string UpdateDescriptor()
    {
        updateDescriptor = true;
        return "Amplitude Descriptor: Update requested";
    }

    // local variables
    bool updateDescriptor = false;

    // Start is called before the first frame update
    void Start() {
        updateDescriptor = true;
    }

    // Update is called once per frame
    void Update() {
        if (OpenMPD_PresentationManager.Instance() && updateDescriptor) {
            GenerateAmplitudes();
            updateDescriptor = false;
        }
    }

    void GenerateAmplitudes() {
        numSamples = (int)(10000 / modulationFreq);
        float[] amplitudes = new float[numSamples];
        for (int s = 0; s < numSamples; s++)
            amplitudes[s] = offsetAmp + (float)(amp * Math.Cos((2 * Math.PI * s) / numSamples));
        descriptor = new Amplitudes_Descriptor(amplitudes);
        descriptorID = descriptor.amplitudesDescriptorID;
    }
}
