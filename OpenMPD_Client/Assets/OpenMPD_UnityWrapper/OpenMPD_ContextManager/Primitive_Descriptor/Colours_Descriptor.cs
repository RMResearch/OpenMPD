using UnityEngine;
using System.Collections;
using System;

public class Colours_Descriptor
{
    //Reference management variables
    public readonly uint coloursDescriptorID;
    protected int referenceCounter;
    //Variables describing colours (contiguous colours, in homogeneous coordinates (x,y,z,1)). 
    public float[] colours;
       
    protected uint DeclareDescriptor()
    {        
        referenceCounter = 0;
        return OpenMPD_ContextManager .Instance().DeclareDescriptor(this);        
    }
    public Colours_Descriptor(float[] coloursInHomogCoords){
        //this.colours = coloursInHomogCoords;
        this.colours = (float[])coloursInHomogCoords.Clone();//Should I use a deep copy? 
        coloursDescriptorID = DeclareDescriptor();
    }
    public Colours_Descriptor(Vector4[] colours) 
    {
        this.colours = new float[4 * colours.Length];
        for (int p = 0; p < colours.Length; p++) {
            this.colours[4 * p + 0] = colours[p].x;
            this.colours[4 * p + 1] = colours[p].y;
            this.colours[4 * p + 2] = colours[p].z;
            this.colours[4 * p + 3] = colours[p].w;
        }
        coloursDescriptorID = DeclareDescriptor();
    }
    public Colours_Descriptor(Vector3[] colours) 
    {
        this.colours = new float[4 * colours.Length];
        for (int p = 0; p < colours.Length; p++)
        {
            this.colours[4 * p + 0] = colours[p].x;
            this.colours[4 * p + 1] = colours[p].y;
            this.colours[4 * p + 2] = colours[p].z;
            this.colours[4 * p + 3] = 1;
        }
        coloursDescriptorID = DeclareDescriptor();
    }
        
    //These methods must only be invoked by PDB_Descriptors_Manager (but C sharp does not support "friend"). 
    public void AddReference()
    {
        referenceCounter++;
    }
    public void DecreaseReference()
    {
        referenceCounter--;
    }
    public bool IsDisposable()
    {
        return referenceCounter == 0;
    }

    ~Colours_Descriptor()
    {
        if (!IsDisposable())
            OpenMPD_Wrapper.PrintWarning("Dispossing of Colours descriptor still in use (" + this.coloursDescriptorID + ").\n Unity wrapper for descriptor will be dispossed, but OpenMPD resources will remain allocated and unreachable.");

        OpenMPD_ContextManager _context = OpenMPD_ContextManager.Instance();
        if (_context != null)
            _context.RemoveDescriptor(this);
    }
}
