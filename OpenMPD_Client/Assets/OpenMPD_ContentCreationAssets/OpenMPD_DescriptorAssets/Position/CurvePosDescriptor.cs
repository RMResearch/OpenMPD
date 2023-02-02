using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CustomCurvePosDescriptor : PositionDescriptorAsset
{
    [Header("Circle parameters:")]
    public float radius_m = 0.03f;
    public int numSamples = 1024;
    public int degreesStart = 0;
    public int degreesCurve = 90;
    public Vector3 center_m = new Vector3(0, 0, 0);
    // change origin_m to public to allow the origin be in a different location
    private Vector3 origin_m = new Vector3(0, 0, 0);

    [Header("Update Descriptor")]
    public bool updateDescriptor = false;

    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    public Vector3 initialPos;
    public Vector3 finalPos;

    // Use this for initialization
    void Start()
    {
        GeneratePositions();
    }

    // Update is called once per frame
    void Update()
    {
        if (updateDescriptor)
        {
            GeneratePositions();
            updateDescriptor = false;
        }
            
    }

    void GeneratePositions()
    {
        // compute the offset - keept it like this to allow the origin be in a different location
        Vector3 offset = center_m - origin_m;
        // compute total samples number
        int totalSamples = (360 * numSamples) / degreesCurve;
        // compute the starting index
        int indexStart = (int)(degreesStart * (totalSamples / 360));
        //Determine how many smaples per second we will need:
        float[] positions = new float[4 * numSamples];

        //Fill circle: 
        for (int s = 0; s < numSamples; s++)
        {
            float angle = 2 * (Mathf.PI * (s + indexStart)) / totalSamples;
            positions[4 * s + 0] = (radius_m * Mathf.Cos(angle)) + offset.x;
            positions[4 * s + 1] = 0 + offset.y;
            positions[4 * s + 2] = (radius_m * Mathf.Sin(angle)) + offset.z;
            positions[4 * s + 3] = 1;
        }

        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);
        finalPos = new Vector3(positions[positions.Length - 4], positions[positions.Length - 3], positions[positions.Length - 2]);

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
    }
}
