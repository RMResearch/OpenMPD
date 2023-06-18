using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using MyBox;

/* A common interface for Primitive-based content within the Unity OpenMPD library
 * This allows for PrimitiveGameObjects and library extensions or user code 
 * such as PrimitiveNode (from the graph design tool) to register themselves
 * with the OpenMPD_PresentationManager in a uniform way.
 * 
 * (C# does not allow for multiple inheritance and so this seemed the best way to group similiar objects
 */
public interface IPrimitive
{
    uint GetPrimitiveID();

    uint GetPositionsDescriptorID();
    uint GetAmplitudesDescriptorID();
    uint GetColoursDescriptorID();
    /**
        The content declares the Postion and Amplitude buffers that it will use.
        If no implementation is provided, the default (fixed) descriptors will be used. 
    */
    void ConfigureDescriptors(); // This was previously a protected method but has now been made public due to the interface approach

    void SetDescriptors(uint positionDescID, uint amplitudeDescID, uint startingPositionSample = 0, uint startingAmplitudeSample = 0);
    void SetPositionDescriptor(uint positionDescID, uint startingPositionSample = 0);

    void SetAmplitudesDescriptor(uint amplitudeDescID, uint startingAmplitudeSample = 0);
    void SetColourDescriptor(uint colourDescID, uint startingColourSample = 0);

    bool AllSetup();
    void Setup();

    void TeleportPrimitive(Matrix4x4 targetPos_world);

    bool PrimitiveEnabled();
    void FillCurrentPBDUpdate(OpenMPD_RenderingUpdate update);

}
