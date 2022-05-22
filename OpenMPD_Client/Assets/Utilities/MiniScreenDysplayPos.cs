using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor.SceneManagement;

public class MiniScreenDysplayPos : MonoBehaviour
{
    bool updatePosRotData = true;
    List<Vector3> particlePosList = new List<Vector3>();
    List<Vector3> prevParticlePosList = new List<Vector3>();
    Vector3 targetPos = new Vector3();
    Vector3 targetNormal = new Vector3();

    private Quaternion initialRot = new Quaternion();
    // Start is called before the first frame update
    void Start()
    {
        // look for the parent game object & store the child particles in the parent gameobject
        GetPointsPos(ref particlePosList);
        if(particlePosList.Count > 0)
            ComputePosAndRotData();
    }

    // Update is called once per frame
    void Update()
    {
        // look for changes in pos
        if (CheckPosChanges(particlePosList) || updatePosRotData)
        {
            ComputePosAndRotData();            
        }          
    }

    private void OnEnable()
    {
        //initialTransform = transform;
        initialRot = transform.rotation;
    }

    private void OnDisable()
    {
        transform.rotation = initialRot;
    }

    private void ComputePosAndRotData()
    {
        Debug.Log("Compute Update ini");
        // comput the position of the miniscreen based on hte average of the points
        targetPos = ComputePosition(particlePosList);
        // comput the normal of the plane from the current position of the particles
        targetNormal = ComputeNormal(particlePosList);
        // apply pos and rot to the mini-screen
        UpdatePosRotData(targetPos, targetNormal);
        // update previous positions list
        prevParticlePosList = new List<Vector3>(particlePosList);
        Debug.Log("Compute Update end");
        //lastNormal = targetNormal;
    }

    private void GetPointsPos(ref List<Vector3> posList)
    {
        posList.Clear();
        Primitive[] primitiveArray;

        primitiveArray = gameObject.transform.parent.gameObject.GetComponentsInChildren<Primitive>();
        foreach (Primitive prim in primitiveArray)
            posList.Add(prim.gameObject.transform.GetChild(0).gameObject.transform.position);
    }

    private Vector3 ComputePosition(List<Vector3> posList)
    {        
        float x = 0, y = 0, z = 0;
        foreach(Vector3 pos in posList)
        {
            x += pos.x;
            y += pos.y;
            z += pos.z;
        }
        return new Vector3(x / posList.Count, y / posList.Count, z / posList.Count);
    }

    private Vector3 ComputeNormal(List<Vector3> posList)
    {
        Vector3 v1 = posList[1] - posList[0];
        Vector3 v2 = posList[3] - posList[0];
        return Vector3.Cross(v1, v2).normalized;
    }

    private void UpdatePosRotData(Vector3 pos, Vector3 norm)
    {
        transform.position = pos;
        // Rotate the forward vector towards the target direction by one step
        Vector3 newDirection1 = Vector3.RotateTowards(transform.forward, norm, 1, 0.0f);
        transform.localRotation = Quaternion.LookRotation(newDirection1);
    }

    private bool CheckPosChanges(List<Vector3> posList)
    {
        bool state = false;
        GetPointsPos(ref posList);
        if (posList.Count > 0)
        {
            bool cond1 = false, cond2=false, cond3=false, cond4 = false;
            if ((posList[0] - prevParticlePosList[0]).magnitude >= 0.00001f)
                cond1 = true;
            if ((posList[1] - prevParticlePosList[1]).magnitude >= 0.00001f)
                cond2 = true;
            if ((posList[2] - prevParticlePosList[2]).magnitude >= 0.00001f)
                cond3 = true;
            if ((posList[3] - prevParticlePosList[3]).magnitude >= 0.00001f)
                cond4 = true;

            if (cond1 || cond2 || cond3 || cond4)
            {
                state = true;
                updatePosRotData = false;
            }                
        }
        return state;
    }
}
