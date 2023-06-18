using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[NodeTint(180, 70, 70)]
[NodeWidth(250)]
[CreateNodeMenu(AEConsts.MENU_OPENMPD_STRING + "Primitive", AEConsts.MENU_OPENMPD_OFFSET + 0)]
// This node represents a primitive within the OpenMPD environment.
// When the game is running this class registers a Primitive with the OpenMPD library and sets it's state according to the input parameters.
//
// The enable state, position, rotation, and scale inputs to this node are sampled every Unity Update call
// (when the graph is loaded in the scene with an AnimationSceneGraph MonoBehaviour script)
// And they are used to calculate a new value for the Primitive's transform (based on the max speed parameters)
//
// The isEnabled output is set to the current enabled state of the primitive
// If the primitive is registered within OpenMPD, the particlePrimitive returns this object, otherwise null.
public class PrimitiveNode : DataNode, IPrimitive
{
    [ShowOnlyAttribute]
	public uint primitiveID = 0;
    //Matrix update controls (currently specified by user, they could be obtained from OpenMPD_PresentationManager)
    public float maxStepInMeters = 0.00025f;
    public float maxRotInDegrees = 1.0f;

    protected Matrix4x4 prevMatrix, curMatrix;
    //Matrices are made relative to the location/alignment of the Levitator 
    Transform levitatorOrigin = null;
    public bool invertZ = false;
    Matrix4x4 OriginWorldToLocal = new Matrix4x4();

    [Input(ShowBackingValue.Always, ConnectionType.Override, TypeConstraint.Inherited)] public bool setEnabled = true;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 scenePosition = Vector3.zero;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 rotation = Vector3.zero;
	[Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public Vector3 scale = Vector3.one;
    [Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public bool isEnabled = false;
    [Output(ShowBackingValue.Never, ConnectionType.Multiple, TypeConstraint.Inherited)] public PrimitiveNode particlePrimitive;

	
    public override object GetValue(NodePort port)
	{
        if (port.fieldName == "particlePrimitive")
        {
            if (!AllSetup())
            {
                Setup();
                if (!AllSetup())
                {
                    return null;
                }
            }
            return this;
        }
        if (port.fieldName == "isEnabled")
        {
            return isEnabled;
        }
        return null;
	}

    public uint GetPrimitiveID()
    {
        return primitiveID;
    }

    public uint GetPositionsDescriptorID()
    {
        return OpenMPD_ContextManager.Instance().GetPositionsDescriptorID(GetPrimitiveID());
    }
    public uint GetAmplitudesDescriptorID()
    {
        return OpenMPD_ContextManager.Instance().GetAmplitudesDescriptorID(GetPrimitiveID());
    }
    public uint GetColoursDescriptorID()
    {
        return OpenMPD_ContextManager.Instance().GetColoursDescriptorID(GetPrimitiveID());
    }

    public virtual void ConfigureDescriptors()
    {
        SetDescriptors(OpenMPD_ContextManager.Instance().GetDefaultPositionsDescriptor(), OpenMPD_ContextManager.Instance().GetDefaultAmplitudesDescriptor());
        SetColourDescriptor(OpenMPD_ContextManager.Instance().GetDefaultColoursDescriptor());
    }

    public void SetDescriptors(uint positionDescID, uint amplitudeDescID, uint startingPositionSample = 0, uint startingAmplitudeSample = 0)
    {
        //Chage primitive state:
        OpenMPD_ContextManager.Instance().UseDescriptors(this.GetPrimitiveID(), positionDescID, amplitudeDescID, startingPositionSample, startingAmplitudeSample);
    }

    public void SetPositionDescriptor(uint positionDescID, uint startingPositionSample = 0)
    {
        //Chage primitive state:
        OpenMPD_ContextManager.Instance().UsePositionsDescriptor(this.GetPrimitiveID(), positionDescID, startingPositionSample);
    }

    public void SetAmplitudesDescriptor(uint amplitudeDescID, uint startingAmplitudeSample = 0)
    {
        //Chage primitive state:
        OpenMPD_ContextManager.Instance().UseAmplitudesDescriptor(this.GetPrimitiveID(), amplitudeDescID, startingAmplitudeSample);
    }
    public void SetColourDescriptor(uint colourDescID, uint startingColourSample = 0)
    {
        //Chage primitive state:
        OpenMPD_ContextManager.Instance().UseColoursDescriptor(this.GetPrimitiveID(), colourDescID, startingColourSample);
    }

    public bool PrimitiveEnabled()
    {
        return isEnabled;
    }

    public bool AllSetup()
    {
        return levitatorOrigin != null && OpenMPD_ContextManager.Instance() != null; ;
    }

   public void Setup()
    {
    	if (OpenMPD_PresentationManager.Instance())
            levitatorOrigin = OpenMPD_PresentationManager.Instance().GetLevitatorNode();
        if (AllSetup())
        {
            primitiveID = OpenMPD_PresentationManager.Instance().CreateContent(this);
            //Template method for subclasses to declare their content's geometry, sound.etc...
            ConfigureDescriptors();
            //Initialize:                        
            // get the particles position relative to the levitatorOrigin
            setEnabled = GetInputValue<bool>("SetEnabled", setEnabled);
            scenePosition = GetInputValue<Vector3>("scenePosition", this.scenePosition);
            rotation = GetInputValue<Vector3>("rotation", this.rotation);
            scale = GetInputValue<Vector3>("scale", this.scale);
            Matrix4x4 primLocalMat = Matrix4x4.TRS(-scenePosition, Quaternion.Euler(rotation), scale).inverse;
            OriginWorldToLocal = levitatorOrigin.worldToLocalMatrix;
            curMatrix = OriginWorldToLocal * primLocalMat;

            SetPrimitiveEnabled(setEnabled);
        }
    }

    // Use this for initialization
    public void OnStart()
    {
        isEnabled = false;
        if (!AllSetup())
        {
            Setup();
        }
    }

    public void OnDestroy()
    {
        if (AllSetup())
        {
            setEnabled = false;
            SetPrimitiveEnabled(false);
            OpenMPD_PresentationManager.Instance().RemoveContent(this);
            this.primitiveID = 0;
        }
    }

    protected override void OnDirtyUpdate()
    {
        setEnabled = GetInputValue<bool>("setEnabled", setEnabled);
        scenePosition = GetInputValue<Vector3>("scenePosition", this.scenePosition);
        rotation = GetInputValue<Vector3>("rotation", this.rotation);
        scale = GetInputValue<Vector3>("scale", this.scale);
    }

    public void OnUpdate()
    {
        if (!AllSetup())
        {
            Setup();
        }
        else
        {
            //Update1 matrices, keeping the (angle, distance) contraints specified. 
            prevMatrix = curMatrix;

            setEnabled = GetInputValue<bool>("setEnabled", setEnabled);
            // get the particles position relative to the levitatorOrigin
            scenePosition = GetInputValue<Vector3>("scenePosition", this.scenePosition);
            rotation = GetInputValue<Vector3>("rotation", this.rotation);
            scale = GetInputValue<Vector3>("scale", this.scale);
            //Matrix4x4 primLocalMat = GameObject.Find("AnchorNode").transform.localToWorldMatrix;
            Matrix4x4 primLocalMat = Matrix4x4.TRS(-scenePosition, Quaternion.Euler(rotation), scale).inverse;
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

            //Rotation = this.transform.rotation;
            //Position = this.transform.position;
            
            SetPrimitiveEnabled(setEnabled);
        }
    }

    public void OnStop()
    {

    }

    public void FillCurrentPBDUpdate(OpenMPD_RenderingUpdate update)
    {
        update.UpdatePrimitive(primitiveID, prevMatrix, curMatrix);
    }

    public void SetPrimitiveEnabled(bool setEnabled)
    {
        OpenMPD_PresentationManager.Instance().SetContentEnabled(this, setEnabled);
        OpenMPD_PresentationManager.Instance().RequestCommit();
        isEnabled = setEnabled;
    }

    public void TeleportPrimitive(Matrix4x4 targetPos_world)
    {
        if (!AllSetup())
            Setup();
        // Update interpolation matrices we use:
        prevMatrix = curMatrix = OriginWorldToLocal * targetPos_world;
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

    //Helper methods for Matrix smoothing and management: 
    public static Quaternion GetRotation(Matrix4x4 matrix)
    {
        if (matrix.GetColumn(2).Equals(matrix.GetColumn(1)))
        {
            return Quaternion.identity;
        }
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