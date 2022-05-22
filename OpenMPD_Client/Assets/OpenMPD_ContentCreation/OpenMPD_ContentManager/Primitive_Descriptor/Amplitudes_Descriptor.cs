using UnityEngine;
using System.Collections;
using System;

public class Amplitudes_Descriptor 
{
    //Reference management variables
    public readonly uint amplitudesDescriptorID;
    protected int referenceCounter;
    //Variables describing amplitudes
    public float[] amplitudes;
    
    protected uint DeclareDescriptor() {
        referenceCounter = 0;
        return OpenMPD_ContextManager .Instance().DeclareDescriptor(this);
    }

    public Amplitudes_Descriptor(float[] amplitudes){
        //this.amplitudes = amplitudes;
        this.amplitudes = (float[])amplitudes.Clone();//Should I use a deep copy? 
        amplitudesDescriptorID = DeclareDescriptor();
    }
   
    //These methods must only be invoked by PDB_Descriptors_Manager (but C sharp does not support "friend"). 
    public void AddReference() {
        referenceCounter++;
    }
    public void DecreaseReference() {
        referenceCounter--;
    }
    public bool IsDisposable() {
        return referenceCounter == 0;
    }

}
