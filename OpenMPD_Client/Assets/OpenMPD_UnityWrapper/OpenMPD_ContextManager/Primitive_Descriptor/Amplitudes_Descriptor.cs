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

    ~Amplitudes_Descriptor()
    {
        if (!IsDisposable())
            OpenMPD_Wrapper.PrintWarning("Dispossing of Amplitudes descriptor still in use (" + this.amplitudesDescriptorID + ").\n Unity wrapper for descriptor will be dispossed, but OpenMPD resources will remain allocated and unreachable.");

        OpenMPD_ContextManager _context = OpenMPD_ContextManager.Instance();
        if (_context!= null)
            _context.RemoveDescriptor(this);
    }

}
