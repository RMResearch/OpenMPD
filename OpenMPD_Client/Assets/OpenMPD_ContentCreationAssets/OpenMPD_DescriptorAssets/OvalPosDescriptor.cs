using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class OvalPosDescriptor : PositionDescriptorAsset
{
    [Header("Oval parameters:")]
    public float radius_m = 0.03f;
    public uint numSamples = 1024;      
    public Vector3 center_m = new Vector3(0, 0, 0);
    public RenderingPlane renderingPlane = RenderingPlane.yz;
    public float axis1MultBy = 2;
    public float axis2MultBy = 1;

    [Header("Update Descriptor")]
    public bool updateDescriptor = false;

    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    public Vector3 initialPos;


    // private
    float[] positions;

    // private variables
    private Vector3 origin_m = new Vector3(0, 0, 0);

    // Use this for initialization
    void Start()
    {
        UpdatePositionDescriptor(renderingPlane);        
    }

    // Update is called once per frame
    void Update()
    {
        if (updateDescriptor)
        {
            UpdatePositionDescriptor(renderingPlane);
            updateDescriptor = false;
        }
           
    }

    void UpdatePositionDescriptor(RenderingPlane rp)
    {
        switch (rp)
        {
            case RenderingPlane.xy:
                generatePositionsXY();
                break;
            case RenderingPlane.xz:
                generatePositionsXZ();
                break;
            case RenderingPlane.yz:
                generatePositionsYZ();
                break;
        }
    }

    void generatePositionsXZ() {
        Vector3 offset = center_m - origin_m;
        //Determine how many smaples per second we will need:
        positions = new float[4 * numSamples];
        //Fill circle: 
        for (int s = 0; s < numSamples; s++)
        {
            float angle = 2 * (Mathf.PI * s) / numSamples;
            positions[4 * s + 0] = (radius_m * Mathf.Cos(angle) * axis1MultBy) + offset.x;
            positions[4 * s + 1] = 0 + offset.y;
            positions[4 * s + 2] = (radius_m * Mathf.Sin(angle) * axis2MultBy) + offset.z;
            positions[4 * s + 3] = 1;
        }
        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = descriptor.positionsDescriptorID;
    }

    void generatePositionsYZ()
    {
        Vector3 offset = center_m - origin_m;
        //Determine how many smaples per second we will need:
        positions = new float[4 * numSamples];
        //Fill circle: 
        for (int s = 0; s < numSamples; s++)
        {
            float angle = 2 * (Mathf.PI * s) / numSamples;
            positions[4 * s + 0] = 0 + offset.x;
            positions[4 * s + 1] = (radius_m * Mathf.Cos(angle) * axis1MultBy) + offset.y;
            positions[4 * s + 2] = (radius_m * Mathf.Sin(angle) * axis2MultBy) + offset.z;
            positions[4 * s + 3] = 1;
        }
        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = descriptor.positionsDescriptorID;
    }

    void generatePositionsXY()
    {
        Vector3 offset = center_m - origin_m;
        //Determine how many smaples per second we will need:
        positions = new float[4 * numSamples];
        //Fill circle: 
        for (int s = 0; s < numSamples; s++)
        {
            float angle = 2 * (Mathf.PI * s) / numSamples;
            positions[4 * s + 0] = (radius_m * Mathf.Cos(angle) * axis1MultBy) + offset.x;
            positions[4 * s + 1] = (radius_m * Mathf.Sin(angle) * axis2MultBy) + offset.y;
            positions[4 * s + 2] = 0 + offset.z;
            positions[4 * s + 3] = 1;
        }
        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = descriptor.positionsDescriptorID;
    }
}
