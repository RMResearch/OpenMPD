using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using Assets.Helper;

public class MultiBeadPositioning : MonoBehaviour
{
    //Configuration variables: Visible in the editor
    public BeadDetector_Controller detector;
    public bool clickToInitialize = false;
    public bool finishedPlacingParticles = false;
    public float trappingAmplitude = 10000;
    public float transducersOFF_Time = 1;
    public float holdTime = 0.5f;
    public float liftTime = 1.0f;
    public float moveTimePerPrimitive = 2.0f;
    public float postLiftWait = 0.2f;
    public float liftStepSize = 0.0001f;
    public float liftHeight = 0.01f;
    //Internal variables:
    PrimitiveMatch[] ourMatch;
    bool phaseOnlyStatus;                   //Detector needs phaseOnly = false to work best. We save the prev status and restore afterwards.
    int step = 0;
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
    Primitive[] primitives;
    int curPrimitiveMoving;
    Amplitudes_Descriptor zeroAmplitude = null, increasingAmplitude = null, trappedAmplitude = null;
    bool descriptorsInitialized() {
        return zeroAmplitude != null && increasingAmplitude != null && trappedAmplitude != null;
    }

    void initializeDescriptors() {
        if (zeroAmplitude == null) {
            float[] a = new float[1]; a[0] = 0;
            zeroAmplitude = new Amplitudes_Descriptor(a);
        }
        if (increasingAmplitude == null) {
            int numSamples = (int)(OpenMPD_PresentationManager.Instance().ResultingFPS * this.holdTime);
            float[] amplitudes = new float[numSamples];
            for (int s = 0; s < numSamples; s++)
                amplitudes[s] = (s * this.trappingAmplitude) / numSamples;
            increasingAmplitude = new Amplitudes_Descriptor(amplitudes);
        }
        if(trappedAmplitude == null)
        {
            float[] a = new float[1]; a[0] = trappingAmplitude;
            trappedAmplitude = new Amplitudes_Descriptor(a);
        }
    }
    void Start()
    {
        clickToInitialize = false;       
    }

    void Update()
    {
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
        //0. Create amplitudes descriptors (to disable, capture and hold)
        if (!descriptorsInitialized())
            initializeDescriptors();
        //1. Check active Primitives in the scene and save their state.
        primitives = FindObjectsOfType<Primitive>();//
        ourMatch = PrimitiveMatch.saveState(primitives);
        //2. Set the zeroAmplitude descriptor to them (board will turn transducers off, makikng them easier to detect and capture). 
        trapPrimitivesTime = Time.realtimeSinceStartup + transducersOFF_Time;  //Give some time to have transducers off and stop particles from dancing around...   
        foreach (Primitive p in primitives)
            //p.gameObject.SetActive(false);       
            p.SetAmplitudesDescriptor(zeroAmplitude.amplitudesDescriptorID);
        //2. Make sure variable amplitudes are being used (phaseOnly = false). Save previous state to restore it afterwards.
        phaseOnlyStatus = OpenMPD_PresentationManager.Instance().phaseOnly;
        OpenMPD_PresentationManager.Instance().phaseOnly = false;
        OpenMPD_PresentationManager.Instance().RequestCommit();
        //3. Indicate the user can place the particles. 
        Debug.Log("POSITIONING: Primitives disabled in "+Time.realtimeSinceStartup + "Place particles in the platform.");
        finishedPlacingParticles = false;
        step = TELEPORT_PRIMITIVES;       
    }

    private void teleportPrimitives()
    {
        //0. Wait until we need to trap the particles
        if (Time.realtimeSinceStartup < trapPrimitivesTime)
        {
            detector.detectBeads();//Detect, but ignore. We keep refreshing the camera feed.
            foreach (Primitive p in primitives)
                if (p.gameObject.active)
                    p.gameObject.SetActive(false);
                return;
        }
        if (!finishedPlacingParticles)
            return;
        //1. Check if we have enough particles in sight and move traps there
        int numBeads = detector.detectBeads();
        float[] beadPositions = detector.getCurrentBeadPositions();
        if (numBeads < primitives.Length)
            return;
        //Adapt format (BeadDetector DLL returns a raw array of floats. We need Vector3).
        Vector3[] positions = new Vector3[numBeads];
        for (int p = 0; p < numBeads; p++)
            positions[p] = new Vector3(beadPositions[3*p+0], beadPositions[3 * p + 1], beadPositions[3 * p + 2]);
        //2. Decide how we are going to use them (which bead will match which primitive)
        if(!PrimitiveMatch.matchState(positions, ourMatch))
            return;
        //3. Reset timers for next stages (how long for lifting, etc)
        stabilizePrimitivesTime = Time.realtimeSinceStartup + holdTime;    //Give some time for particle to stabilize, once trapped
        liftPrimitivesTime = stabilizePrimitivesTime + liftTime;    //Give some time to lift the particle
        //3. Move traps to particles locations (they are still disabled).
        for (int p = 0; p < primitives.Length; p++)
        {
            Matrix4x4 targetPos = primitives[p].transform.localToWorldMatrix;
            targetPos.SetColumn(3, new Vector4(ourMatch[p].initialPosition.x, ourMatch[p].initialPosition.y, ourMatch[p].initialPosition.z, 1));
            primitives[p].GetComponent<Primitive>().TeleportPrimitive(targetPos);
        }
        Debug.Log("POSITIONING: Primitives teleported in "+Time.realtimeSinceStartup);
        step = ENABLE_PRIMITIVES;
    }

    private void enablePrimitives()
    {
        //1. Re-enable the primitives to trap the particles. 
        for (int p = 0; p < primitives.Length; p++)
        {
            //primitives[p].gameObject.SetActive(true);
            primitives[p].maxStepInMeters = liftStepSize;
            // We enqueue 2 descriptors: First to trap the particle slowly and then to retain that high amplitude. 
            primitives[p].SetAmplitudesDescriptor(this.increasingAmplitude.amplitudesDescriptorID);
            primitives[p].SetAmplitudesDescriptor(this.trappedAmplitude.amplitudesDescriptorID);
            primitives[p].gameObject.SetActive(true);
        }
        //2. Once all descriptors have been set, we enable them all at once.
        OpenMPD_PresentationManager.Instance().RequestCommit();
        Debug.Log("POSITIONING: Primitives re-enabled in "+Time.realtimeSinceStartup);
        step = STABILIZE_PRIMITIVES;
    }

    private void stabilizePrimitives()  
    {        
        //Wait until stabilize deadline time
        if (Time.realtimeSinceStartup < stabilizePrimitivesTime)
            return;
        //Transition to next state:
        for (int p = 0; p < primitives.Length; p++)
            primitives[p].transform.position = primitives[p].transform.position + new Vector3(0,liftHeight,0);
        Debug.Log("POSITIONING: Begin lifting Primitives in "+Time.realtimeSinceStartup);
        step = LIFT_PRIMITIVES;
    }

    private void liftPrimitives() {
        //Wait until lift deadline time
        float curTime = Time.realtimeSinceStartup;
        if (curTime < liftPrimitivesTime)
            return;
        // Restore its initial movement speed and set when we will start moving each primitive:
        for (int p = 0; p < primitives.Length; p++)
        {
            primitives[p].maxStepInMeters = ourMatch[p].prev_moveStepSize;
            ourMatch[p].startMoveTime = curTime + p * moveTimePerPrimitive;
        }
        //Transition to next state:
        step = MOVE_PRIMITIVES;
        curPrimitiveMoving = 0;
        Debug.Log("POSITIONING: Primitives stabilizign after lifting in " + Time.realtimeSinceStartup);

    }

    private void movePrimitives() {
        //0. Check if we are finished.
        if (curPrimitiveMoving == primitives.Length)
        {
            //... finish restoring previous contextg and go!
            for (int p = 0; p < primitives.Length; p++)
                primitives[p].SetAmplitudesDescriptor(ourMatch[p].prev_amplitudeDescriptor);
            OpenMPD_PresentationManager.Instance().phaseOnly = phaseOnlyStatus; //Prev state of phase only (before initialization).
            OpenMPD_PresentationManager.Instance().RequestCommit();
            clickToInitialize = false;
            return;
        }
        //1. Wait until we have to move current primitive
        if (Time.realtimeSinceStartup < ourMatch[curPrimitiveMoving].startMoveTime)
            return;
        //Move the primitive to target position
        primitives[curPrimitiveMoving].transform.position = ourMatch[curPrimitiveMoving].prev_position;
        curPrimitiveMoving++;
        Debug.Log("POSITIONING: Primitive "+curPrimitiveMoving+"started moving in " + Time.realtimeSinceStartup);        
    }
}
