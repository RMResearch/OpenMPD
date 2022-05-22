using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using MyBox;

public class ReadAmpDescriptorCSV : AmplitudeDescriptorAsset
{
    [Header("DescriptorVariables")]
    [ShowOnly] public int numSamples = 0;
    [ShowOnly] public uint descriptorID = 0;
    public string FileName = "empty";
    public bool isSizeLimitOn = true;
    public int sizeLimit = 10000;

    [ButtonMethod]
    private string UpdateDescriptor() {
        loadFile = true;
        return "Amplitude Descriptor: Update requested";
    }

    // local variables
    bool loadFile = false;
    WriteData rw = new WriteData();

    private void Start() {
        loadFile = true;
    }

    // Update is called once per frame
    void Update()
    {
        if (OpenMPD_PresentationManager.Instance() && loadFile)
        {
            if (FileName != "empty") {
                List<float> ampList = new List<float>();

                // read the file
                rw.readAmplitudesFromFile(FileName, ref ampList, isSizeLimitOn, sizeLimit);

                float[] amplitudes = ampList.ToArray();
                //Create descriptor
                descriptor = new Amplitudes_Descriptor(amplitudes);
                // update sample size on inspector
                numSamples = ampList.Count;

                // retreive desciptor id
                descriptorID = descriptor.amplitudesDescriptorID; 
            }
            else {
                Debug.Log("The fields FileName and ShapeToLoad are empty, please select a file or shape to load");
            }
            loadFile = false;
        }
    }

}
