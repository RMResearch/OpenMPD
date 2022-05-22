using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using MyBox;

//[ExecuteInEditMode]
public class UpdateFixPosDescriptors : PositionDescriptorAsset
{
    [Header("reference Descriptor")]
    public GameObject refDescriptor;

    [Header("Variables")]
    public Vector3 position;
    [ShowOnly] public uint desciptorID = 0;

    [HideInInspector]
    public float[] positions = new float[4];

    [ButtonMethod]
    private string UpdateDescriptor()
    {
        updateDescriptor = true;
        return "Amplitude Descriptor: Update requested";
    }

    // local variables
    bool updateDescriptor = false;

    // Use this for initialization
    void Start()
    {
        updateDescriptor = true;
    }

    // Update is called once per frame
    void Update()
    {
        if (updateDescriptor && OpenMPD_PresentationManager.Instance() && refDescriptor != null)
        {
            GeneratePositions();
            updateDescriptor = false;
        }
    }

    void GeneratePositions()
    {
        Vector3 posIni = refDescriptor.GetComponent<ReadPosDesciptorCSV>().initialPos;
        position = posIni;
        //Create descriptor
        positions[0] = posIni.x;
        positions[1] = posIni.y;
        positions[2] = posIni.z;
        positions[3] = 1;

        descriptor = new Positions_Descriptor(positions);
        desciptorID = GetDescriptorID();
    }

}
