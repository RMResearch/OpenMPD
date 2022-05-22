using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using UnityEngine;
using System.Runtime.InteropServices;

namespace Assets.DLL_Loader
{
    class SplineWrapper
    {
        #region DLL Imports         

        #region main functions
        [DllImport("SplineDll", EntryPoint = "initializeSpline")]
        public static extern void initializeSpline(int upsIn, float TIn, int nIn, int targetSamplingIn, string destination, int writeDataIn);// remove printLabeslIn

        [DllImport("SplineDll", EntryPoint = "computeSpline")]
        public static extern void computeSpline(float[] posIn, float[] AtIn, int useHomoTime);

        [DllImport("SplineDll", EntryPoint = "resampleSpline")]
        public static extern void resampleSpline(float[] AtIn);

        [DllImport("SplineDll", EntryPoint = "resampleSplineNoChange")]
        public static extern void resampleSplineNoChange(int sampling);

        [DllImport("SplineDll", EntryPoint = "releaseAllMemory")]
        public static extern void releaseAllMemory();
        #endregion

        #region Get functions
        [DllImport("SplineDll", EntryPoint = "getPosSize")]
        public static extern int getPosSize();

        [DllImport("SplineDll", EntryPoint = "getPosArraySize")]
        public static extern int getPosArraySize();

        [DllImport("SplineDll", EntryPoint = "getPosArrayAt")]
        public static extern float getPosArrayAt(int index);

        [DllImport("SplineDll", EntryPoint = "getDeltaTSize")]
        public static extern int getDeltaTSize();

        [DllImport("SplineDll", EntryPoint = "getDeltaAt")]
        public static extern float getDeltaAt(int index);

        [DllImport("SplineDll", EntryPoint = "getMaxDelta")]
        public static extern float getMaxDelta();
        #endregion

        #endregion

        // main functions
        public void InitializeSpline(int upsIn, float TIn, int nIn, int targetSamplingIn, string destination, int printLabelsIn, int writeDataIn)
        {
            initializeSpline(upsIn, TIn, nIn, targetSamplingIn, destination, writeDataIn);
        }

        public void ComputeSpline(float[] posIn, float[] AtIn, int useHomoTime)
        {
            computeSpline(posIn, AtIn, useHomoTime);
        }

        public void ResampleSpline(float[] AtIn)
        {
            resampleSpline(AtIn);
        }

        public void ResampleSplineNoChange(int samples)
        {
            resampleSplineNoChange(samples);
        }

        // get functions
        public int GetPosSize()
        {
            return getPosSize();
        }

        public int GetPosArraySize()
        {
            return getPosArraySize();
        }

        public float GetPosArrayAt(int index)
        {
            return getPosArrayAt(index);
        }

        public int GetDeltaTSize()
        {
            return getDeltaTSize();
        }

        public float GetDeltaAt(int index)
        {
            return getDeltaAt(index);
        }

        public float GetMaxDelta()
        {
            return getMaxDelta();
        }

        public void ReleaseAllMemory()
        {
            releaseAllMemory();
        }

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
        // At, x, y, z
        public void readFromFile(string name, ref List<Vector4> pos, ref List<float> deltas)
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
                        if (splits.Length > 4)
                        {
                            deltas.Add(float.Parse(splits[0]));
                            pos.Add(new Vector4(float.Parse(splits[1]),
                                        float.Parse(splits[3]),
                                        float.Parse(splits[2]),
                                        float.Parse(splits[4])));
                        }
                        else
                        {
                            deltas.Add(float.Parse(splits[0]));
                            pos.Add(new Vector4(float.Parse(splits[1]),
                                        float.Parse(splits[3]),
                                        float.Parse(splits[2]),
                                        1));
                        }

                    }

                    index++;
                }
            }
        }
    }
}
