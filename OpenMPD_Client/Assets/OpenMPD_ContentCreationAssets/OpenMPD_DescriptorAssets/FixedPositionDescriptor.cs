using UnityEngine;
using System.Collections;
using MyBox;

public class FixedPositionDescriptor : PositionDescriptorAsset
{

    [Header("Variables")]    
    public Vector3 position;    
    [ShowOnly] public uint desciptorID = 0;

    [HideInInspector]
    public float[] positions;// = new float[4];

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
        if (updateDescriptor && OpenMPD_PresentationManager.Instance())
        {
            GeneratePositions();
            updateDescriptor = false;
        }
    }

    void GeneratePositions()
    {
        //Create descriptor
        positions = new float[4] { position.x, position.y, position.z, 1 };
        if(descriptor != null)
        {
            OpenMPD_ContextManager.Instance().RemoveDescriptor(descriptor);
        }
        descriptor = new Positions_Descriptor(positions);
        desciptorID = GetDescriptorID();
    }
}
