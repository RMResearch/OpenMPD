using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using MyBox;

public class ReadDescriptorFromFile : PositionDescriptorAsset
{
    [Header("DescriptorVariables")]
    [ShowOnly] public int numSamples = 0;
    [ShowOnly] public uint descriptorID = 0;
    public Vector3 initialPos;

    [Header("Update")]
    public bool loadFile = false;
    public string FileName = "empty";
    public string folderInRoot = "empty";
    public bool writePositions = false;
    public bool isFileInSingleRow = false;
    public OptiPathShape shapeToLoad = OptiPathShape.noShape;

    [Header("Debbug")]
    public int mode = 0;
    public int stepSize = 1;
    public int maxSamples = 10000;
    public bool invertXY = false;
    public string outPutName = "";

    // local variables
    WriteData rw = new WriteData();
    List<Vector4> positions = new List<Vector4>();

    //Start is called before the first frame update
    void Start()
    {
    }

    // Update is called once per frame
    void Update()
    {
        if (OpenMPD_PresentationManager.Instance())
        {
            if (FileName != "empty" || shapeToLoad != OptiPathShape.noShape)
            {
                positions.Clear();
                float[] arrayPos = new float[1000];
                List<Vector4> posList = new List<Vector4>();

                if (shapeToLoad != OptiPathShape.noShape)
                {
                    FileName = RetreiveShapeName(shapeToLoad);
                    rw.readFromFileVec4(FileName, ref posList, invertXY);

                    if (writePositions)
                    {
                        // write the file
                        string error = "null";
                        rw.writeFromVec4ToFile(FileName,"" ,posList, ref error);
                    }

                    initialPos = new Vector3(posList[0].x, posList[0].y, posList[0].z);
                    //Create descriptor
                    //descriptor = new Positions_Descriptor(positions.ToArray());
                    descriptor = new Positions_Descriptor(posList.ToArray());
                    numSamples = posList.Count;
                }
                else
                {
                    // read the file
                    if (isFileInSingleRow)
                    {
                        //rw.readFromFileSingleRow(FileName, ref positions, invertXY, mode, stepSize, maxSamples);
                        float[] arrayPosAux = rw.readFromFileSingleRowToFloatArray(FileName, invertXY, mode, stepSize, maxSamples);
                        //Syste.Array.Copy(arr1, arr2, length);
                        arrayPos = (float[])arrayPosAux.Clone();
                    }
                    else
                    {
                        rw.readFromFileVec4(FileName, ref posList, invertXY);
                    }

                    if (writePositions)
                    {
                        // write the file
                        string error = "null";
                        if (isFileInSingleRow)
                        {
                            //rw.writeFromVec4ToFile(outPutName , positions, ref error);
                            rw.writeFromFloatArrayToFile(outPutName + "_" +
                                mode.ToString() + "_" +
                                stepSize.ToString() + "_" +
                                maxSamples.ToString(), arrayPos, ref error);
                        }
                        else
                        {
                            rw.writeFromVec4ToFile(FileName, folderInRoot,posList, ref error);
                        }
                    }

                    // update the initialPos to be visible in hte UI
                    if (isFileInSingleRow)
                    {
                        //initialPos = new Vector3(positions[0].x, positions[0].y, positions[0].z);
                        initialPos = new Vector3(arrayPos[0], arrayPos[1], arrayPos[2]);
                        //Create descriptor
                        //descriptor = new Positions_Descriptor(positions.ToArray());
                        descriptor = new Positions_Descriptor(arrayPos);
                        numSamples = arrayPos.Length / 4;
                    }
                    else
                    {
                        //initialPos = new Vector3(positions[0].x, positions[0].y, positions[0].z);
                        initialPos = new Vector3(posList[0].x, posList[0].y, posList[0].z);
                        //Create descriptor
                        //descriptor = new Positions_Descriptor(positions.ToArray());
                        descriptor = new Positions_Descriptor(posList.ToArray());
                        numSamples = posList.Count;
                    }
                }
                // retreive desciptor id
                descriptorID = descriptor.positionsDescriptorID;
                loadFile = false;
            }
            else
            {
                Debug.Log("The fields FileName and ShapeToLoad are empty, please select a file or shape to load");
            }
        }
    }

    string RetreiveShapeName(OptiPathShape shape)
    {
        switch (shape)
        {
            case OptiPathShape.Circle:
                return "Circle";
            case OptiPathShape.Square:
                return "Square";
            case OptiPathShape.Fish:
                return "Fish";
            case OptiPathShape.Cardioid:
                return "Cardioid";
            default:
                break;
        }
        return "noShape";
    }
}


