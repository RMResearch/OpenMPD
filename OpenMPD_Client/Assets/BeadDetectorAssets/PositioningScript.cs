using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using MyBox;
using Assets.Utilities;

public class PositioningScript : MonoBehaviour
{
    /*public*/ BeadDetector_Controller detector;
    [Separator("Trigger detection and lifting:")]
    public bool clickToInitialize = false;
   
    //public bool isGradualPick = false;
    //public bool isSingingPick = false;
    //public bool isFixAmpPick = false;
    public Vector3 targetPos = new Vector3(0.02f, 0, 0);

    [Separator("Configuration:")]
    public float transducersOFF_Time = 1;
    public float holdTime = 0.5f;
    public float liftTime = 1.0f;
    public float postLiftWait = 0.2f;
    public float liftStepSize = 0.0001f;
    public float liftHeight = 0.01f;
    float moveStepSize ;//This gets initialized to the primitive's move step value (to restore when we finish) 

    public int step = 0;
    const int DISABLE_PRIMITIVES=0;         //Primitives are turned off, so that beads don't wiggle
    const int TELEPORT_PRIMITIVES =1;       //Traps moved to where particles are (as seen by BeadDetector)
    const int ENABLE_PRIMITIVES=2;          //Traps are re-enabled, as to trap beads in place
    const int STABILIZE_PRIMITIVES = 3;     //Traps stay there for a bit, for particles to stabilize
    const int LIFT_PRIMITIVES = 4;          //Traps gently lifted from surface
    const int MOVE_PRIMITIVES = 5;          //Move to target location.    
    //TIME VARIABLES CONTROLLING MOVEMENT OF PRIMITIVE TO TARGET: 
    //Values computed from the time when we start positionig plus the user-defined durations defined above (holdTime, liftTime...)
    float trapPrimitivesTime = 0;
    float stabilizePrimitivesTime = 0;
    float liftPrimitivesTime = 0;
    GameObject bead;

    [Separator("Gradual picking:")]
    public float startAmplitudeInPa = 1000;
    public float endAmplitudeInPa = 15000;
    public float fixAmplitudeInPa = 10000;
    public int frequency = 200;
    [Separator(" 0=none 1=Gradual 2=Singing 3=FixAmp")]
    public int ampPickMode = 0; // 0= none, 1=GradualPick, 2=SingingPick, 3=FixAmpPick

    Amplitudes_Descriptor gradualPick;
    Amplitudes_Descriptor singingHold;
    Amplitudes_Descriptor fixAmpPick;
    int prevDesciptor = 0;
    ReadWriteData writer = new ReadWriteData();
    AmpController ampHandler;

    uint previousAmpDescriptor;//We will keep it here to restore it
    void Start()
    {
        gradualPick = null;
        singingHold = null;
        fixAmpPick = null;
        ampHandler = new AmpController(10000, startAmplitudeInPa, endAmplitudeInPa, fixAmplitudeInPa);
        Debug.Log("Amp Handler: Generaed");

        detector = this.GetComponent<BeadDetector_Controller>();
        bead = GameObject.FindGameObjectsWithTag("primitive")[0];//I think you can look for object types (to avoid needing to "tag" stuff)
                                                                //Try: GameObject.FindObjectOfType<Primitive>();
        clickToInitialize = false;   
    }

    void Update()
    {
        // check amplitude configuration changes and update the descriptors
        checkAmpValuesForChanges();

        if (detector.isActive() && clickToInitialize)
        {
            switch (step) {
                case DISABLE_PRIMITIVES:
                    disablePrimitive();
                    break;
                case TELEPORT_PRIMITIVES:
                    teleportPrimitives();
                    break;
                case ENABLE_PRIMITIVES:
                    enablePrimitives();
                    break;
                case STABILIZE_PRIMITIVES:
                    stabilizePrimitives();
                    break;
                case LIFT_PRIMITIVES:
                    liftPrimitives();
                    break;
                case MOVE_PRIMITIVES:
                    movePrimitives();
                    break;
            }

        }
        else {
            clickToInitialize = false;
            step = DISABLE_PRIMITIVES;
        }
    }

    private void disablePrimitive() {
        //Disable the primitives (board will turn transducers off)     
        DisableAxesCorrection(bead);
        bead.SetActive(false);

        Debug.Log("POSITIONING: Primitives disabled in "+Time.realtimeSinceStartup);
        step = TELEPORT_PRIMITIVES;
        //Give some time to have transducers off and stop particles from dancing around...    
        trapPrimitivesTime = Time.realtimeSinceStartup + transducersOFF_Time;       
    }

    private void teleportPrimitives()
    {
        //Wait until we need to trap the particles
        if (Time.realtimeSinceStartup < trapPrimitivesTime)
            return;
        //Check if we have particles in sight and move trap there
        int numBeads = detector.detectBeads();
        float[] beadPositions = detector.getCurrentBeadPositions();
        if (numBeads == 0)
            return;
        //Start trapping:
        Matrix4x4 targetPos = Matrix4x4.identity;
        targetPos.SetColumn(3, new Vector4(beadPositions[0], beadPositions[1], beadPositions[2]*-1, 1));
        bead.GetComponent<PrimitiveGameObject>().TeleportPrimitive(targetPos);
        //Initialize times for current and next steps (e.g. stabilize, lift, move)
        stabilizePrimitivesTime = Time.realtimeSinceStartup + holdTime;    //Give some time for particle to stabilize, once trapped
        liftPrimitivesTime = stabilizePrimitivesTime + liftTime;    //Give some time to lift the particle
        Debug.Log("POSITIONING: Primitives teleported in " + Time.realtimeSinceStartup);
        step = ENABLE_PRIMITIVES;
    }

    private void enablePrimitives()
    {
        //Enable the primitive to enable its trap:
        //bead.GetComponent<Primitive>().enabled = true;
        bead.SetActive(true);
        
        //p.SetAmplitudesDescriptor(singingHold.amplitudesDescriptorID);
        UpdateAmplitudeDescriptor();

        Debug.Log("POSITIONING: Primitives re-enabled in "+Time.realtimeSinceStartup);
        step = STABILIZE_PRIMITIVES;
    }

    private void stabilizePrimitives()  
    {        
        //Wait until stabilize deadline time
        if (Time.realtimeSinceStartup < stabilizePrimitivesTime)
            return;
        //Transition to next state:
        moveStepSize = bead.GetComponent<PrimitiveGameObject>().maxStepInMeters; //Save its step size (we'll restore it once lift is finished). 
        bead.GetComponent<PrimitiveGameObject>().maxStepInMeters = liftStepSize;
        bead.transform.position = bead.transform.position + new Vector3(0,liftHeight,0);
        Debug.Log("POSITIONING: Begin lifting Primitives in "+Time.realtimeSinceStartup);
        step = LIFT_PRIMITIVES;
    }

    private void liftPrimitives() {
        //Wait until stabilize deadline time
        if (Time.realtimeSinceStartup < liftPrimitivesTime)
            return;
        //Transition to next state:
        step = MOVE_PRIMITIVES;
        Debug.Log("POSITIONING: Primitives stabilizign after lifting in " + Time.realtimeSinceStartup);
        bead.GetComponent<PrimitiveGameObject>().maxStepInMeters = moveStepSize;
    }

    private void movePrimitives() {
        //Move the primitive to target position
        bead.transform.position = targetPos;// new Vector3(targetPos.x, targetPos.y, targetPos.z);
        EnableAxesCorretion(bead);
        Debug.Log("POSITIONING: Primitives moving in " + Time.realtimeSinceStartup);
        clickToInitialize = false;
    }

    // Utilities
    private void UpdateAmplitudeDescriptor()
    {
        //Enable the primitive component
        //bead.GetComponent<Primitive>().enabled = true;
        PrimitiveGameObject p = bead.GetComponent<PrimitiveGameObject>();

        switch (ampPickMode)
        {
            case 0:
                break;
            case 1://gradual pick
                p.SetAmplitudesDescriptor(gradualPick.amplitudesDescriptorID);
                Debug.Log("Amp Handler: Amplitud desctiptors Updated -> gradualPick");
                break;
            case 2://singing pick
                p.SetAmplitudesDescriptor(singingHold.amplitudesDescriptorID);
                Debug.Log("Amp Handler: Amplitud desctiptors Updated -> singingHold");
                break;
            case 3://fix amp pick
                p.SetAmplitudesDescriptor(fixAmpPick.amplitudesDescriptorID);
                Debug.Log("Amp Handler: Amplitud desctiptors Updated -> fixAmpPick");
                break;
        }       
    }
        
    private void checkAmpValuesForChanges()
    {
        bool clearAmp = false;
        if (startAmplitudeInPa != ampHandler.GetMinAmpInPa() || endAmplitudeInPa != ampHandler.GetMaxAmpInPa())
        {
            Debug.Log("Amp Handler: Updating gradual pick range : (" + startAmplitudeInPa.ToString() +" ,"+endAmplitudeInPa.ToString()+")");
            clearAmp = true;
        }

        if (fixAmplitudeInPa != ampHandler.GetFixAmpInPa())
        {
            Debug.Log("Amp Handler: Updating FixAmp pick : Value -> " + fixAmplitudeInPa.ToString());
            clearAmp = true;
        }

        if (frequency != ampHandler.GetFrequencyHz())
        {
            Debug.Log("Amp Handler: Updating Singing pick : Value -> " + frequency.ToString());
            clearAmp = true;
        }

        if (clearAmp)
            clearAmpDescriptors();

        if (gradualPick == null || singingHold == null || fixAmpPick == null)
            defineAmpDescriptors();

        if (ampPickMode != prevDesciptor)
        {
            prevDesciptor = ampPickMode;
            UpdateAmplitudeDescriptor();
        }
    }
    private void clearAmpDescriptors()
    {
        ampHandler.UpdateAmplitudeRange(10000, startAmplitudeInPa, endAmplitudeInPa, fixAmplitudeInPa, frequency);
        Debug.Log("Amp Handler: Amplitud Range updated");
        gradualPick = null;
        singingHold = null;
        fixAmpPick = null;
        Debug.Log("Amp Handler: Amplitud desctiptors are now null");
    }

    private void defineAmpDescriptors()
    {
        gradualPick = new Amplitudes_Descriptor(ampHandler.GetGradualPick(holdTime));
        singingHold = new Amplitudes_Descriptor(ampHandler.getSingingAmpDesc());
        fixAmpPick = new Amplitudes_Descriptor(ampHandler.getFixAmpDesc(fixAmplitudeInPa));
        Debug.Log("Amp Handler: Amplitud desctiptors reconfigured and active");
    }

    private void DisableAxesCorrection( GameObject particle)
    {
        ;//particle.GetComponent<Primitive>().invertZ = false;       
    }

    private void EnableAxesCorretion(GameObject particle)
    {
        ;// particle.GetComponent<Primitive>().invertZ = true;
    }
}
