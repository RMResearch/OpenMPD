using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;

public class LineSmoothPosDescriptor : PositionDescriptorAsset
{
    [Header("Configuration")]      
    public float vmax = 0.5f;
    public Vector3 iniPos = new Vector3();
    public Vector3 endPos = new Vector3();

    [Header("Edited")]
    [ShowOnly] public int numSamples = 1024;
    [ShowOnly] public float len = 0.03f;

    [Header("Update Descriptor")]
    public bool updateDescriptor = false;
    public uint DescID = 0;

    private int ups = 10000;

    // Start is called before the first frame update
    void Start()
    {
        updateDescriptor = true;
    }

    // Update is called once per frame
    void Update()
    {
        if (updateDescriptor && OpenMPD_PresentationManager.Instance())
        {
            GeneratePositions(iniPos, endPos);
            updateDescriptor = false;
        }
    }

    void GeneratePositions( Vector3 ini, Vector3 end)
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

        // Forward: acceleration part
        while (v < vset)
        {
            posList.Add(ini);
            ini += dir * updateSeconds * v;
            v += amax;
            index++;
        }
        v = vset;

        // Forward: deceleration part
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
            //Debug.Log("Pos: (" + posList[i].x + ", " + posList[i].y + ", " + posList[i].z + ")");
            positions[4 * i + 0] = posList[i].x;
            positions[4 * i + 1] = posList[i].y;
            positions[4 * i + 2] = posList[i].z;
            positions[4 * i + 3] = 1;
        }

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        DescID = this.GetDescriptorID();
    }
}
