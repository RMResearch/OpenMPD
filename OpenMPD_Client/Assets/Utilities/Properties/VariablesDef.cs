using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Text;
using System.IO;
using UnityEditor;

public class ReadWriteData 
{
    public string data="";
    private string endl = "\n";

    public ReadWriteData()
    {
        data = "";
    }

    public void InitializeArray()
    {
        data = "";
    }

    public void clearData()
    {
        data = "";
    }

    public void SetData(string dataIn)
    {
        data = dataIn;
    }

    public void AddStringToDataToPrint(string input)
    {
        data += input + endl;
        data += endl;
    }

    public void AddEmptyLine()
    {
        data += endl;
    }

    
    public void AddVectorToDataToPrint(double[] input, int size, string message = "")
    {
        if (message != "")
            data += message + endl;
        for (int i = 0; i < size; i++)
        {
            data += input[i].ToString() + ",";
        }
        data += endl;
        data += endl;
    }

    public void AddSpaceToDataToPrint()
    {
        data += endl;
        data += endl;
    }

    public void AddListVec3ToDataToPrint(List<Vector3> input, int size, string message = "")
    {
        if (message != "")
            data += message + endl;

        for (int i = 0; i < size; i++)
        {
            data += input[i].x.ToString() + "," + input[i].y.ToString() + "," + input[i].z.ToString() + "," + endl;
        }
        data += endl;
        data += endl;
    }

    public void AddListColorToDataToPrint(List<Color32> input, int size, string message = "")
    {
        if (message != "")
            data += message + endl;

        for (int i = 0; i < size; i++)
        {
            data += input[i].r.ToString() + "," + input[i].g.ToString() + "," + input[i].b.ToString() + "," + input[i].a.ToString() + "," + endl;
        }
        data += endl;
        data += endl;
    }

    public void AddListDoubleToDataToPrint(List<double> input, string message = "")
    {
        if (message != "")
            data += message + endl;

        for (int i = 0; i < input.Count; i++)
        {
            data += input[i].ToString() + ",";
        }
        data += endl;
        data += endl;
    }

    public void AddMatToDataToPrint(double[,] input, int xSize, int ySize, string message = "")
    {
        if (message != "")
            data += message + endl;

        for (int i = 0; i < xSize; i++)
        {
            for (int j = 0; j < ySize; j++)
            {
                data += input[i, j].ToString() + ",";
            }
            data += endl;
        }
        data += endl;
    }

    
   

    public void WriteDataToFile(string title)
    {
        string fileName;
        fileName = EditorUtility.SaveFilePanel("Choose the file path", "", title, "csv");
        System.IO.File.WriteAllText(fileName, data);
        data = "";
    }
    public string ReadDataFromFile(string name)
    {
        string fileName;
        if(name == "")
            fileName = EditorUtility.OpenFilePanel("Overwrite with csv", "", "csv");
        else//ColourConfigData_A_Samp-1100_step-30
            fileName = "Assets/Utilities/External/ProjCalib/" + name + ".csv";

        //fileName = "Assets/Utilities/External/ProjCalib/" + name + ".csv";
        //EditorUtility.read("Choose the file path", "", title, "csv");
        data = System.IO.File.ReadAllText(fileName);
        return data;
    }
    // x, y, z order
    public void readFromFileVec3(string name, ref List<Vector3> pos, bool addExtension)
    {
        //string address = Application.dataPath + "/../ExperimentResults/" + name.ToString() + ".csv";
        string address = name ;
        if (addExtension)
            address += ".csv";

        using (var rd = new StreamReader(address))
        {
            int index = 0;
            while (!rd.EndOfStream)
            {
                var splits = rd.ReadLine().Split(',');
              
                    if (splits.Length >= 3)
                        pos.Add(new Vector3(float.Parse(splits[0]),
                                    float.Parse(splits[1]),
                                    float.Parse(splits[2])));
                

                index++;
            }
        }
        // print column1
        //Console.WriteLine("Position:");
        //foreach (var element in pos)
        //Console.WriteLine(element.ToString());
    }
    public string ReadColorDataFromFile(string name)
    {
        string fileName;
        if (name == "")
            fileName = EditorUtility.OpenFilePanel("Overwrite with csv", "", "csv");
        else//ColourConfigData_A_Samp-1100_step-30
            fileName = "Assets/Utilities/External/ColouringFiles/" + name + ".csv";

        //fileName = "Assets/Utilities/External/ProjCalib/" + name + ".csv";
        //EditorUtility.read("Choose the file path", "", title, "csv");
        data = System.IO.File.ReadAllText(fileName);
        return data;
    }
}

#region Enum Variables
public enum RenderingPlane
{
    xy, xz, yz
}

public enum CapturingMode
{
    readPixels, readPixelsOnPostRender, screenCapture, screenCaptureCallBack, SCPtr, EmptyTexturePtr
}

public enum ColouringMode
{
    Read, Write, SendToShader, ClearAll, none
}

public enum MediaOption
{
    image, video
}

public enum InputMode
{
    Tablet, Mouse, LeapMotion
}

public enum InputShape
{
    FromDevice, FromFile, FromOptFile, FromMouse, Linear, LinearSmooth, Circle
}

public enum OptMode
{
    RTConst, RTUnconst, Disable
}

public enum C3Mode
{
     Jtotal, NJtotal, Jmax, NJmax, noJ
}

public enum PathUnits
{
    meters, millimeters 
}

public enum Shapes
{
    Heart, Pentagon, Star, Butterfly, Torus43, Torus54
}

public enum ShapeSize
{
    _70mm, _140mm, _200mm, _270mm, _330mm, _200mm_Spline, _270mm_Spline 
}

public enum ReadCond
{
    noRead, readFile, readOptFile 
}

public enum WriteCond
{
    noWrite, writeData
}

public enum SamplesCond
{
    computeSamples, usePreComputedSamples 
}

#endregion

public class VariablesDef : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }
}


