using UnityEngine;
using System.Collections;
using System;

public class Positions_Descriptor
{
    //Reference management variables
    public readonly uint positionsDescriptorID;
    protected int referenceCounter;
    //Variables describing positions (contiguous positions, in homogeneous coordinates (x,y,z,1)). 
    public float[] positions;
    private int curPosition;
       
    protected uint DeclareDescriptor()
    {        
        referenceCounter = 0;
        return OpenMPD_ContextManager .Instance().DeclareDescriptor(this);        
    }
    public Positions_Descriptor(float[] positionsInHomogCoords){
        this.positions = (float[])positionsInHomogCoords.Clone();
        positionsDescriptorID = DeclareDescriptor();
        curPosition=0;
    }
    public Positions_Descriptor(Vector4[] positions) 
    {
        this.positions = new float[4 * positions.Length];
        for (int p = 0; p < positions.Length; p++) {
            this.positions[4 * p + 0] = positions[p].x;
            this.positions[4 * p + 1] = positions[p].y;
            this.positions[4 * p + 2] = positions[p].z;
            this.positions[4 * p + 3] = positions[p].w;
        }
        positionsDescriptorID = DeclareDescriptor();
    }
    public Positions_Descriptor(Vector3[] positions) 
    {
        this.positions = new float[4 * positions.Length];
        for (int p = 0; p < positions.Length; p++)
        {
            this.positions[4 * p + 0] = positions[p].x;
            this.positions[4 * p + 1] = positions[p].y;
            this.positions[4 * p + 2] = positions[p].z;
            this.positions[4 * p + 3] = 1;
        }
        positionsDescriptorID = DeclareDescriptor();
    }

    //Just to visualise the current position of the particle:
    
    /**
     This method returns a position from the position descriptor, to update the visual representation in Unity. 
     Please note this is just for reference and is not synchronised with the actual particle in the device (the OpenGL Rendering Manager does that).
     Actually, if several primitives share the same ddescriptor, they will all share the same reference position from this method, even if they use
     the descriptor with different initial positions. 
     */
    public Vector3 getCurrentSimulatedPosition(int visualUpdateSpeed) {
        Vector3 result = new Vector3(positions[4 * curPosition + 0], positions[4 * curPosition + 1], positions[4 * curPosition + 2]);
        curPosition = (curPosition + visualUpdateSpeed) % (positions.Length/4);
        return result;
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

    ~Positions_Descriptor()
    {
        if (!IsDisposable())
            OpenMPD_Wrapper.PrintWarning("Dispossing of Positions descriptor still in use (" + this.positionsDescriptorID+ ").\n Unity wrapper for descriptor will be dispossed, but OpenMPD resources will remain allocated and unreachable.");

        OpenMPD_ContextManager _context = OpenMPD_ContextManager.Instance();
        if (_context != null)
            _context.RemoveDescriptor(this);
    }
}
