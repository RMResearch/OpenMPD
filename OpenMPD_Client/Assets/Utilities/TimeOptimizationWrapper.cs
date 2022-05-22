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
    class TimeOptimizationWrapper
    {
        #region DLL Imports

        #region main fucntions
        [DllImport("OptimizationDll", EntryPoint = "optParam")]
        public static extern void optParam(int maxIterations, float epsilon, float maxStep, int linearSearchMode, float wolfGamma);

        [DllImport("OptimizationDll", EntryPoint = "initializeOpt")]
        public static extern bool initializeOpt(float Amax, float Vini, float Jmax, float Vmax, float k, float w0, float w1, float w2, float w3, int n, float T, int UPS, int isRTConst, int isNewC3In, int C0onIn);

        [DllImport("OptimizationDll", EntryPoint = "optimizePath")]
        public static extern void optimizePath(double[] posIn, string destination, int saveResults, int printLabels, int computeExtraVar); //float[] pos, ref float[] deltaTimes

        [DllImport("OptimizationDll", EntryPoint = "computeInitialPathProperties")]
        public static extern void computeInitialPathProperties(double[] posIn, string destination, int saveResults, int printLabels, int computeExtraVar); //float[] pos, ref float[] deltaTimes
        #endregion

        #region get functions
        [DllImport("OptimizationDll", EntryPoint = "getOptiIterationsNum")]
        public static extern int getOptiIterationsNum();

        [DllImport("OptimizationDll", EntryPoint = "getTotalAcc")]
        public static extern float getTotalAcc();

        [DllImport("OptimizationDll", EntryPoint = "getTotalTime")]
        public static extern float getTotalTime();

        [DllImport("OptimizationDll", EntryPoint = "getDeltaSum")]
        public static extern float getDeltaSum();

        [DllImport("OptimizationDll", EntryPoint = "getTotalJerk")]
        public static extern float getTotalJerk();

        [DllImport("OptimizationDll", EntryPoint = "getIniTotalAcc")]
        public static extern float getIniTotalAcc();

        [DllImport("OptimizationDll", EntryPoint = "getIniTotalTime")]
        public static extern float getIniTotalTime();

        [DllImport("OptimizationDll", EntryPoint = "getIniTotalJerk")]
        public static extern float getIniTotalJerk();

        [DllImport("OptimizationDll", EntryPoint = "getDeltaAt")]
        public static extern float getDeltaAt(int index);

        [DllImport("OptimizationDll", EntryPoint = "getMaxVel")]
        public static extern float getMaxVel();

        [DllImport("OptimizationDll", EntryPoint = "getMaxAcc")]
        public static extern float getMaxAcc();

        [DllImport("OptimizationDll", EntryPoint = "getMaxJerk")]
        public static extern float getMaxJerk();
        #endregion

        #region utilities
        [DllImport("OptimizationDll", EntryPoint = "releaseAllMemory")]
        public static extern float releaseAllMemory();
        #endregion

        #endregion

        // manin functions
        public void OptParam(int maxIterations, float epsilon, float maxStep, int linearSearchMode, float wolfGamma)
        {
            optParam(maxIterations, epsilon, maxStep, linearSearchMode, wolfGamma);
        }

        public bool InitializeOpt(float Amax, float Vini, float Jmax, float Vmax, float k, float w0, float w1, float w2, float w3, int n, float T, int UPS, int isRTConst, int isNewC3In, int isC0)
        {
            return initializeOpt(Amax, Vini, Jmax, Vmax, k, w0, w1, w2, w3, n, T, UPS, isRTConst, isNewC3In, isC0);
        }

        public void OptimizePath(double[] posIn, string destination, int saveResults, int printLabels, int computeExtraVar)
        {
            optimizePath(posIn, destination, saveResults, printLabels, computeExtraVar);
        }

        public void ComputeInitialPathProperties(double[] posIn, string destination, int saveResults, int printLabels, int computeExtraVar)
        {
            computeInitialPathProperties(posIn, destination, saveResults, printLabels, computeExtraVar);
        } 


        // Get funcitons
        public int GetOptiIterationsNum()
        {
            return getOptiIterationsNum();
        }

        public float GetTotalAcc()
        {
            return getTotalAcc();
        }

        public float GetTotalTime()
        {
            return getTotalTime();
        }
        public float GetDeltaSum()
        {
            return getDeltaSum();
        }

        public float GetTotalJerk()
        {
            return getTotalJerk();
        }

        public float GetIniTotalAcc()
        {
            return getIniTotalAcc();
        }

        public float GetIniTotalTime()
        {
            return getIniTotalTime();
        }

        public float GetIniTotalJerk()
        {
            return getIniTotalJerk();
        }

        public float GetDeltaAt(int index)
        {
            return getDeltaAt(index);
        }

        //public float GetVelAt(int index)
        //{
        //    return getVelAt(index);
        //}

        //public float GetAccAt(int index)
        //{
        //    return getAccAt(index);
        //}

        //public float GetJerkAt(int index)
        //{
        //    return getJerkAt(index);
        //}

        public float GetMaxVel()
        {
            return getMaxVel();
        }

        public float GetMaxAcc()
        {
            return getMaxAcc();
        }

        public float GetMaxJerk()
        {
            return getMaxJerk();
        }

        public float ReleaseAllMemory()
        {
            return releaseAllMemory();
        }
    }
}
