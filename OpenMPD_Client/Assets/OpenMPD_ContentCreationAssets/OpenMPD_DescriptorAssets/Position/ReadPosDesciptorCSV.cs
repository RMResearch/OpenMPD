using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using MyBox;

public class ReadPosDesciptorCSV : PositionDescriptorAsset
{
    [Header("DescriptorVariables")]
    [ShowOnly] public int numSamples = 0;
    [ShowOnly] public uint descriptorID = 0;
    public Vector3 initialPos;

    [Header("File Properties")]    
    public string FileName = "empty";


    [ButtonMethod]
    private string UpdateDescriptor()
    {
        loadFile = true;
        return "Amplitude Descriptor: Update requested";
    }

    // local variables
    bool loadFile = false;

    WriteData rw = new WriteData();
    [HideInInspector] public List<Vector4> positions = new List<Vector4>();

    //Start is called before the first frame update
    void Start() {
        loadFile = true;
    }

    // Update is called once per frame
    void Update()  {
        if (OpenMPD_PresentationManager.Instance() && loadFile) {
            if (FileName != "empty")
            {
                positions.Clear();               
                // read the data and store it on a vector4 list
                rw.readFromFileVec4(FileName, ref positions, false);

                initialPos = new Vector3(positions[0].x, positions[0].y, positions[0].z);
                //Create descriptor
                descriptor = new Positions_Descriptor(positions.ToArray());
                // update sample size on the inspector
                numSamples = positions.Count;

                // retreive desciptor id
                descriptorID = descriptor.positionsDescriptorID;                
            }
            else
                Debug.Log("The fields FileName and ShapeToLoad are empty, please select a file or shape to load");

            loadFile = false;
        }
    }
}
