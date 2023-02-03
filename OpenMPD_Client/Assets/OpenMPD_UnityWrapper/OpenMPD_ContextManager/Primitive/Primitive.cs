using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using MyBox;

public class Primitive : MonoBehaviour
{
    private uint primitiveID;
    //Primitive's are relative to the location/alignment of the Levitator. We need to keep track of that node. 
    Transform levitatorOrigin = null;
    //Matrix update controls (currently specified by user, they could be obtained from OpenMPD_PresentationManager)
    public float maxStepInMeters = 0.00025f;
    public float maxRotInDegrees = 1.0f;
    public int visualUpdateSpeed = 32;
    protected Matrix4x4 prevMatrix, curMatrix;   //Current location used for the matrix, according to the constraints above. Prev location retained for continuity.

    //Access methods to retrieve Primitive data (descriptors and location). 
    public uint GetPrimitiveID() {
        return primitiveID;
    }

    public uint GetPositionsDescriptorID() {
        return OpenMPD_ContextManager .Instance().GetPositionsDescriptorID(primitiveID);
    }
    public uint GetAmplitudesDescriptorID() {
        return OpenMPD_ContextManager .Instance().GetAmplitudesDescriptorID(primitiveID);
    }
    public uint GetColoursDescriptorID()
    {
        return OpenMPD_ContextManager .Instance().GetColoursDescriptorID(primitiveID);
    }
    public Vector3 GetCurrentPrimitivePosition() {
        //return curMatrix.GetColumn(3);//This would return the position of the primitive, but the descriptor might add other local disoplacements to the particle.
        //We make use of the physical bead we use to represent the trap, as this is updated from the data stored in the descriptors. 
        return this.gameObject.transform.GetChild(0).position;
    }

    //Methods to modify/control the Primitive
    /**
        Configures the primitive to use the default descriptors (good for a simple levitation trap). 
        The client creating the content should declare the descriptors that s/he will use and assign them to the Primitive later
        , but this ensures all primitives start from a valid state.
    */
    protected virtual void ConfigureDescriptors() {
        SetDescriptors(OpenMPD_ContextManager .Instance().GetDefaultPositionsDescriptor(), OpenMPD_ContextManager .Instance().GetDefaultAmplitudesDescriptor());
        SetColourDescriptor(OpenMPD_ContextManager .Instance().GetDefaultColoursDescriptor());
    }
    /**Configures the position and amplitudes descriptors to use and the starting position wihtin the descriptor. 
       The client must ensure that the descriptors are well defined (e.g. feasible paths, trap cpontinuity, feasible amplitudes).
       The client must also ensure starting positions are valid.
     */
    public void SetDescriptors(uint positionDescID, uint amplitudeDescID, uint startingPositionSample=0, uint startingAmplitudeSample=0) {
        //Change primitive state:
        OpenMPD_ContextManager .Instance().UseDescriptors(this.GetPrimitiveID(), positionDescID, amplitudeDescID, startingPositionSample, startingAmplitudeSample);
    }
    /**Configures the position descriptor to use and the starting position. The client must ensure that the descriptors is well defined (e.g. feasible paths, trap cpontinuity).
       The client must also ensure starting position is valid.*/
    public void SetPositionDescriptor(uint positionDescID, uint startingPositionSample = 0)
    {
        //Change primitive state:
        OpenMPD_ContextManager .Instance().UsePositionsDescriptor(this.GetPrimitiveID(), positionDescID, startingPositionSample);
    }
    /**Configures the amplitude descriptor to use and the starting position. The client must ensure that the descriptor is well defined (e.g., achievable amplitudes).
       The client must also ensure starting position is valid.*/
    public void SetAmplitudesDescriptor(uint amplitudeDescID, uint startingAmplitudeSample = 0)
    {
        //Change primitive state:
        OpenMPD_ContextManager .Instance().UseAmplitudesDescriptor(this.GetPrimitiveID(), amplitudeDescID, startingAmplitudeSample);
    }
    /**Configures the colour descriptor to use and the starting position. The client must ensure that the descriptors is well defined (e.g., its length matches the length of the positions descriptor used, so colours remain "static").
       The client must also ensure starting position is valid.*/
    public void SetColourDescriptor(uint colourDescID, uint startingColourSample = 0)
    {
        //Change primitive state:
        OpenMPD_ContextManager .Instance().UseColoursDescriptor(this.GetPrimitiveID(), colourDescID, startingColourSample);
    }
    /**This method returns true if OpenMPD is ready to run.
     * Please note one cannot assume this will be ready on the first frame (i.e., Start(), first call to Update()).
     * OpenMPD might need to capture the OpenGL context used by the Unity renderer, which happens on a separate thread. 
     * Also, OpenMPD then needs to wait for OpenCL initialization, device connection, solver creation, etc.    
     * No OpenMPD content should be created until this happens.
     */
    bool AllSetup()
    {
        return levitatorOrigin != null && OpenMPD_ContextManager .Instance() != null; ;
    }
    void Setup()
    {
        if (OpenMPD_PresentationManager.Instance())
            levitatorOrigin = OpenMPD_PresentationManager.Instance().GetLevitatorNode();
        if (AllSetup())
        {
            primitiveID = OpenMPD_PresentationManager.Instance().CreateContent(this);
            //Template method for subclasses to declare their content's geometry, sound.etc...
            ConfigureDescriptors();
            //Initialize:                        
            Matrix4x4 primLocalMat = transform.localToWorldMatrix;            
            curMatrix = levitatorOrigin.worldToLocalMatrix * primLocalMat;

            if (this.enabled)
                OnEnable();
            else 
                OnDisable();
        }
    }


    public void TeleportPrimitive(Matrix4x4 targetPos_world)
    {
        if (!AllSetup())
            Setup();
        //1. Update the transformation matrix of the Unity node (this is annoying, but Unity makes it hard to update matrices)
        //1.a. Compute matrix relative to parent: 
        Matrix4x4 targetPos_parent;
        if (transform.parent)
            targetPos_parent = transform.parent.worldToLocalMatrix * targetPos_world;
        else
            targetPos_parent = targetPos_world;

        //2.a. Modify node's position, scale and orientation 
        transform.localPosition = targetPos_parent.GetColumn(3);//Translation is stored in 4th column of the matrix
        // transform.localScale = new Vector3(
        //     targetPos_parent.GetColumn(0).magnitude,
        //     targetPos_parent.GetColumn(1).magnitude,
        //     targetPos_parent.GetColumn(2).magnitude
        //);//Scale is magnitude of direction vectors (3 first columns).
        // transform.localRotation = ExtractRotationFromMatrix(ref targetPos_parent);

        //2. Update interpolation matrices we use:
        prevMatrix = curMatrix = levitatorOrigin.worldToLocalMatrix * targetPos_world;
    }
    /**
     * Update is called once per Unity frame. It updates the matrix (pos/orient) of contents, 
     * which are updated at low (Unity) framerates.  
     */
    
    void Update()
    {
        if (!AllSetup())
            Setup();
        else
        {
            //1. Update the actual primitive. Compute location relative to device and send data to device
            //Update matrices, keeping the (angle, distance) contraints specified. 
            prevMatrix = curMatrix;
            // get the particles position relative to the levitatorOrigin
            Matrix4x4 primLocalMat = transform.localToWorldMatrix;
            // get the transformation matrix to go from the primitive to the levitator origin M_from-prim_to-ori 
            Matrix4x4 target_M_LocalToLevitator = levitatorOrigin.worldToLocalMatrix * primLocalMat;
            // get rotations
            Quaternion prevRot = GetRotation(prevMatrix);
            Quaternion targetRot = GetRotation(target_M_LocalToLevitator);
            // get positions
            Vector4 prevPos = GetPosition(prevMatrix);
            Vector4 targetPos = GetPosition(target_M_LocalToLevitator);
            curMatrix = BuildMatrix(InterpolateOrientationCapped(prevRot, targetRot, this.maxRotInDegrees)
                      , InterpolatePositionCapped(prevPos, targetPos, this.maxStepInMeters));

            //2. Update the bead we use to represent the primitive with its (simulated) current position:
            Vector3 curPosition = OpenMPD_ContextManager.Instance().GetPositionsDescriptor(this.primitiveID).getCurrentSimulatedPosition(visualUpdateSpeed);
            curPosition.y *= -1;//(See (1) below)
            this.gameObject.transform.GetChild(0).localPosition = curPosition;
        }
        /** (1) This Y inversion is nasty, but its a simple way of fixing a hard problem: 
         *  -  The levitator operates on a right-hand system of reference, and Unity operates in a left-handed one. The node LevitatorOrigin applies that transfiormation. 
         *  -  The physical bead position is computed by multiplying descriptors position by their matrices, making them relative to the levitator (origin, Right handed)
         *  -  The "simulated" bead position is also computed by multiplying descriptors by their parents' matrices. However, they are made relative to the camera. 
         *     Thus, the left to right hand transformation in LevitatorOrigin is never applied. 
         *  With this, we define the descriptor in "left-handed" space. Once we multiply by the parents' matrices, it all stays nice and left-handy as Unity likes it. 
         *  There would be other solutions, but they were disregarded.
         *     - Force all primitives to be children of LevitatorOrigin (i.e., LevitatorOrigin becomes the "root"). This solves the probolem, but is restrictive. It might cause integration isues with other systems (VR, tracking, etc). 
         *     - Make OpenMPD left handed. Nope. Over my dead body. Right handed is the way to go, I do not care what Unity says (OpenMPD's developer opinion here). 
         *     - Juggle with two definitions. Once we create a descriptor, we could chaini all the transformations (descriptor -> world ->levitatorOrigin) and take it back to the primitive (having applied left-2-right transformation once). 
         *       This is just silly and we end up with redundant definitions, just to rendeer a visual aid... Not worth it.          
         */
    }

 

    /**
        Declares the Primitive as enabled, but changes will not take place until "Commit" is called.
        This allows clients to "group" a set of changes (e.g. for continuity of trap positions, etc).
    */
    private void OnEnable()
    {
        if (AllSetup())
        {
            OpenMPD_PresentationManager.Instance().SetContentEnabled(this, true);
            OpenMPD_PresentationManager.Instance().RequestCommit();           
        }
    }
    /**
        Declares the Primitive as disabled, but changes will not take place until "Commit" is called.
        This allows clients to "group" a set of changes (e.g. for continuity of trap positions, etc).
    */
    private void OnDisable()
    {
        if (AllSetup())
        {
            OpenMPD_PresentationManager.Instance().SetContentEnabled(this, false);
            OpenMPD_PresentationManager.Instance().RequestCommit();            
        }
    }   
    public void FillCurrentPBDUpdate(OpenMPD_RenderingUpdate update)
    {
       update.UpdatePrimitive(primitiveID, prevMatrix, curMatrix);
    }

    //Helper methods for Matrix smoothing and management: 
    public static Quaternion GetRotation(Matrix4x4 matrix)
    {        
        return Quaternion.LookRotation(matrix.GetColumn(2), matrix.GetColumn(1));//
    }
    public static Vector4 GetPosition(Matrix4x4 matrix)
    {
        return matrix.GetColumn(3);
    }
    public static Vector4 InterpolatePositionCapped(Vector4 current, Vector4 target, float maxStepSize)
    {
        Vector4 difference = target - current;
        float distance = difference.magnitude;
        if (distance > maxStepSize)
        {
            Vector4 direction = difference.normalized;
            return current + maxStepSize * direction;
        }
        else
            return target;
    }

    public static Quaternion InterpolateOrientationCapped(Quaternion current, Quaternion target, float maxAngle)
    {
        float angleDifference = Quaternion.Angle(current, target);
        if (angleDifference > maxAngle)
            return Quaternion.Slerp(current, target, maxAngle / angleDifference);
        else return target;
    }

    public static Matrix4x4 BuildMatrix(Quaternion rot, Vector4 pos)
    {
        return Matrix4x4.TRS(new Vector3(pos.x, pos.y, pos.z), rot, new Vector3(1, 1, 1));
    }

    public static Quaternion ExtractRotationFromMatrix(ref Matrix4x4 matrix)
    {
        Vector3 forward;
        forward.x = matrix.m02;
        forward.y = matrix.m12;
        forward.z = matrix.m22;

        Vector3 upwards;
        upwards.x = matrix.m01;
        upwards.y = matrix.m11;
        upwards.z = matrix.m21;

        return Quaternion.LookRotation(forward, upwards);
    }

}
