using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using MyBox;
using System.Diagnostics;

public class GimbalAnimator : MonoBehaviour
{
    public GimbalController gimbal;
    public bool playAnimation = false;
    public bool autoLoop = true;
    [HideInInspector]
    public bool shouldStop = false;
    public GameObject idleState;
    public int currentAnimationIndex = 0;
    public int nextAnimationIndex = 0;
    [ReadOnly]
    public double secondSinceAnimationStart = 0;
    private Stopwatch timer;
    public Primitive primitive; // Used for scaling
    public List<Animation> animations;
    [System.Serializable]
    public class Animation
    {
        [HideInInspector]
        public Stopwatch animationTimer;

        [Separator("State (debug)")]
        public bool rotationEntered = false;
        public bool rotationStarted = false;
        public bool rotationFinished = false;
        public bool rotationExited = false;
        public bool animationEntered = false;
        public bool animationStarted = false;
        public bool animationFinished = false;
        public bool animationExited = false;

        [Separator("Gimbal Movement")]
        public float rotationDelayS = 0;
        [HideInInspector]
        public float rotationEnterDurationS = 0.01f;
        public float rotationDurationS = 1;
        [HideInInspector]
        public float rotationExitDurationS = 0.01f;
        public string gimbalStartCommand = "";
        public float rotationDegreesInner = 0;
        public float rotationDegreesOuter = 0;
        public string gimbalEndCommand = "";

        [Separator("Content Animation")]
        public float animationDelayS = 0;
        public float animationEnterDurationS = 0;
        public float animationDurationS = 1;
        public float animationExitDurationS = 0;
        public GameObject AnimationEnter;
        public GameObject AnimationContent;
        public GameObject AnimationExit;

        [Separator("Primitive Adjustment")]
        // Translation not possible as start positions would not align?
        public Vector3 PrimitiveRotation;
        public Vector3 PrimitiveScale = Vector3.one;
    }
    // Start is called before the first frame update
    void Start()
    {
        if(gimbal == null)
        {
            gimbal = GameObject.FindObjectOfType<GimbalController>();
        }
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetKeyDown(KeyCode.Alpha0))
        {
            nextAnimationIndex = 0;
        }
        if (Input.GetKeyDown(KeyCode.Alpha1))
        {
            nextAnimationIndex = 1;
        }
        if (Input.GetKeyDown(KeyCode.Alpha2))
        {
            nextAnimationIndex = 2;
        }
        if (Input.GetKeyDown(KeyCode.Alpha3))
        {
            nextAnimationIndex = 3;
        }
        if (Input.GetKeyDown(KeyCode.Alpha4))
        {
            nextAnimationIndex = 4;
        }

        if (Input.GetKeyDown(KeyCode.R))
        {
            playAnimation = false;
            nextAnimationIndex = 0;
        }
        if (Input.GetKeyDown(KeyCode.S))
        {
            shouldStop = true;
        }

        if (Input.GetKeyDown(KeyCode.P)){
            if (!playAnimation)
            {
                currentAnimationIndex = nextAnimationIndex;
                Animation currentAnimation = animations[currentAnimationIndex];
                currentAnimation.animationTimer = Stopwatch.StartNew();
                currentAnimation.rotationEntered = false;
                currentAnimation.rotationStarted = false;
                currentAnimation.rotationFinished = false;
                currentAnimation.rotationExited = false;
                currentAnimation.animationEntered = false;
                currentAnimation.animationStarted = false;
                currentAnimation.animationFinished = false;
                currentAnimation.animationExited = false;
                primitive.transform.localScale = currentAnimation.PrimitiveScale;
                primitive.transform.localRotation = Quaternion.Euler(currentAnimation.PrimitiveRotation);
                nextAnimationIndex = (currentAnimationIndex + 1) % animations.Count;
            }
            else
            {
                if (!animations[currentAnimationIndex].rotationEntered && !animations[currentAnimationIndex].animationEntered)
                {
                    nextAnimationIndex = currentAnimationIndex;
                }
            }
            playAnimation = !playAnimation;
        }
        if (Input.GetKeyDown(KeyCode.H) && !playAnimation)
        {
            gimbal.RotateTo(0,0);
        }
        if (Input.GetKeyDown(KeyCode.Z) && !playAnimation)
        {
            gimbal.SetAsHome();
        }
        if (Input.GetKeyDown(KeyCode.Space))
        {
            // Stop the gimbal.
            playAnimation = false;
        }

        float innerMultiplier = 0;
        float outerMultiplier = 0;
        if (Input.GetKey(KeyCode.LeftShift) || Input.GetKey(KeyCode.RightShift))
        {
            innerMultiplier = 1;
        }
        if (Input.GetKey(KeyCode.LeftControl) || Input.GetKey(KeyCode.RightControl))
        {
            outerMultiplier = 1;
        }
        if (Input.GetKey(KeyCode.KeypadPlus) && !playAnimation)
        {
            int degrees = 1;
            gimbal.RotateBy((int)innerMultiplier * degrees, (int)outerMultiplier * degrees);
        }
        if (Input.GetKey(KeyCode.KeypadMinus) && !playAnimation)
        {
            int degrees = -1;
            gimbal.RotateBy((int)innerMultiplier * degrees, (int)outerMultiplier * degrees);
        }
        if (Input.GetKey(KeyCode.KeypadMultiply) && !playAnimation)
        {
            int degrees = 360;
            gimbal.RotateBy((int)innerMultiplier * degrees, (int)outerMultiplier * degrees);
        }
        if (Input.GetKey(KeyCode.KeypadDivide) && !playAnimation)
        {
            int degrees = 180;
            gimbal.RotateBy((int)innerMultiplier * degrees, (int)outerMultiplier * degrees);
        }




        if (currentAnimationIndex < animations.Count)
        {
            Animation currentAnimation = animations[currentAnimationIndex];
            if (playAnimation || (currentAnimation.rotationEntered && !currentAnimation.rotationExited) || (currentAnimation.animationEntered && !currentAnimation.animationExited))
            {
                double animationTimeS = currentAnimation.animationTimer.Elapsed.TotalSeconds;
                secondSinceAnimationStart = animationTimeS;
                // Rotation Controller
                // Begin Entry state
                if (!currentAnimation.rotationEntered && animationTimeS >= currentAnimation.rotationDelayS)
                {
                    if (currentAnimation.gimbalStartCommand.Length >= 1) gimbal.SendStringToGimbal(currentAnimation.gimbalStartCommand);
                    currentAnimation.rotationEntered = true;
                }
                // End entry state
                // Begin rotation state
                if (currentAnimation.rotationEntered && !currentAnimation.rotationStarted && animationTimeS >= currentAnimation.rotationDelayS + currentAnimation.rotationEnterDurationS)
                {
                    if (currentAnimation.rotationDegreesInner > 0 || currentAnimation.rotationDegreesOuter > 0)
                    {
                        gimbal.RotateBy(currentAnimation.rotationDegreesInner, currentAnimation.rotationDegreesOuter);
                    }
                    currentAnimation.rotationStarted = true;
                }
                // End rotation state
                // Begin exit state
                if (currentAnimation.rotationEntered && currentAnimation.rotationStarted && !currentAnimation.rotationFinished && animationTimeS >= currentAnimation.rotationDelayS + currentAnimation.rotationEnterDurationS + currentAnimation.rotationDurationS)
                {
                    currentAnimation.rotationFinished = true;
                    if (currentAnimation.gimbalEndCommand.Length >= 1) gimbal.SendStringToGimbal(currentAnimation.gimbalEndCommand);
                }
                // End exit state
                if (currentAnimation.rotationEntered && currentAnimation.rotationStarted && currentAnimation.rotationFinished && !currentAnimation.rotationExited && animationTimeS >= currentAnimation.rotationDelayS + currentAnimation.rotationEnterDurationS + currentAnimation.rotationDurationS + currentAnimation.rotationExitDurationS)
                {
                    currentAnimation.rotationExited = true;
                }

                // Animation Controller
                // Begin Entry state
                if (!currentAnimation.animationEntered && animationTimeS >= currentAnimation.animationDelayS)
                {
                    if(currentAnimation.AnimationEnter) currentAnimation.AnimationEnter.SetActive(true);
                    currentAnimation.animationEntered = true;
                }
                // End Entry State
                // Begin Animation Content
                if (currentAnimation.animationEntered && !currentAnimation.animationStarted && animationTimeS >= currentAnimation.animationDelayS + currentAnimation.animationEnterDurationS)
                {
                    if (currentAnimation.AnimationContent) currentAnimation.AnimationContent.SetActive(true);
                    currentAnimation.animationStarted = true;
                }
                // End Animation Content
                // Begin Exit State
                if (currentAnimation.animationEntered && currentAnimation.animationStarted && !currentAnimation.animationFinished && animationTimeS >= currentAnimation.animationDelayS + currentAnimation.animationEnterDurationS + currentAnimation.animationDurationS)
                {
                    if (currentAnimation.AnimationExit) currentAnimation.AnimationExit.SetActive(true);
                    currentAnimation.animationFinished = true;
                }
                // End Exit State
                // Return To Idle
                if (currentAnimation.animationEntered && currentAnimation.animationStarted && currentAnimation.animationFinished && !currentAnimation.animationExited && animationTimeS >= currentAnimation.animationDelayS + currentAnimation.animationEnterDurationS + currentAnimation.animationDurationS + currentAnimation.animationExitDurationS)
                {
                    if (idleState) idleState.SetActive(true);
                    currentAnimation.animationExited = true;
                }

                if (currentAnimation.animationExited && currentAnimation.rotationExited && playAnimation)
                {
                    currentAnimationIndex = nextAnimationIndex;
                    currentAnimation = animations[currentAnimationIndex];
                    currentAnimation.animationTimer = Stopwatch.StartNew();
                    currentAnimation.rotationEntered = false;
                    currentAnimation.rotationStarted = false;
                    currentAnimation.rotationFinished = false;
                    currentAnimation.rotationExited = false;
                    currentAnimation.animationEntered = false;
                    currentAnimation.animationStarted = false;
                    currentAnimation.animationFinished = false;
                    currentAnimation.animationExited = false;
                    primitive.transform.localScale = currentAnimation.PrimitiveScale;
                    primitive.transform.localRotation = Quaternion.Euler(currentAnimation.PrimitiveRotation);
                    if (autoLoop)
                    {
                        nextAnimationIndex = (nextAnimationIndex + 1) % animations.Count;
                    }
                    else
                    {
                        nextAnimationIndex++;
                    }
                }
            }
        }
        else
        {
            idleState.SetActive(true);
            playAnimation = false;
            currentAnimationIndex = 0;
        }
    }

    [ButtonMethod]
    private void StartAnimation()
    {
        currentAnimationIndex = nextAnimationIndex;
        Animation currentAnimation = animations[currentAnimationIndex];
        currentAnimation.animationTimer = Stopwatch.StartNew();
        currentAnimation.rotationEntered = false;
        currentAnimation.rotationStarted = false;
        currentAnimation.rotationFinished = false;
        currentAnimation.rotationExited = false;
        currentAnimation.animationEntered = false;
        currentAnimation.animationStarted = false;
        currentAnimation.animationFinished = false;
        currentAnimation.animationExited = false;
        primitive.transform.localScale = currentAnimation.PrimitiveScale;
        primitive.transform.localRotation = Quaternion.Euler(currentAnimation.PrimitiveRotation);
        nextAnimationIndex = (currentAnimationIndex + 1) % animations.Count;
        playAnimation = true;
    }
}
