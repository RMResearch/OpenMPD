using UnityEngine;
using System.Collections;

public class CirclePosDescriptor : PositionDescriptorAsset
{
    [Header("Circle parameters:")]
    public float radius = 0.02f;
    public uint numSamples = 1024;
    
    // Use this for initialization
    void Start()
    {
        //Determine how many smaples per second we will need:
         float[] positions = new float[4*numSamples];
        //Fill circle: 
        for (int s = 0; s < numSamples; s++) {
            float angle = 2*(Mathf.PI * s) / numSamples;
            positions[4 * s + 0] = radius * Mathf.Cos(angle);
            positions[4 * s + 1] = 0;
            positions[4 * s + 2] = radius * Mathf.Sin(angle);
            positions[4 * s + 3] = 1;
        }
        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
    }

    // Update is called once per frame
    void Update()
    {
        ;
    }
}
