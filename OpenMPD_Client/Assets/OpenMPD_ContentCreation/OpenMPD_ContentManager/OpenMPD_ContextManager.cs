using UnityEngine;
using System.Collections;
using System.Collections.Generic;
/**
 This class keeps track of the (Position/Amplitude) descriptors declared in the system.
 It also keeps track of the descriptors associated to each Primitive. 
*/
public class OpenMPD_ContextManager
{
    //Helper default Position Descriptor: 
    Positions_Descriptor DefaultPositionsDescriptor ;
    Amplitudes_Descriptor DefaultAmplitudesDescriptor ;
    Colours_Descriptor DefaultColoursDescriptor;
    //Variables to store our registry of descriptors and descriptors' assignemnts.
    private static OpenMPD_ContextManager  _theInstance = null;
    private Dictionary<uint, Positions_Descriptor> positionDescriptors;
    private Dictionary<uint, Amplitudes_Descriptor> amplitudeDescriptors;
    private Dictionary<uint, Colours_Descriptor> colourDescriptors;
    private Dictionary<uint, uint> StaringPosDescriptor;
    private Dictionary<uint, uint> StaringColDescriptor;
    OpenMPD_PresentationManager renderingManager=null;
    class DescriptorsUsedByPrimitive {
        public uint primitiveID;
        public uint amplitudesDescriptorID;
        public uint positionsDescriptorID;
        public uint coloursDescriptorID;
        public DescriptorsUsedByPrimitive(uint primitiveID, uint positionsID, uint amplitudesID) {
            this.primitiveID = primitiveID;
            amplitudesDescriptorID = amplitudesID;
            positionsDescriptorID = positionsID;
            coloursDescriptorID = 0;
        }
    };
    Dictionary<uint, DescriptorsUsedByPrimitive> descriptorsByPrimitive;
    protected OpenMPD_ContextManager() {
        positionDescriptors = new Dictionary<uint, Positions_Descriptor>();
        amplitudeDescriptors = new Dictionary<uint, Amplitudes_Descriptor>();
        colourDescriptors = new Dictionary<uint, Colours_Descriptor>();

        descriptorsByPrimitive = new Dictionary<uint, DescriptorsUsedByPrimitive>();
        StaringPosDescriptor = new Dictionary<uint, uint>();
        StaringColDescriptor = new Dictionary<uint, uint>();

        renderingManager = OpenMPD_PresentationManager.Instance();
    }

    public static OpenMPD_ContextManager  Instance() {
        if (_theInstance == null && OpenMPD_PresentationManager.Instance())
        {//We are not initialized yet, but the Rendering Manager is already ready.
            _theInstance = new OpenMPD_ContextManager ();
        }

        return _theInstance;
    }

    /**
        Declares a descriptor so that contents can use them. 
        Contents are also free to swap and start using other (Position/Amplitude) descriptors
        For instance, a content can change its AmplitudesDescriptor, to start using one creating audible sound. 
        Alternatively, they can change their PositionsDesriptor to trace a different shape.
    */
    public uint DeclareDescriptor(Positions_Descriptor d) {
        //0. Declare the descriptor
        uint positionsDescriptorID=OpenMPD_Wrapper.OpenMPD_CWrapper_createPositionsDescriptor(renderingManager.getCurrentEngineHandler(), d.positions, d.positions.Length/4);
        //2. Add to current database. 
        positionDescriptors.Add(positionsDescriptorID, d);
        return positionsDescriptorID;

    }
    /**
        Declares a descriptor so that contents can use them. 
        Contents are also free to swap and start using other (Colour/Position/Amplitude) descriptors
        For instance, a content can change its AmplitudesDescriptor, to start using one creating audible sound. 
        Alternatively, they can change their PositionsDesriptor to trace a different shape.
    */
    public uint DeclareDescriptor(Colours_Descriptor d)
    {
        //0. Declare the descriptor
        uint coloursDescriptorID = OpenMPD_Wrapper.OpenMPD_CWrapper_createColoursDescriptor(renderingManager.getCurrentEngineHandler(), d.colours, d.colours.Length / 4);
        //2. Add to current database. 
        colourDescriptors.Add(coloursDescriptorID, d);
        return coloursDescriptorID;

    }
    public uint DeclareDescriptor(Amplitudes_Descriptor a)
    {
        uint amplitudesDescriptorID=OpenMPD_Wrapper.OpenMPD_CWrapper_createAmplitudesDescriptor(renderingManager.getCurrentEngineHandler(), a.amplitudes, a.amplitudes.Length);
        amplitudeDescriptors.Add(amplitudesDescriptorID, a);
        return amplitudesDescriptorID;
    }

    
    /**
        Single method to declare the descriptors used by a content.
        The descriptors specified must exist (returns false otherwise).
        If the content already had descriptors associated, the method replaces them.
        If this is a brand new declaration (content had no descriptors associated), a new entry is created for this content
    */
    public bool UseDescriptors(uint primitiveID, uint positionsDescriptorID, uint amplitudesDescriptorID, uint firstPositionIndexToUse = 0, uint firstAmplitudeIndexToUse=0) {
        //1. Check that descriptors exist
        if (!positionDescriptors.ContainsKey(positionsDescriptorID)
            || !amplitudeDescriptors.ContainsKey(amplitudesDescriptorID))
            return false;
        //2. Check if content already had descriptors associated
        if (!descriptorsByPrimitive.ContainsKey(primitiveID))
        {   //Nope --> Create new entry
            descriptorsByPrimitive.Add(primitiveID, new DescriptorsUsedByPrimitive(primitiveID, positionsDescriptorID, amplitudesDescriptorID));
        }
        else {
            //Yep --> Update existing information:
            //a. Decrease references to previous descriptors used 
            GetPositionsDescriptor(primitiveID).DecreaseReference();
            GetAmplitudesDescriptor(primitiveID).DecreaseReference();
            //b. Replace descriptors
            descriptorsByPrimitive[primitiveID].amplitudesDescriptorID = amplitudesDescriptorID;
            descriptorsByPrimitive[primitiveID].positionsDescriptorID = positionsDescriptorID;
            //c. Add references to new descriptors used.
            GetPositionsDescriptor(primitiveID).AddReference();
            GetAmplitudesDescriptor(primitiveID).AddReference();
        }
        if (!StaringPosDescriptor.ContainsKey(primitiveID))
            StaringPosDescriptor.Add(primitiveID, firstPositionIndexToUse);
        else
            StaringPosDescriptor[primitiveID] = firstPositionIndexToUse;

        //3. Once local definitions are updated, update the underlying C++ engine also:
        OpenMPD_Wrapper.OpenMPD_CWrapper_updatePrimitive_Amplitudes(renderingManager.getCurrentEngineHandler(), primitiveID, amplitudesDescriptorID, firstAmplitudeIndexToUse);
        OpenMPD_Wrapper.OpenMPD_CWrapper_updatePrimitive_Positions (renderingManager.getCurrentEngineHandler(), primitiveID, positionsDescriptorID, firstPositionIndexToUse);

        return true;
    }
    /**
        Methods used to swap only one of the Descriptors (Position/Amplitude).
        CAN ONLY BE USED IF CONTENT WAS ALREADY DECLARED (using "UseDescriptors" method).
        Returns false if either the content or the descriptor do not exist. 
     */
    public bool UseAmplitudesDescriptor(uint primitiveID, uint amplitudesDescriptorID, uint firstAmplitudeIndexToUse=0) {
        if (    !descriptorsByPrimitive.ContainsKey(primitiveID)
            ||  !amplitudeDescriptors.ContainsKey(amplitudesDescriptorID))
            return false;
        //2. Update descriptor used (and references (old -1; new +1))
        GetAmplitudesDescriptor(primitiveID).DecreaseReference();
        descriptorsByPrimitive[primitiveID].amplitudesDescriptorID = amplitudesDescriptorID;
        GetAmplitudesDescriptor(primitiveID).AddReference();
        //3. Update underlying C++ engine:
        OpenMPD_Wrapper.OpenMPD_CWrapper_updatePrimitive_Amplitudes(renderingManager.getCurrentEngineHandler(), primitiveID, amplitudesDescriptorID, firstAmplitudeIndexToUse);
        return true;
    }
    public bool UsePositionsDescriptor(uint primitiveID, uint positionsDescriptorID, uint firstPositionIndexToUse=0)
    {
        if (!descriptorsByPrimitive.ContainsKey(primitiveID)
            || !positionDescriptors.ContainsKey(positionsDescriptorID))
            return false;
        //2. Update descriptor used (and references (old -1; new +1))
        GetPositionsDescriptor(primitiveID).DecreaseReference();
        descriptorsByPrimitive[primitiveID].positionsDescriptorID= positionsDescriptorID;
        GetPositionsDescriptor(primitiveID).AddReference();
        if (!StaringPosDescriptor.ContainsKey(primitiveID))
            StaringPosDescriptor.Add(primitiveID, firstPositionIndexToUse);
        else
            StaringPosDescriptor[primitiveID] = firstPositionIndexToUse;

        //3. Update underlying C++ engine:
        OpenMPD_Wrapper.OpenMPD_CWrapper_updatePrimitive_Positions(renderingManager.getCurrentEngineHandler(), primitiveID, positionsDescriptorID, firstPositionIndexToUse);
        return true;
    }
    
    public bool UseColoursDescriptor(uint primitiveID, uint coloursDescriptorID, uint firstColourIndexToUse = 0)
    {
        if (!descriptorsByPrimitive.ContainsKey(primitiveID)
            || !colourDescriptors.ContainsKey(coloursDescriptorID))
            return false;
        //2. Update descriptor used (and references (old -1; new +1))
        if(GetColoursDescriptorID(primitiveID)!=0) 
            GetColoursDescriptor(primitiveID).DecreaseReference();
        descriptorsByPrimitive[primitiveID].coloursDescriptorID = coloursDescriptorID;
        GetColoursDescriptor(primitiveID).AddReference();
        if (!StaringColDescriptor.ContainsKey(primitiveID))
            StaringColDescriptor.Add(primitiveID, firstColourIndexToUse);
        else
            StaringColDescriptor[primitiveID] = firstColourIndexToUse;

        //3. Update underlying C++ engine:
        OpenMPD_Wrapper.OpenMPD_CWrapper_updatePrimitive_Colours(renderingManager.getCurrentEngineHandler(), primitiveID, coloursDescriptorID, firstColourIndexToUse);
        return true;
    }



    /**
     Destroys the declaration of a (Position/Amplitude) Descriptor, so that it will no
     longer be available for contents to use. 
     Only succeeds if no content is using it (checked via reference counting)
    */
    public bool RemoveDescriptor(Amplitudes_Descriptor d) {
        //Check that it exists and it is disposable (refCount==0)
        if (!amplitudeDescriptors.ContainsKey(d.amplitudesDescriptorID)
            || !d.IsDisposable())
            return false;
        amplitudeDescriptors.Remove(d.amplitudesDescriptorID);
        //Notify engine
        OpenMPD_Wrapper.OpenMPD_CWrapper_releaseAmplitudesDescriptor(renderingManager.getCurrentEngineHandler(), d.amplitudesDescriptorID);
        return true;
    }
    public bool RemoveDescriptor(Positions_Descriptor d)
    {
        //Check that it exists and it is disposable (refCount==0)
        if (!positionDescriptors.ContainsKey(d.positionsDescriptorID)
            || !d.IsDisposable())
            return false;
        positionDescriptors.Remove(d.positionsDescriptorID);
        OpenMPD_Wrapper.OpenMPD_CWrapper_releasePositionsDescriptor(renderingManager.getCurrentEngineHandler(), d.positionsDescriptorID);
        return true;
    }
    public bool RemoveDescriptor(Colours_Descriptor d)
    {
        //Check that it exists and it is disposable (refCount==0)
        if (!colourDescriptors.ContainsKey(d.coloursDescriptorID)
            || !d.IsDisposable())
            return false;
        colourDescriptors.Remove(d.coloursDescriptorID);
        OpenMPD_Wrapper.OpenMPD_CWrapper_releaseColoursDescriptor(renderingManager.getCurrentEngineHandler(), d.coloursDescriptorID);
        return true;
    }
    public void RemoveAllDescriptors() {
        foreach (KeyValuePair<uint, Positions_Descriptor> p in positionDescriptors)
            OpenMPD_Wrapper.OpenMPD_CWrapper_releasePositionsDescriptor(renderingManager.getCurrentEngineHandler(), p.Value.positionsDescriptorID);
        foreach (KeyValuePair<uint, Amplitudes_Descriptor> a in amplitudeDescriptors)
            OpenMPD_Wrapper.OpenMPD_CWrapper_releaseAmplitudesDescriptor(renderingManager.getCurrentEngineHandler(), a.Value.amplitudesDescriptorID);
        foreach (KeyValuePair<uint, Colours_Descriptor> c in colourDescriptors)
            OpenMPD_Wrapper.OpenMPD_CWrapper_releaseColoursDescriptor(renderingManager.getCurrentEngineHandler(), c.Value.coloursDescriptorID);

        positionDescriptors.Clear();
        amplitudeDescriptors.Clear();
        colourDescriptors.Clear();
    }

    //Accesibility methods: 
    public uint GetPositionsDescriptorID(uint primitiveID)
    {
        return descriptorsByPrimitive[primitiveID].positionsDescriptorID;
    }
    public uint GetAmplitudesDescriptorID(uint primitiveID)
    {
        return descriptorsByPrimitive[primitiveID].amplitudesDescriptorID;
    }
    public uint GetColoursDescriptorID(uint primitiveID)
    {
        return descriptorsByPrimitive[primitiveID].coloursDescriptorID;
    }
    public Positions_Descriptor GetPositionsDescriptor(uint primitiveID) {// this should be private
        //return positionDescriptors[descriptorsByPrimitive[primitiveID].positionsDescriptorID];
        Positions_Descriptor Desc;

        // method 1
        if (!positionDescriptors.TryGetValue(descriptorsByPrimitive[primitiveID].positionsDescriptorID, out Desc))
            Debug.Log("The Dictionary does not contain the intended Key");

        // method 2
        //if (positionDescriptors.ContainsKey(descriptorsByPrimitive[primitiveID].positionsDescriptorID))
        //    positionDescriptors.TryGetValue(descriptorsByPrimitive[primitiveID].positionsDescriptorID, out Desc);
        //else
        //    Debug.Log("The Dictionary does not contain the intended Key");
        
        return Desc;
    }
    

    private Amplitudes_Descriptor GetAmplitudesDescriptor(uint primitiveID)
    {
        return amplitudeDescriptors[GetAmplitudesDescriptorID(primitiveID)];
    }

    private Colours_Descriptor GetColoursDescriptor(uint primitiveID)
    {
        return colourDescriptors[GetColoursDescriptorID(primitiveID)];
    }
    public uint GetPBD_Position_Staring_Index(uint primitiveID)
    {
        uint index = 0;
        if (!StaringPosDescriptor.TryGetValue(primitiveID, out index))
            Debug.Log("The Dictionary does not contain the intended Key");
        return index;
        //return StaringPosDescriptor[primitiveID];
    }

    public uint GetDefaultPositionsDescriptor() {
        if (DefaultPositionsDescriptor == null)
            DefaultPositionsDescriptor = new Positions_Descriptor(new float[] { 0, 0, 0, 1.0f });
        return DefaultPositionsDescriptor.positionsDescriptorID;
    }
    public uint GetDefaultAmplitudesDescriptor() {
        if (DefaultAmplitudesDescriptor == null)
            DefaultAmplitudesDescriptor = new Amplitudes_Descriptor(new float[] { 10000.0f, 10000.0f, 10000.0f, 10000.0f });
        return DefaultAmplitudesDescriptor.amplitudesDescriptorID;
    }

    public uint GetDefaultColoursDescriptor()
    {
        if (DefaultColoursDescriptor == null)
            DefaultColoursDescriptor = new Colours_Descriptor(new float[] { 1.0f, 1.0f, 1.0f, 1.0f });
        return DefaultColoursDescriptor.coloursDescriptorID;
    }

}
