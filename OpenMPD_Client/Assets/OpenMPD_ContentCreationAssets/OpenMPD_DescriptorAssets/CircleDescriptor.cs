using UnityEngine;
using System.Collections;

public class CircleDescriptor : PositionDescriptorAsset
{
    [Header("Circle parameters:")]
    public float radius = 0.02f;
    public Vector3 axis = Vector3.up;
    public Vector3 centre = Vector3.zero;
    public float percentOffset = 0.0f;
    public uint numSamples = 1024;

    [Header("Update Descriptor")]
    public bool updateDescriptor = false;

    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    [ShowOnly] public Vector3 initialPosCM;

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
            float angle = 2 * (Mathf.PI * (((float)s / numSamples) + percentOffset));
            Vector3 pos = radius * new Vector3(Mathf.Sin(angle), Mathf.Cos(angle), 0);
            Quaternion rotation = Quaternion.FromToRotation(new Vector3(0, 0, 1), axis.normalized);
            pos = rotation * pos;
            pos += centre;
            positions[4 * s + 0] = pos.x;
            positions[4 * s + 1] = pos.y;
            positions[4 * s + 2] = pos.z;
            positions[4 * s + 3] = 1;
        }
        // update the initialPos to be visible in the UI
        initialPosCM = 100 * new Vector3(positions[0], positions[1], positions[2]);        
        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        // retreive desciptor id
        descriptorID = descriptor.positionsDescriptorID;
    }
}
