using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;

public class SmoothSegmentDescriptor : PositionDescriptorAsset
{
    [Header("Configuration")]
    public float vmax = 0.5f;
    public Vector3 iniPos0 = new Vector3();
    public Vector3 endPos0 = new Vector3();
    public Vector3 iniPos1 = new Vector3();
    public Vector3 endPos1 = new Vector3();
    public Vector3 iniPos2 = new Vector3();
    public Vector3 endPos2 = new Vector3();

    [Header("Edited")]
    [ShowOnly] public int numSamples = 1024;
    [ShowOnly] public float len = 0.03f;
    public Vector3 initialPos = new Vector3();
    public Vector3 finalPos = new Vector3();

    [Header("Update Descriptor")]
    public bool updateDescriptor = false;
    public uint descriptorID = 0;

    private int ups = 10000;
    ReadWriteData writer = new ReadWriteData();

    // Start is called before the first frame update
    void Start()
    {
        GeneratePsotions();
    }

    // Update is called once per frame
    void Update()
    {
        if (updateDescriptor)
        {
            GeneratePsotions();

            updateDescriptor = false;
        }
    }

    public void GeneratePsotions()
    {
        float[] positions0 = GenerateSegmentPositions(iniPos0, endPos0);
        float[] positions1 = GenerateSegmentPositions(iniPos1, endPos1);
        float[] positions2 = GenerateSegmentPositions(iniPos2, endPos2);
        float[] positions = CreateCombinedArrayFrom(positions0, positions1);

        //Debug.Log("On generatePos: Appending the third array");
        AppendSecondArrayToFirst(ref positions, positions2);

        //Debug.Log("On generatePos: computing initial and final pos");
        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);
        finalPos = new Vector3(positions[positions.Length - 4], positions[positions.Length - 3], positions[positions.Length - 2]);

        #region
        //List<Vector3> posList = new List<Vector3>();
        //FromArrayToList(positions, ref posList);
        //writer.clearData();
        //writer.AddListVec3ToDataToPrint(posList, posList.Count);
        //writer.WriteDataToFile("TestingData");
        #endregion

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = descriptor.positionsDescriptorID;
    }

    float[] GenerateSegmentPositions(Vector3 ini, Vector3 end)
    {
        Vector3 dir = end - ini;
        len = dir.magnitude;
        dir.Normalize();

        float updateSeconds = 1 / (float)ups; // ~0.00001
        float aconstant = (updateSeconds * vmax * vmax) / ((len / 2.0f) * 2.0f); // ~0.00267
        float vset = 1.0f * vmax; // 1.0 Vmax, 0.8 Vmax or 0.5 Vmax
        float amax = 1.0f * aconstant; // from 1.0 amax to 2.0 amax ~0.00267 m/s^2

        List<Vector3> posList = new List<Vector3>();
        float v = 0.0f; // m / s
        //float posInc = -(len / 2);
        int index = 0;

        // Fordward: acceration part
        while (v < vset)
        {
            posList.Add(ini);
            ini += dir * updateSeconds * v;
            v += amax;
            index++;
        }
        v = vset;

        // Fordward: deceration part
        while (v > 0)
        {
            posList.Add(ini);
            ini += dir * updateSeconds * v;
            v -= amax;
            index++;
        }
        v = 0;

        //// Backward: acceration part
        //while (v < vset)
        //{
        //    posList.Add(ini);
        //    ini += dir * -updateSeconds * v;
        //    v += amax;
        //    index++;
        //}
        //v = vset;

        //// Backward: deceration part
        //while (v > 0)
        //{
        //    posList.Add(ini);
        //    ini += dir * -updateSeconds * v;
        //    v -= amax;
        //    index++;
        //}

        // updating size and positions buffer
        numSamples = posList.Count;
        // Determine how many smaples per second we will need:
        float[] positions = new float[4 * numSamples];

        for (int i = 0; i < numSamples; i++)
        {
            positions[4 * i + 0] = posList[i].x;
            positions[4 * i + 1] = posList[i].y;
            positions[4 * i + 2] = posList[i].z;
            positions[4 * i + 3] = 1;
        }

        //Create descriptor
        return positions;       
    }

    void FromArrayToList(float[] pos, ref List<Vector3> posList)
    {
        posList.Clear();

        for (int i = 0; i < pos.Length; i = i+4)
        {
            Vector3 newPos = new Vector4(pos[i], pos[i + 1], pos[i + 2]);
            posList.Add(newPos);
        }
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
