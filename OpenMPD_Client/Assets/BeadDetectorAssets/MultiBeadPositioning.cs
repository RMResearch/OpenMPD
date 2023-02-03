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
    public float transducersOFF_Time = 1;
    public float holdTime = 0.5f;
    public float liftTime = 1.0f;
    public float moveTimePerPrimitive = 2.0f;
    public float postLiftWait = 0.2f;
    public float liftStepSize = 0.0001f;
    public float liftHeight = 0.01f;
    //Internal variables:
    PrimitiveMatch[] ourMatch;
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
                    clickToInitialize = false;
                    //liftPrimitives();
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
        //Check active Primitives in the scene
        primitives = FindObjectsOfType<Primitive>();//
        trapPrimitivesTime = Time.realtimeSinceStartup + transducersOFF_Time;  //Give some time to have transducers off and stop particles from dancing around...   
        //Disable the primitives (board will turn transducers off)
        foreach (Primitive p in primitives)
            p.gameObject.SetActive(false);       
       
        Debug.Log("POSITIONING: Primitives disabled in "+Time.realtimeSinceStartup);
        step = TELEPORT_PRIMITIVES;       
    }

    private void teleportPrimitives()
    {
        //0. Wait until we need to trap the particles
        if (Time.realtimeSinceStartup < trapPrimitivesTime)
        {
            detector.detectBeads();//Detect, but ignore. We keep refreshing the camera feed.
            return;
        }
        //1. Check if we have enough particles in sight and move trap there
        int numBeads = detector.detectBeads();
        float[] beadPositions = detector.getCurrentBeadPositions();
        if (numBeads < primitives.Length)
            return;
        //Adapt format (BeadDetector DLL returns a raw array of floats. We need Vector3).
        Vector3[] positions = new Vector3[numBeads];
        for (int p = 0; p < numBeads; p++)
            positions[p] = new Vector3(beadPositions[3*p+0], beadPositions[3 * p + 1], beadPositions[3 * p + 2]);
        //2. Decide how we are going to use them (which bead will match which primitive)
        ourMatch = PrimitiveMatch.generateMatch(positions, primitives);
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
        //Enable the primitive to enable its trap:
        for (int p = 0; p < primitives.Length; p++)
        {
            primitives[p].gameObject.SetActive(true);
            primitives[p].maxStepInMeters = liftStepSize;
        }
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
        //Wait until stabilize deadline time
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
