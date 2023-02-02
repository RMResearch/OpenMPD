using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine.UI;
using UnityEngine;
using NativeAccess;
using System;
using System.Threading;

namespace Levitation
{
    public class BeadDetector_DLL_Loader : NativeWrapperBase
    {
        //1. Methods inherited from NativeWrapperBase
        protected override string GetPluginPath()
        {
            return @"\Assets\External\BeadDetector\";
        }

        public override string GetPluginName() //Name of DLL without .dll extension
        {
            return "BeadDetector";
        }
        protected override string defaultPrefix => "BeadDetector_";

        public delegate long _createInstance(int deviceID, float[] p1_World, float[] p2_World, float[] p3_World, float[] p4_World, int pixelsPerMeter, int threshold, int erodeDilate, float sphericity, float minRadiusInMeters, float maxRadiusInMeters, bool visualize);
        public _createInstance createInstance;

        public delegate long _calibrateDetector(long instance);
        public _calibrateDetector calibrateDetector;

        public delegate int _detectBeads(long instance);
        public _detectBeads detectBeads;

        public delegate void _getCurrentBeadPositions(long instance, float[] posBuffer);
        public _getCurrentBeadPositions getCurrentBeadPositions;

        public delegate float _destroyInstance(long instance);
        public _destroyInstance destroyInstance;

        public static BeadDetector_DLL_Loader getInstance()
        {
            return FindObjectOfType<BeadDetector_DLL_Loader>();
        }
        protected override void PostInit()
        {
            printPrefix = "BeadDetector";
        }
    }

}