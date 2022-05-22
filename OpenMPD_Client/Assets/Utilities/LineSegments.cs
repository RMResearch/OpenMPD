using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using MyBox;

public class LineSegments : PositionDescriptorAsset
{
    [Header("Line parameters:")]
    public Vector3 pos0 = new Vector3();
    public Vector3 pos1 = new Vector3();
    public Vector3 pos2 = new Vector3();
    public int numSamples = 1024;

    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    [ShowOnly] public Vector3 initialPos = new Vector3();

    // local variables
    bool updateDescriptor = false;

    [ButtonMethod]
    private string UpdateDescriptor() {
        updateDescriptor = true;
        return "Descriptor update requested";
    }

    // Start is called before the first frame update
    void Start()  {
        updateDescriptor = true;
    }

    // Update is called once per frame
    void Update() {
        if (updateDescriptor && OpenMPD_PresentationManager.Instance()) {
            GeneratePositions();
            updateDescriptor = false;
        }
    }

    public void GeneratePositions()
    {
        int samples = (int)(numSamples / 4);

        // generate the line segments from centre to tright, right to left and left to centre
        float[] positions1 = GenerateSegmentPositions(pos0, pos1, samples);
        float[] positions2 = GenerateSegmentPositions(pos1, pos2, samples * 2);
        float[] positions3 = GenerateSegmentPositions(pos2, pos0, samples);

        // combine the set of arrays
        float[] positions = CreateCombinedArrayFrom(positions1, positions2);
        AppendSecondArrayToFirst(ref positions, positions3);

        initialPos = new Vector3(positions[0], positions[1], positions[2]);
        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = this.GetDescriptorID();
    }

    float[] GenerateSegmentPositions(Vector3 startPos, Vector3 endPos, int samples)  {
        // this function will genrate a linear segment of length
        // Determine how many smaples per second we will need:
        float[] positions = new float[4 * samples];
        float len = (endPos - startPos).magnitude;

        // define direction and startig position
        Vector3 vectDir = (endPos - startPos).normalized;
        Vector3 curPos = startPos;

        // the len variable represents the linear segment's length
        float step = len / samples;

        for (int i = 0; i < samples; i++) {
            // one way
            positions[4 * i + 0] = curPos.x;
            positions[4 * i + 1] = curPos.y;
            positions[4 * i + 2] = curPos.z;
            positions[4 * i + 3] = 1;
            curPos += step * vectDir;
        }

        return positions;
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
