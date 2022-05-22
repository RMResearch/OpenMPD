using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DexterousAnimationController : MonoBehaviour
{
    [Header("Iniital Pos")]
    public Vector3 mainAnimationCentre = new Vector3(0, 0, 0);
    public FixedPositionDescriptor iniPos1;
    public FixedPositionDescriptor iniPos2;
    public FixedPositionDescriptor iniPos3;

    [Header("Secondary Paths")]
    public CircleCustomDir circle1;
    public CircleCustomDir circle2;
    public CircleCustomDir circle3;

    bool isConfigured = false;

    // new variables
    Vector3 dir1 = new Vector3();
    Vector3 dir2 = new Vector3();
    Vector3 dir3 = new Vector3();

    Vector3 centre1 = new Vector3();
    Vector3 centre2 = new Vector3();
    Vector3 centre3 = new Vector3();

    List<Vector3> posList1 = new List<Vector3>();
    List<Vector3> posList2 = new List<Vector3>();
    List<Vector3> posList3 = new List<Vector3>();

    float rot1 = 0;
    float rot2 = 0;
    float rot3 = 0;


    // Start is called before the first frame update
    void Start()
    {
        initializePosRot();
    }

    // Update is called once per frame
    void Update()
    {
        if (!isConfigured)
        {
            initializePosRot();
            if (0 <= rot1 && rot1 <= 360 && 0 <= rot2 && rot2 <= 360 && 0 <= rot3 && rot3 <= 360)
                isConfigured = true;
        }
    }

    void initializePosRot()
    {
        getListFromArrays();
        // get the direction vectors
        computeDirVectors();
        // compute the circles centres position
        computeAnimationCentres();
        // compute the rotation per circle path 
        computeAnimationRotations();
    }

    void getListFromArrays()
    {
        getListFromArray(ref posList1, circle1.positions);
        getListFromArray(ref posList2, circle2.positions);
        getListFromArray(ref posList3, circle3.positions);
    }

    void getListFromArray(ref List<Vector3> listIn, float[] arrayIn)
    {
        for(int i=0; i< arrayIn.Length; i=i + 4)
        {
            listIn.Add(new Vector3(arrayIn[i], arrayIn[i + 1], arrayIn[i + 2]));
        }
    }

    void computeDirVectors()
    {
        dir1 = (iniPos1.position - mainAnimationCentre).normalized;
        dir2 = (iniPos2.position - mainAnimationCentre).normalized;
        dir3 = (iniPos3.position - mainAnimationCentre).normalized;
    }

    void computeAnimationCentres()
    {
        centre1 = iniPos1.position + dir1 * circle1.radius_m;
        centre2 = iniPos2.position + dir2 * circle2.radius_m;
        centre3 = iniPos3.position + dir3 * circle3.radius_m;

        UpdateAnimationCentres();
    }

    void UpdateAnimationCentres()
    {
        circle1.center_m = centre1;
        circle2.center_m = centre2;
        circle3.center_m = centre3;
    }

    void computeAnimationRotations()
    {
        // the threshold value is 1mm
        int index1 = GetIndex(posList1, iniPos1.position, 0.0005f);
        int index2 = GetIndex(posList2, iniPos2.position, 0.0005f);
        int index3 = GetIndex(posList3, iniPos3.position, 0.0005f);

        rot1 = (index1 * 360) / (float)posList1.Count;
        rot2 = (index2 * 360) / (float)posList2.Count;
        rot3 = (index3 * 360) / (float)posList3.Count;

        circle1.nornalRot = rot1;
        circle2.nornalRot = rot2;
        circle3.nornalRot = rot3;

        circle1.updateDescriptor = true;
        circle2.updateDescriptor = true;
        circle3.updateDescriptor = true;        
    }

    int GetIndex(List<Vector3> listIn, Vector3 pos, float thres)
    {
        float minDist=1;
        int index = 0;

        for(int i=0; i< listIn.Count; i++)
        {
            float dist = (pos - listIn[i]).magnitude;
            if (dist < minDist)
            {
                minDist = dist;
                index = i;
            }
        }

        if (minDist > thres)
        {
            index = 0;
            Debug.Log("The pos is not in the current list <<---------------");
        }
        return index;
    }

     
}
