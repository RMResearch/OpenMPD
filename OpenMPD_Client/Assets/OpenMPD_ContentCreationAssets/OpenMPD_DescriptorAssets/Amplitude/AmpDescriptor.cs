using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using MyBox;

public class AmpDescriptor : AmplitudeDescriptorAsset
{
    [Separator("Params")]
    public int samples = 1024;
    public float amp = 10000.0f;

    [Separator("Update Descriptor")]
    [ReadOnly] public bool updateDescriptor = false;    
    [ReadOnly] public uint descriptorID = 0;

    [ButtonMethod]
    private string UpdateDescriptor()
    {
        updateDescriptor = true;
        return "Amplitude Descriptor: Updated";       
    }

    private float[] amplitudes;

    // Start is called before the first frame update
    void Start()
    {
        GenerateAmplitudes();
    }

    // Update is called once per frame
    void Update()
    {
        if(updateDescriptor)
        {
            GenerateAmplitudes();
            updateDescriptor = false;
        }
    }

    void GenerateAmplitudes()
    {
        amplitudes = new float[samples];
        for (int i = 0; i < samples; i++)
        {
            amplitudes[i] = amp;
        }
        descriptor = new Amplitudes_Descriptor(amplitudes);
        descriptorID = descriptor.amplitudesDescriptorID;
    }
}
