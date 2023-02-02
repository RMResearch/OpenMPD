using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using MyBox;
using Assets.Utilities;

public class Positioning : MonoBehaviour
{
    /*public*/ BeadDetector_Controller detector;
    [Separator("Trigger detection and lifting:")]
    public bool clickToInitialize = false;
    public bool isGradualPick = false;
    public bool isSingingPick = false;
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
    
    Amplitudes_Descriptor gradualPick;
    Amplitudes_Descriptor singingHold;
    ReadWriteData writer = new ReadWriteData();
    AmpController ampHandler;


    uint previousAmpDescriptor;//We will keep it here to restore it
    void Start()
    {
        gradualPick = null;
        singingHold = null;
        ampHandler = new AmpController(10000, 100, 15000, 100000, 200);

        detector = this.GetComponent<BeadDetector_Controller>();
        bead = GameObject.FindGameObjectsWithTag("primitive")[0];//I think you can look for object types (to avoid needing to "tag" stuff)
                                                                //Try: GameObject.FindObjectOfType<Primitive>();
        clickToInitialize = false;   
    }

    void Update()
    {
        if (gradualPick == null && singingHold==null) {
            //int numSamples = (int)(10000 * holdTime);
            //float[] amplitudes = new float[numSamples];
            //for (int s = 0; s < numSamples; s++)
            //    amplitudes[s] = startAmplitudeInPa + s * (endAmplitudeInPa - startAmplitudeInPa) / numSamples;
            //gradualPick = new Amplitudes_Descriptor(amplitudes);
            gradualPick = new Amplitudes_Descriptor(ampHandler.GetGradualPick(holdTime));

            //float freq = 200;
            //int numSamples2 = (int)(10000 / freq);
            //float[] amplitudes2 = new float[numSamples2];
            //for (int s = 0; s < numSamples2; s++)
            //    amplitudes2[s] = 10000 +(float) (5000 * Math.Cos((2 * Math.PI * s) / numSamples2));
            //singingHold = new Amplitudes_Descriptor(amplitudes2);
            singingHold = new Amplitudes_Descriptor(ampHandler.getSingingAmpDesc());
        }

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
        //Primitive p = bead.GetComponent<Primitive>();
        //previousAmpDescriptor =p.GetAmplitudesDescriptorID();
        //if (isGradualPick) 
        //    p.SetAmplitudesDescriptor(gradualPick.amplitudesDescriptorID);
        //else if (isSingingPick)
        //    p.SetAmplitudesDescriptor(singingHold.amplitudesDescriptorID);
        //p.SetAmplitudesDescriptor( previousAmpDescriptor);
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
        bead.GetComponent<Primitive>().TeleportPrimitive(targetPos);
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
        Primitive p = bead.GetComponent<Primitive>();
        //p.SetAmplitudesDescriptor(singingHold.amplitudesDescriptorID);
        if(isGradualPick)
            p.SetAmplitudesDescriptor(gradualPick.amplitudesDescriptorID);
        else if (isSingingPick)
            p.SetAmplitudesDescriptor(singingHold.amplitudesDescriptorID);

        Debug.Log("POSITIONING: Primitives re-enabled in "+Time.realtimeSinceStartup);
        step = STABILIZE_PRIMITIVES;
    }

    private void stabilizePrimitives()  
    {        
        //Wait until stabilize deadline time
        if (Time.realtimeSinceStartup < stabilizePrimitivesTime)
            return;
        //Transition to next state:
        moveStepSize = bead.GetComponent<Primitive>().maxStepInMeters; //Save its step size (we'll restore it once lift is finished). 
        bead.GetComponent<Primitive>().maxStepInMeters = liftStepSize;
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
        bead.GetComponent<Primitive>().maxStepInMeters = moveStepSize;
    }

    private void movePrimitives() {
        //Move the primitive to target position
        bead.transform.position = targetPos;// new Vector3(targetPos.x, targetPos.y, targetPos.z);
        EnableAxesCorretion(bead);
        Debug.Log("POSITIONING: Primitives moving in " + Time.realtimeSinceStartup);
        clickToInitialize = false;
    }

    private void DisableAxesCorrection( GameObject particle)
    {
        ;//REMOVE
    }

    private void EnableAxesCorretion(GameObject particle)
    {
        ;//REMOVE
    }
}
