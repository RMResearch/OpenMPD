using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class testZeroAmplitude : MonoBehaviour
{
    public bool zeroAllAmplitudes=false;
    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        if (zeroAllAmplitudes) {
            Primitive [] primitives = FindObjectsOfType<Primitive>();//
            float[] a = new float[1]; a[0] = 0;
            Amplitudes_Descriptor zeroAmplitude = new Amplitudes_Descriptor(a); 
            //2. Set the zeroAmplitude descriptor to them (board will turn transducers off, makikng them easier to detect and capture). 
            
            foreach (Primitive p in primitives)
                //p.gameObject.SetActive(false);       
                p.SetAmplitudesDescriptor(zeroAmplitude.amplitudesDescriptorID);
            OpenMPD_PresentationManager.Instance().RequestCommit();
            zeroAllAmplitudes = false;
        }
        
    }
}
