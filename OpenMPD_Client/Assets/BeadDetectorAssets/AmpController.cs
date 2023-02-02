using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Assets.Utilities
{
    class AmpController
    {
        //public const string GradualPickup = "GradualPick";
        //public const string GradualRelease = "GradualRelease";
        //public const string SingingPickup = "SingingPickup";

        // Gradual picking/droping
        private float minAmpInPa = 100;
        private float maxAmpInPa = 15000;
        private float fixAmpInPa = 10000;
        private float frequency = 200;
        int sampleNumPerCycle = 10000;

        public AmpController(int sampPerCycle, float minAmp = 100, float maxAmp = 15000, float fixAmp = 10000, float freq=200) {
            sampleNumPerCycle = sampPerCycle;
            minAmpInPa = minAmp;
            maxAmpInPa = maxAmp;
            fixAmpInPa = fixAmp;
            frequency = freq;
        }

        public void UpdateAmplitudeRange(int sampPerCycle, float minAmp = 100, float maxAmp = 15000, float fixAmp = 10000, float freq = 200)
        {
            sampleNumPerCycle = sampPerCycle;
            minAmpInPa = minAmp;
            maxAmpInPa = maxAmp;
            fixAmpInPa = fixAmp;
            frequency = freq;
        }

        public float GetMaxAmpInPa()
        {
            return maxAmpInPa;
        }

        public float GetMinAmpInPa()
        {
            return minAmpInPa;
        }

        public float GetFixAmpInPa()
        {
            return fixAmpInPa;
        }

        public float GetFrequencyHz()
        {
            return frequency;
        }

        public float[] GetGradualPick(float holdingTimeInSec)//sampleNum = 10 000
        {
            // computing the required number of samples based on the holding time
            int numSamples = (int)(sampleNumPerCycle * holdingTimeInSec);
            // initializing hte amplitudes array 
            float[] amplitudes = new float[numSamples];
            // computing the ramp up behaviour data to fill the array out
            for (int s = 0; s < numSamples; s++)
                amplitudes[s] = minAmpInPa + s * (maxAmpInPa - minAmpInPa) / numSamples;
            // returning the amplitude descriptor
            return amplitudes;
        }

        public float[] GetGradualRelease(float holdingTimeInSec)//sampleNum = 10 000
        {
            // computing the required number of samples based on the holding time
            int numSamples = (int)(sampleNumPerCycle * holdingTimeInSec);
            // initializing hte amplitudes array 
            float[] amplitudes = new float[numSamples];
            // computing the ramp down behaviour data to fill the array out
            for (int s = 0; s < numSamples; s++)
                amplitudes[s] = maxAmpInPa + s * (maxAmpInPa - minAmpInPa) / numSamples;
            // returning the amplitude descriptor
            return amplitudes;
        }

        public float[] getSingingAmpDesc()//freq = 200;
        {
            // computing the required number of samples based on the given frequency
            int numSamples = (int)(sampleNumPerCycle / frequency);
            // initializing hte amplitudes array 
            float[] amplitudes = new float[numSamples];
            // computing the cosine wave shape data to fill hte array out
            for (int s = 0; s < numSamples; s++)
                amplitudes[s] = 10000 + (float)(5000 * Math.Cos((2 * Math.PI * s) / numSamples));
            // returning the amplitude descriptor
            return amplitudes;

            ////B. Audio at 200Hz (used as "audio" for fixed position).
            //static const int min = 10000, range = 5000, FPS = 10000, freq = 500;
            //float* a2_data = new float[FPS / freq];
            //for (int s = 0; s < FPS / freq; s++)
            //    a2_data[s] = min + range * cosf(2 * CL_M_PI * (1.0f * s) / (FPS / freq));
            //amp2 = PBDEngine_CWrapper_createAmplitudesDescriptor(pm, a2_data, FPS / freq);
        }

        public float[] getFixAmpDesc(float ampInPa)
        {
           return new float[] { ampInPa, ampInPa, ampInPa, ampInPa };
        }
    }
}
