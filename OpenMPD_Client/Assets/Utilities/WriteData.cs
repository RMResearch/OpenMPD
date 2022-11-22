using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using System.Text;
using System;

public class WriteData  
{
    // Utilities: write data to file
    #region Write data
    public bool writeToFile(string name, List<string> dataArray, ref string error)
    {
        bool state = true;
        try
        {
            string fileName = Application.dataPath + "/../ExperimentResults/" + name.ToString() + ".csv";
            System.IO.File.WriteAllLines(fileName, dataArray.ToArray());
        }
        catch (FileLoadException)
        {
            error = "File error: the file cannot be created/accessed";
            state = false;
            return state;
        }
        catch (IOException)
        {
            error = "IO error: the file cannot be created";
            state = false;
            return state;
        }

        return state;
    }

    public bool writeFromVec3ToFile(string name, string filePath, List<Vector3> dataArray, ref string error)
    {
        StreamWriter writer;
        bool state = true;
        try
        {
            filePath = System.IO.Directory.GetCurrentDirectory();
            writer = new StreamWriter(filePath);
        }
        catch (FileLoadException)
        {
            error = "File error: the file can not be created";
            state = false;
            return state;
        }
        catch (IOException)
        {
            error = "IO error: the file can not be created";
            state = false;
            return state;
        }
        string localLine = "";
        for (int i = 0; i < dataArray.Count; i++)
        {
            localLine += dataArray[i].ToString() + ",";
            writer.WriteLine(localLine);
        }
        return state;
    }

    public bool writeFromVec4ToFile(string name, string folderInRoot, List<Vector4> dataArray, ref string error)
    {
        StreamWriter writer;
        string fileName = Application.dataPath + "/../"+ folderInRoot +"/" + name.ToString() + ".csv";
        bool state = true;
        try
        {
            //filePath = System.IO.Directory.GetCurrentDirectory();
            writer = new StreamWriter(fileName);
        }
        catch (FileLoadException)
        {
            error = "File error: the file can not be created";
            state = false;
            return state;
        }
        catch (IOException)
        {
            error = "IO error: the file can not be created";
            state = false;
            return state;
        }

        //string localLine = "";
        for (int i = 0; i < dataArray.Count; i++)
        {
            //localLine += dataArray[i].ToString() + ",";
            writer.WriteLine(dataArray[i].ToString("F6") + ",");

        }
        return state;
    }

    public bool writeFromFloatArrayToFile(string name, float[] dataArray, ref string error)
    {
        StreamWriter writer;
        bool state = true;
        try
        {
            string fileName = Application.dataPath + "/../ExperimentResults/" + name.ToString() + ".csv";
            writer = new StreamWriter(fileName);
        }
        catch (FileLoadException)
        {
            error = "File error: the file can not be created";
            state = false;
            return state;
        }
        catch (IOException)
        {
            error = "IO error: the file can not be created";
            state = false;
            return state;
        }

        for (int i = 0; i + 3 < dataArray.Length; i += 4)
        {
            string localLine = dataArray[i + 0].ToString() + ",";
            localLine += dataArray[i + 1].ToString() + ",";
            localLine += dataArray[i + 2].ToString() + ",";
            localLine += dataArray[i + 3].ToString() + ",";
            writer.WriteLine(localLine);
        }
        return state;
    }

    #endregion

    #region Read data
    // x, y, z
    public void readFromFile(string name, ref List<Vector4> pos)
    {
        //string address = Application.dataPath + "/../ExperimentResults/" + name.ToString() + ".csv";
        string address = name + ".csv";
        using (var rd = new StreamReader(address))
        {
            int index = 0;
            while (!rd.EndOfStream)
            {
                var splits = rd.ReadLine().Split(',');
                if (index > 0)
                {
                    if (splits.Length > 3)
                        pos.Add(new Vector4(float.Parse(splits[0]),
                                    float.Parse(splits[2]),
                                    float.Parse(splits[1]),
                                    float.Parse(splits[3])));
                    else
                        pos.Add(new Vector4(float.Parse(splits[0]),
                                    float.Parse(splits[2]),
                                    float.Parse(splits[1]),
                                    1));
                }

                index++;
            }
        }
        // print column1
        //Console.WriteLine("Position:");
        //foreach (var element in pos)
        //Console.WriteLine(element.ToString());
    }

    // x, y, z order
    public void readFromFileVec3(string name, ref List<Vector3> pos)
    {
        //string address = Application.dataPath + "/../ExperimentResults/" + name.ToString() + ".csv";
        string address = name + ".csv";
        using (var rd = new StreamReader(address))
        {
            int index = 0;
            while (!rd.EndOfStream)
            {
                var splits = rd.ReadLine().Split(',');
                if (index > 0)
                {
                    if (splits.Length >= 3)
                        pos.Add(new Vector3(float.Parse(splits[0]),
                                    float.Parse(splits[1]),
                                    float.Parse(splits[2])));
                }

                index++;
            }
        }
        // print column1
        //Console.WriteLine("Position:");
        //foreach (var element in pos)
        //Console.WriteLine(element.ToString());
    }

    public void readFromFileVec4(string name, ref List<Vector4> pos, bool invertXY = false)
    {
        string address = Application.dataPath + "/../OptimizedPaths/" + name.ToString() + ".csv";
        using (var rd = new StreamReader(address))
        {
            int index = 0;
            while (!rd.EndOfStream)
            {
                var splits = rd.ReadLine().Split(',');
                if (index > 0)
                {
                    if (splits.Length >= 3)
                    {
                        if (!invertXY)
                            pos.Add(new Vector4(float.Parse(splits[0]),
                                   float.Parse(splits[1]),
                                   float.Parse(splits[2]),//0.12f - float.Parse(splits[2]),
                                   float.Parse(splits[3])));
                        else pos.Add(new Vector4(float.Parse(splits[1]),
                                    float.Parse(splits[0]),
                                    float.Parse(splits[2]),//0.12f - float.Parse(splits[2]),
                                    float.Parse(splits[3])));
                    }

                }
                index++;
            }
        }
        // print column1
        //Console.WriteLine("Position:");
        //foreach (var element in pos)
        //Console.WriteLine(element.ToString());
    }

    //in Column format
    public void readAmplitudesFromFile(string name, ref List<float> amp, bool isSizeLimitOn, int sizeLimit)
    {
        string address = Application.dataPath + "/../PAudio/" + name.ToString() + ".csv";
        using (var rd = new StreamReader(address)) {
            amp.Clear();
            int index = 0;
            while (!rd.EndOfStream) {
                var splits = rd.ReadLine().Split(',');
                if (isSizeLimitOn && amp.Count >= sizeLimit)
                    return;
                if (index > 0) {
                    if (splits.Length <= 2) {
                        amp.Add(float.Parse(splits[0]));
                    }
                }
                index++;
            }
        }
    }

    // x, y, z
    public void readFromFileNoneInvert(string name, ref List<Vector4> pos)
    {
        //string address = Application.dataPath + "/../ExperimentResults/" + name.ToString() + ".csv";
        string address = name + ".csv";
        using (var rd = new StreamReader(address))
        {
            int index = 0;
            while (!rd.EndOfStream)
            {
                var splits = rd.ReadLine().Split(',');
                if (index > 0)
                {
                    if (splits.Length > 3)
                        pos.Add(new Vector4(float.Parse(splits[0]),
                                    float.Parse(splits[1]),
                                    float.Parse(splits[2]),
                                    float.Parse(splits[3])));
                    else
                        pos.Add(new Vector4(float.Parse(splits[0]),
                                    float.Parse(splits[1]),
                                    float.Parse(splits[2]),
                                    1));
                }

                index++;
            }
        }
        // print column1
        //Console.WriteLine("Position:");
        //foreach (var element in pos)
        //Console.WriteLine(element.ToString());
    }
    public float[] readFromFileSingleRow(string name, ref List<Vector4> pos, bool invertXY = false, int mode = 0, int stepSize = 1, int maxSampNum = 10000)
    {
        string address = Application.dataPath + "/../OprimizedPaths/" + name.ToString() + ".csv";

        //string address = name + ".csv";
        using (var rd = new StreamReader(address))
        {
            pos.Clear();
            while (!rd.EndOfStream)
            {
                var splits = rd.ReadLine().Split(',');
                float[] positions = new float[splits.Length];
                for (int i = 0; i < splits.Length;)
                {
                    if (invertXY)
                    {
                        // fill out float array
                        positions[i + 0] = float.Parse(splits[i + 1]);
                        positions[i + 1] = float.Parse(splits[i + 0]);
                        positions[i + 2] = float.Parse(splits[i + 2]);
                        positions[i + 3] = float.Parse(splits[i + 3]);

                        // generate vector4
                        pos.Add(new Vector4(float.Parse(splits[i + 1]),
                                       float.Parse(splits[i + 0]),
                                       float.Parse(splits[i + 2]),
                                       float.Parse(splits[i + 3])));

                    }
                    else
                    {
                        // fill out float array
                        positions[i + 1] = float.Parse(splits[i + 0]);
                        positions[i + 0] = float.Parse(splits[i + 1]);
                        positions[i + 2] = float.Parse(splits[i + 2]);
                        positions[i + 3] = float.Parse(splits[i + 3]);
                        // generate vector4
                        pos.Add(new Vector4(float.Parse(splits[i + 0]),
                                       float.Parse(splits[i + 1]),
                                       float.Parse(splits[i + 2]),
                                       float.Parse(splits[i + 3])));
                    }
                    switch (mode)
                    {
                        case 0:
                            i += 4;
                            break;
                        case 1:
                            if (i < maxSampNum)
                                i += 4;
                            else
                                continue;
                            break;
                        case 2:
                            i += 4 * stepSize;
                            break;
                    }
                }
                return positions;
            }
        }
        return null;
    }

    public float[] readFromFileSingleRowToFloatArray(string name, bool invertXY = false, int mode = 0, int stepSize = 1, int maxSampNum = 10000)
    {
        string address = Application.dataPath + "/../OprimizedPaths/" + name.ToString() + ".csv";

        //string address = name + ".csv";
        using (var rd = new StreamReader(address))
        {
            while (!rd.EndOfStream)
            {
                var splits = rd.ReadLine().Split(',');
                int splitsCount = splits.Length;
                float[] positions;
                if (mode == 1)
                    positions = new float[maxSampNum];
                else if (mode == 2)
                    positions = new float[splitsCount / stepSize];
                else
                    positions = new float[splitsCount];

                splitsCount = positions.Length;
                int k = 0;

                for (int i = 0; i < splits.Length;)
                {
                    if (k + 3 >= splitsCount)
                        Debug.Log("here is the point where it brakes");

                    if (invertXY)
                    {
                        // fill out float array
                        positions[k + 0] = float.Parse(splits[i + 1]);
                        positions[k + 1] = float.Parse(splits[i + 0]);
                        positions[k + 2] = float.Parse(splits[i + 2]);
                        positions[k + 3] = float.Parse(splits[i + 3]);
                    }
                    else
                    {
                        // fill out float array
                        positions[k + 0] = float.Parse(splits[i + 0]);
                        positions[k + 1] = float.Parse(splits[i + 1]);
                        positions[k + 2] = float.Parse(splits[i + 2]);
                        positions[k + 3] = float.Parse(splits[i + 3]);
                    }
                    switch (mode)
                    {
                        case 0:
                            i += 4;
                            break;
                        case 1:
                            if (i < maxSampNum)
                                i += 4;
                            else
                                return positions;
                            break;
                        case 2:
                            i += 4 * stepSize;
                            break;
                    }
                    k += 4;
                    if (i + 4 >= splits.Length || k + 4 >= splitsCount)
                        return positions;
                }
                return positions;
            }
        }
        return null;
    }

    public void AddLineToFile(string fileName, string line)
    {
        try
        {
            string filePath = Application.dataPath + "/../ExperimentResults/Records/" + fileName;
            using (System.IO.StreamWriter file = new System.IO.StreamWriter(@filePath, true))
            {
                file.WriteLine(line);
            }
        }
        catch (Exception ex)
        {
            throw new ApplicationException("Exeption :", ex);
        }


    }
    #endregion
}
