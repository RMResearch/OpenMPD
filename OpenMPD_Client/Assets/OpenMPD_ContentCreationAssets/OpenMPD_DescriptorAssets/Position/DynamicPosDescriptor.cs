using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DynamicPosDescriptor : PositionDescriptorAsset
{
    [Header(" update ")]
    /*[ShowOnly]*/    public bool updateDescriptor = false;

    [Header("Main")]
    public int numSamples = 1024;
    
    [ShowOnly] public int lenght = 0;
    [ShowOnly] public uint positionID = 0;
    public Vector3 iniPos = new Vector3(0, 0, 0);

    [HideInInspector] public float[] positions;
    // Start is called before the first frame update

    int multiplier = 2;
    void Start()
    {        
        defineDefaultContent1();
        descriptor = new Positions_Descriptor(positions);
        positionID = this.GetDescriptorID();
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetKeyDown(KeyCode.U))
            updateDescriptor = true;

        if (updateDescriptor)
        {
            //descriptor.RemoveDescriptor();
            descriptor = new Positions_Descriptor(positions);
            //descriptor.positions = positions;

            iniPos = new Vector3(positions[0], positions[1], positions[2]);
            positionID = descriptor.positionsDescriptorID;
            lenght = positions.Length;
            updateDescriptor = false;
        }
    }

    void defineDefaultContent1()
    {
        float radius = 0.02f;
        positions = new float[4 * numSamples];

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
        iniPos = new Vector3(positions[0], positions[1], positions[2]);
        lenght = positions.Length;
    }

    void defineDefaultContent2()
    {
        float radius = 0.02f;
        
        positions = new float[4 * numSamples * multiplier];

        //Determine how many smaples per second we will need:
        positions = new float[4 * numSamples * multiplier];
        //Fill circle: 
        for (int s = 0; s < numSamples* multiplier; s++)
        {
            float angle = 2 * (Mathf.PI * s) / (numSamples * multiplier);
            positions[4 * s + 0] = radius * Mathf.Cos(angle);
            positions[4 * s + 1] = 0;
            positions[4 * s + 2] = radius * Mathf.Sin(angle);
            positions[4 * s + 3] = 1;
        }

        lenght = positions.Length;
    }
}
