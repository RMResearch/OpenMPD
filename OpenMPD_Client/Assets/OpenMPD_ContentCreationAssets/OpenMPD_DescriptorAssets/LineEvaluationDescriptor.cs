using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;

public class LineEvaluationDescriptor : PositionDescriptorAsset
{
    [Header("Line parameters:")]
    public Vector3 iniPos0 = new Vector3();
    public Vector3 endPos0 = new Vector3();
    public Vector3 iniPos1 = new Vector3();
    public Vector3 endPos1 = new Vector3();
    public Vector3 iniPos2 = new Vector3();
    public Vector3 endPos2 = new Vector3();
    public int numSamples = 1024;
    [Header("Circle parameters:")]
    public float radius = 0.02f;
    public uint numSamplesCircle = 1024;


    [Header("Update Descriptor")]
    public bool updateDescriptor = false;

    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    //public Vector3 initialPos;

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
    public void GeneratePositions()
    {
        int samples = (int)(numSamples / 4);

        // genetate the circle path
        float[] positions0 = GenerateCiclePosArray();
        // generate the line segments from centre to tright, right to left and left to centre
        float[] positions1 = GenerateSegmentPositions(iniPos0, endPos0, samples);
        float[] positions2 = GenerateSegmentPositions(iniPos1, endPos1, samples*2);
        float[] positions3 = GenerateSegmentPositions(iniPos2, endPos2, samples);

        // combine the set of arrays
        float[] positions = CreateCombinedArrayFrom(positions0, positions1);
        AppendSecondArrayToFirst(ref positions, positions2);
        AppendSecondArrayToFirst(ref positions, positions3);

        //initialPos = new Vector3(positions[0], positions[1], positions[2]);
        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = this.GetDescriptorID();
    }

    float[] GenerateSegmentPositions(Vector3 startPos, Vector3 endPos, int samples)
    {
        // this function will genrate a linear segment of length
        // Determine how many smaples per second we will need:
        float[] positions = new float[4 * samples];

        float len = (endPos - startPos).magnitude;

        // define direction and startig position
        Vector3 vectDir = (endPos - startPos).normalized;
        Vector3 curPos = startPos;

        // the len variable represents the linear segment's length
        float step = len / samples;

        for (int i = 0; i < samples; i++)
        {
            // one way            
            positions[4 * i + 0] = curPos.x;
            positions[4 * i + 1] = curPos.y;
            positions[4 * i + 2] = curPos.z;
            positions[4 * i + 3] = 1;
            curPos += step * vectDir;
        }        

        return positions;
    }

    float[] GenerateCiclePosArray()
    {
        //Determine how many smaples per second we will need:
        float[] positions = new float[4 * numSamplesCircle];
        //Fill circle: 
        for (int s = 0; s < numSamplesCircle; s++)
        {
            float angle = 2 * (Mathf.PI * s) / numSamplesCircle;
            positions[4 * s + 0] = radius * Mathf.Cos(angle);
            positions[4 * s + 1] = 0;
            positions[4 * s + 2] = radius * Mathf.Sin(angle);
            positions[4 * s + 3] = 1;
        }
        return positions;

        // update the initialPos to be visible in hte UI
        //initialPos = new Vector3(positions[0], positions[1], positions[2]);
        //Create descriptor
        //descriptor = new Positions_Descriptor(positions);
        // retreive desciptor id
        //descriptorID = descriptor.positionsDescriptorID;
    }

    public static void AppendSecondArrayToFirst<T>(ref T[] first, T[] second)
    {
        int arrayOriginalSize = first.Length;
        Array.Resize<T>(ref first, first.Length + second.Length);
        Array.Copy(second, 0, first, arrayOriginalSize, second.Length);
    }

    public static T[] CreateCombinedArrayFrom<T>(T[] first, T[] second)
    {
        T[] result = new T[first.Length + second.Length];
        Array.Copy(first, 0, result, 0, first.Length);
        Array.Copy(second, 0, result, first.Length, second.Length);
        return result;
    }
}
