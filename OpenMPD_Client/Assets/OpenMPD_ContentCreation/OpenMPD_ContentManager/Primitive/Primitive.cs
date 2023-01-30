using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using MyBox;

public class Primitive : MonoBehaviour
{
    private uint primitiveID;
    //Matrix update controls (currently specified by user, they could be obtained from OpenMPD_PresentationManager)
    public float maxStepInMeters = 0.00025f;
    public float maxRotInDegrees = 1.0f;

    //Matrices are made relative to the location/alignment of the Levitator 
    Transform levitatorOrigin = null;
    protected Matrix4x4 prevMatrix, curMatrix;
    protected bool PBD_Status;
    //[HideInInspector] public Quaternion Rotation;
    //[HideInInspector] public Vector3 Position;

    // this deals witht eh difference between coordinate systems
    
    Matrix4x4 OriginWorldToLocal = new Matrix4x4();
    public bool invertZ = false;

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
    /**
        The content declares the Postion and Amplitude buffers that it will use.
        If no implementation is provided, the default (fixed) descriptors will be used. 
    */
    protected virtual void ConfigureDescriptors() {
        SetDescriptors(OpenMPD_ContextManager .Instance().GetDefaultPositionsDescriptor(), OpenMPD_ContextManager .Instance().GetDefaultAmplitudesDescriptor());
        SetColourDescriptor(OpenMPD_ContextManager .Instance().GetDefaultColoursDescriptor());
    }

    public void SetDescriptors(uint positionDescID, uint amplitudeDescID, uint startingPositionSample=0, uint startingAmplitudeSample=0) {
        //Chage primitive state:
        OpenMPD_ContextManager .Instance().UseDescriptors(this.GetPrimitiveID(), positionDescID, amplitudeDescID, startingPositionSample, startingAmplitudeSample);
    }

    public void SetPositionDescriptor(uint positionDescID, uint startingPositionSample = 0)
    {
        //Chage primitive state:
        OpenMPD_ContextManager .Instance().UsePositionsDescriptor(this.GetPrimitiveID(), positionDescID, startingPositionSample);
    }

    public void SetAmplitudesDescriptor(uint amplitudeDescID, uint startingAmplitudeSample = 0)
    {
        //Chage primitive state:
        OpenMPD_ContextManager .Instance().UseAmplitudesDescriptor(this.GetPrimitiveID(), amplitudeDescID, startingAmplitudeSample);
    }
    public void SetColourDescriptor(uint colourDescID, uint startingColourSample = 0)
    {
        //Chage primitive state:
        OpenMPD_ContextManager .Instance().UseColoursDescriptor(this.GetPrimitiveID(), colourDescID, startingColourSample);
    }

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
            OriginWorldToLocal = levitatorOrigin.worldToLocalMatrix;
            curMatrix = OriginWorldToLocal * primLocalMat;

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
        prevMatrix = curMatrix = OriginWorldToLocal * targetPos_world;
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
            //1. Update the actual primitive (device)
            //Update matrices, keeping the (angle, distance) contraints specified. 
            prevMatrix = curMatrix;

            // get the particles position relative to the levitatorOrigin
            Matrix4x4 primLocalMat = transform.localToWorldMatrix;
            //getAdjustedMatrix(ref primLocalMat);
            // get the transformation matrix to go from the primitive to the levitator origin M_from-prim_to-ori 
            Matrix4x4 target_M_LocalToLevitator = OriginWorldToLocal * primLocalMat;


            // get rotations
            Quaternion prevRot = GetRotation(prevMatrix);
            Quaternion targetRot = GetRotation(target_M_LocalToLevitator);

            // get positions
            Vector4 prevPos = GetPosition(prevMatrix);
            Vector4 targetPos = GetPosition(target_M_LocalToLevitator);

            if (invertZ)
                FromRightToLeftCoord(ref targetPos, ref targetRot);


            curMatrix = BuildMatrix(InterpolateOrientationCapped(prevRot, targetRot, this.maxRotInDegrees)
                      , InterpolatePositionCapped(prevPos, targetPos, this.maxStepInMeters));

            //2. Update the bead we use to represent the primitive with its (simulated) current position:
            this.gameObject.transform.GetChild(0).localPosition = OpenMPD_ContextManager.Instance().GetPositionsDescriptor(this.primitiveID).getCurrentSimulatedPosition();

        }
    }

    //Inverts z axis to transform from right hand to left hand coordinate system(position and rotation)
    void FromRightToLeftCoord(ref Vector4 pos, ref Quaternion rot)
    {
        pos.y *= -1;
        rot.Set(rot.x, -rot.y, rot.z, -rot.w);
    }
    //void FromRightToLeftCoord(ref Vector4 pos, ref Quaternion rot)
    //{
    //    pos = new Vector4(pos.x, pos.z, pos.y, pos.w);
    //    //rot.Set(-rot.x, -rot.z, -rot.y, rot.w);
    //    rot.Set(-rot.x, -rot.y, rot.z, rot.w);
    //}
    void FromRigthToLeftCoord(ref Matrix4x4 matrix)
    {
        /* invert z tranlation and rotation
         [ M00,  M01, -M02,  M03]
         [ M10,  M11, -M12,  M13]
         [-M20, -M21,  M22, -M23]
         [ M30,  M31,  M32,  M33]
         */
        matrix.m02 *= -1;
        matrix.m12 *= -1;
        matrix.m20 *= -1;
        matrix.m21 *= -1;
        matrix.m23 *= -1;
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
            PBD_Status = true;

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
            PBD_Status = false;
        }
    }

    public bool PrimitiveEnabled() {
        return PBD_Status ;
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
