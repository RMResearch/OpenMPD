using UnityEngine;
using System.Collections;

public class CircleDescriptor : PositionDescriptorAsset
{
    [Header("Circle parameters:")]
    public float radius = 0.02f;
    public uint numSamples = 1024;

    [Header("Update Descriptor")]
    public bool updateDescriptor = false;

    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    [ShowOnly] public Vector3 initialPos;

    [HideInInspector]
    public float[] positions;

    // Use this for initialization
    void Start()
    {
        updateDescriptor = true;
    }

    // Update is called once per frame
    void Update()
    {
        if (updateDescriptor && OpenMPD_PresentationManager.Instance())
        {
            GeneratePosArray();
            updateDescriptor = false;
        }
    }

    void GeneratePosArray()
    {
        //Determine how many smaples per second we will need:
        positions = new float[4 * numSamples];
        //Fill circle: 
        for (int s = 0; s < numSamples; s++)
        {
            float angle = 2 * (Mathf.PI * s) / numSamples;
            positions[4 * s + 0] = radius * Mathf.Cos(angle);
            positions[4 * s + 1] = 0;
            positions[4 * s + 2] = radius * Mathf.Sin(angle);
            positions[4 * s + 3] = 1;
        }
        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);        
        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        // retreive desciptor id
        descriptorID = descriptor.positionsDescriptorID;
    }
}
