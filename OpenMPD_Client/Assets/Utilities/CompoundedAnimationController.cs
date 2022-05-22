using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CompoundedAnimationController : MonoBehaviour
{
    [Header("Objects")]
    public GameObject objA;
    public GameObject objB;
    public GameObject objC;

    [Header("Control")]
    public bool startSequence = false;
    
    public int animationState = 0;
    public int iterA = 0;
    public int iterB = 0;
    public int maxIter = 2;
    public float angle_a = 0;
    public float angle_b = 0;
    public int maxAngle = 20;
    public int rotDirA = 0;
    public int rotDirB = 0;
    public int samplingRate = 2000;

    // private
    Vector3 targetA0 = new Vector3(0, 0.14f, 0);
    Vector3 targetB0 = new Vector3(0, 0.10f, 0);
    Vector3 targetA1 = new Vector3(0, 0.12f, 0);
    Vector3 targetB1 = new Vector3(0, 0.12f, 0);
    
    Vector3 curposA = new Vector3();
    Vector3 curposB = new Vector3();
    Vector3 dirA = new Vector3(0, 0, 0);
    Vector3 dirB = new Vector3(0, 0, 0);
    float step = 0.0001f;
    float dThres = 0.0001f;
    float distA = 0;
    float distB = 0;
    bool goNext = true;
    
    // Start is called before the first frame update
    void Start()
    {
    }

    
    // Update is called once per frame
    void Update()
    {
        if (startSequence && OpenMPD_PresentationManager.Instance())  {
            if (animationState == 0)
                InitializeAnimation();
            else if (animationState == 1)
                Animate();
        }
        else {
            startSequence = false;
            Debug.Log("The presentation manager is not instanced yet");
        }
    }

    void InitializeAnimation()  {
        // set initial values for this animation
        step = maxAngle / (float)samplingRate;
        //check if iter is les than the limit
        if (iterA < maxIter)   {
            GenerateRotationXpos(ref objA, ref angle_a, ref rotDirA, ref iterA, step, maxAngle);
            GenerateRotationXneg(ref objB, ref angle_b, ref rotDirB, ref iterB, step, maxAngle);
        }
        else {
            animationState = 1;
            iterA = 0;
            iterB = 0;
            goNext = true;
        }
    }

    void Animate()
    {
        Vector3 targetA = new Vector3();
        Vector3 targetB = new Vector3();
        if (iterA == 0)
        {
            targetA = targetA0;
            targetB = targetB0;
        }
        else if (iterA == 1)
        {
            targetA = targetA1;
            targetB = targetB1;
        }

        if (goNext)
        {
            UpdateParticlePos(objA, ref dirA, targetA, ref step, samplingRate);
            UpdateParticlePos(objB, ref dirB, targetB, ref step, samplingRate);
            goNext = false;
        }

        curposA = objA.transform.position;
        curposB = objB.transform.position;
        distA = (targetA - curposA).magnitude;
        distB = (targetB - curposB).magnitude;

        if (distA > dThres)
            objA.transform.position += step * dirA;
        else if (iterA < maxIter)
        {
            iterA++;
            goNext = true;
        }
        else
        {
            animationState = 0;
            iterA = 0;
            iterB = 0;
        }

        if (distB > dThres)
            objB.transform.position += step * dirB;
        else if (iterB < maxIter)
        {
            iterB++;
            goNext = true;
        }
        else
        {
            animationState = 0;
            iterA = 0;
            iterB = 0;
        }
    }

    void GenerateRotationXpos(ref GameObject obj, ref float angle, ref int rotDir,ref int iter, float step, int maxAngle) {
        if (rotDir == 0) {
            //check if angle is less than threshold
            if (angle < maxAngle) {
                angle += step;
                obj.transform.Rotate(step, 0, 0);
            }
            else
                rotDir = 1;
        }
        else if (rotDir == 1) {
            //check if angle is less than threshold
            if (angle > -maxAngle) {
                angle -= step;
                obj.transform.Rotate(-step, 0, 0);
            }
            else
                rotDir = 2;
        }
        else if (rotDir == 2) {
            //check if angle is less than threshold
            if (angle < 0 - step)  {
                angle += step;
                obj.transform.Rotate(step, 0, 0);
            }
            else  {
                rotDir = 0;
                iter++;
            }
        }
    }

    void GenerateRotationXneg(ref GameObject obj, ref float angle, ref int rotDir, ref int iter, float step, int maxAngle) {
        if (rotDir == 0) {
            //check if angle is less than threshold
            if (angle > -maxAngle) {
                angle -= step;
                obj.transform.Rotate(-step, 0, 0);
            }
            else
                rotDir = 1;
        }
        else if (rotDir == 1) {
            //check if angle is less than threshold
            if (angle < maxAngle)  {
                angle += step;
                obj.transform.Rotate(step, 0, 0);
            }
            else
                rotDir = 2;
        }
        else if (rotDir == 2) {
            //check if angle is less than threshold
            if (angle > 0 + step)  {
                angle -= step;
                obj.transform.Rotate(-step, 0, 0);
            }
            else  {
                rotDir = 0;
                iter++;
            }
        }
    }

    void UpdateParticlePos(GameObject obj, ref Vector3 directionlocal, Vector3 target, ref float steplocal, int samples )  {
        Vector3 startlocal = new Vector3();
        Vector3 endlocal = new Vector3();

        startlocal = obj.transform.position;
        endlocal = target;
        //endlocal.y += 0.12f;
        directionlocal = (endlocal - startlocal).normalized;
        float dist = (endlocal - startlocal).magnitude;
        steplocal = dist / (float)samples;
    }

}
