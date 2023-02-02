using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LinePosDescriptor : PositionDescriptorAsset
{
    [Header("Line parameters:")]
    public Vector3 startPos = new Vector3(-0.015f, 0, 0);
    public Vector3 endPos = new Vector3(0.015f, 0, 0);
    public int numSamples = 1024;

    [Header("Update Descriptor")]
    public bool updateDescriptor = false;

    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    public Vector3 initialPos;
    
    // Start is called before the first frame update
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
        // this function will genrate a linear segment of length
        // Determine how many smaples per second we will need:
        float[] positions = new float[4 * numSamples];

        float len = (endPos - startPos).magnitude;

        // define direction and startig position
        Vector3 vectDir = (endPos - startPos).normalized;
        Vector3 curPos = startPos;

        // the len variable represents the linear segment's length
        float step = len / numSamples;

        for (int i = 0; i < numSamples; i++)
        {
            // one way            
            positions[4 * i + 0] = curPos.x;
            positions[4 * i + 1] = curPos.y;
            positions[4 * i + 2] = curPos.z;
            positions[4 * i + 3] = 1;
            curPos += step * vectDir;
        }
        initialPos = new Vector3(positions[0], positions[1], positions[2]);

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = this.GetDescriptorID();
    }
}
