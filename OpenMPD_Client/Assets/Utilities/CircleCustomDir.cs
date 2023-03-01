using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CircleCustomDir : PositionDescriptorAsset
{
    [Header("Circle parameters:")]
    public float radius_m = 0.03f;
    public uint numSamples = 1024;
    public RenderingPlane renderingPlane = RenderingPlane.xz;
    public Vector3 center_m = new Vector3(0, 0, 0);
    public float nornalRot = 0;
    public bool invertDir = false;

    [Header("Update Descriptor")]
    public bool updateDescriptor = false;

    [Header("ReadOnly")]
    [ShowOnly] public uint descriptorID = 0;
    public Vector3 initialPos;

    [Header("Get Position from Index")]
    public int index = 0;
    public Vector3 posFromIndex = new Vector3();

    [HideInInspector]
    public float[] positions;


    // private variables
    private Vector3 origin_m = new Vector3(0, 0, 0);

    // Use this for initialization
    void Start()
    {
        updateDescriptor = true;//We cannot create things yet. With this we delay creation until Update method is called (which also waits for OpenMPD to be fully ready).
    }

    // Update is called once per frame
    void Update()
    {
        if (updateDescriptor && OpenMPD_PresentationManager.Instance())
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
                generatePositionsXY(invertDir);
                break;
            case RenderingPlane.xz:
                generatePositionsXZ(invertDir);
                break;
            case RenderingPlane.yz:
                generatePositionsYZ(invertDir);
                break;
        }
    }

    void generatePositionsXZ(bool invert)
    {
        // checking invert direction flag
        int adj = 1;
        if (invert)
            adj = -1;

        // A. Compute the circle positions, assuming it is in the centre of the working space
        Vector3 offset = center_m - origin_m;
        //Determine how many smaples per second we will need:
        positions = new float[4 * numSamples];
        // Fill out circle positions
        ComputePosXZ(ref positions, adj);
        // B. Apply the rottion matrix
        if(nornalRot>=0 && nornalRot <= 360)
            RotatePosArray(ref positions, nornalRot);
        // C. Apply te offset
        SetOffset(ref positions, offset);

        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);
        posFromIndex = new Vector3(positions[index * 4], positions[index * 4 + 1], positions[index * 4 + 2]);

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = descriptor.positionsDescriptorID;
    }

    void ComputePosXZ(ref float[] posArray, int adj)
    {
        for (int s = 0; s < numSamples; s++)
        {
            float angle = 2 * (Mathf.PI * s) / numSamples;
            Vector3 newPoint = new Vector3(radius_m * Mathf.Cos(angle) * adj, 0, radius_m * Mathf.Sin(angle));

            posArray[4 * s + 0] = newPoint.x;
            posArray[4 * s + 1] = newPoint.y;
            posArray[4 * s + 2] = newPoint.z;
            posArray[4 * s + 3] = 1;
        }
    }

    void RotatePosArray( ref float[] posArray, float rot)
    {
        for (int s=0; s< numSamples; s++)
        {
            // get the point
            Vector3 newPoint = new Vector3(posArray[4 * s], posArray[4 * s + 1], posArray[4 * s + 2]);
            // multiply the point by the rotation
            newPoint = Quaternion.Euler(new Vector3(0, rot, 0)) * newPoint;
            // store the rotated position
            posArray[4 * s + 0] = newPoint.x;
            posArray[4 * s + 1] = newPoint.y;
            posArray[4 * s + 2] = newPoint.z;
            posArray[4 * s + 3] = 1;
        }
    }

    void SetOffset(ref float[] posArray, Vector3 offset)
    {
        for (int s = 0; s < numSamples; s++)
        {
            Vector3 newPoint = new Vector3(posArray[4 * s], posArray[4 * s + 1], posArray[4 * s + 2]);
            posArray[4 * s + 0] = newPoint.x + offset.x;
            posArray[4 * s + 1] = newPoint.y + offset.y;
            posArray[4 * s + 2] = newPoint.z + offset.z;
            posArray[4 * s + 3] = 1;
        }
    }

    void generatePositionsYZ(bool invert)
    {        
        Vector3 offset = center_m - origin_m;
        //Determine how many smaples per second we will need:
        positions = new float[4 * numSamples];
        //Fill circle: 
        for (int s = 0; s < numSamples; s++)
        {
            float angle = 2 * (Mathf.PI * s) / numSamples;
            positions[4 * s + 0] = 0 + offset.x;
            positions[4 * s + 1] = (radius_m * Mathf.Cos(angle)) + offset.y;
            positions[4 * s + 2] = (radius_m * Mathf.Sin(angle)) + offset.z;
            positions[4 * s + 3] = 1;
        }
        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);
        posFromIndex = new Vector3(positions[index * 4], positions[index * 4 + 1], positions[index * 4 + 2]);

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = descriptor.positionsDescriptorID;
    }

    void generatePositionsXY(bool invert)
    {
        Vector3 offset = center_m - origin_m;
        //Determine how many smaples per second we will need:
        positions = new float[4 * numSamples];
        //Fill circle: 
        for (int s = 0; s < numSamples; s++)
        {
            float angle = 2 * (Mathf.PI * s) / numSamples;
            positions[4 * s + 0] = (radius_m * Mathf.Cos(angle)) + offset.x;
            positions[4 * s + 1] = (radius_m * Mathf.Sin(angle)) + offset.y;
            positions[4 * s + 2] = 0 + offset.z;
            positions[4 * s + 3] = 1;
        }
        // update the initialPos to be visible in hte UI
        initialPos = new Vector3(positions[0], positions[1], positions[2]);
        posFromIndex = new Vector3(positions[index * 4], positions[index * 4 + 1], positions[index * 4 + 2]);

        //Create descriptor
        descriptor = new Positions_Descriptor(positions);
        descriptorID = descriptor.positionsDescriptorID;
    }
}
