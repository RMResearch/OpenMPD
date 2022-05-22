using UnityEngine;
using System.Collections;

public class PrimitiveStateAsset : MonoBehaviour
{
    [Header("Primitive configuration")]
    public Primitive primitive;
    public PositionDescriptorAsset positionDescriptor;
    public uint startingPositionSample=0;
    public AmplitudeDescriptorAsset amplitudeDescriptor;
    public uint startingAmplitudeSample = 0;
    public ColourDescriptorAsset colourDescriptor;
    public uint startingColourSample = 0;

    // Use this for initialization
    void Start(){}
    // Update is called once per frame
    void Update(){

    }
    public bool EnterState() {

       // if (!enabled)
        {
            OpenMPD_ContextManager  cm = OpenMPD_ContextManager .Instance();
            //1. Access low level identifiers
            if (primitive == null || primitive.GetPrimitiveID() == 0)
                return false; //Primitive not initialized
            uint positionDescID = (positionDescriptor == null ? cm.GetDefaultPositionsDescriptor() : positionDescriptor.GetDescriptorID());
            uint amplitudeDescID = (amplitudeDescriptor == null ? cm.GetDefaultAmplitudesDescriptor() : amplitudeDescriptor.GetDescriptorID());
            uint colourDesID = (colourDescriptor == null ? cm.GetDefaultColoursDescriptor() : colourDescriptor.GetDescriptorID());
            
            //2. Chage primitive state:
            primitive.SetDescriptors(positionDescID, amplitudeDescID, startingPositionSample, startingAmplitudeSample);
            primitive.SetColourDescriptor(colourDesID, startingColourSample);
            OpenMPD_PresentationManager.Instance().SetContentEnabled(primitive, true);
        }
        this.enabled = true;
        return true;
    }

    public void LeaveState() {
        if(enabled)
            OpenMPD_PresentationManager.Instance().SetContentEnabled(this.primitive, false);
        this.enabled = false;
    }
}
