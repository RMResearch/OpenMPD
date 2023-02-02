using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PosDescriptor_xt : PositionDescriptorAsset
{
    [Header("Path parameters:")]
    public GameObject circleObj;
    public int multiplier = 4;
    public int len = 0;

    [Header("Update descriptor")]
    public bool updateDescriptor=false;

    CircleDescriptor path;

    // Use this for initialization
    public void Start() //IEnumerator Start()
    {
        GeneratePosArray();
    }
   
    private void Update()
    {
        if (updateDescriptor || len <= 0)
        {
            GeneratePosArray();
            updateDescriptor = false;
        }
    }

    private void GeneratePosArray()
    {
        if (path == null)
            path = circleObj.GetComponent<CircleDescriptor>();

        if (path == null)
            return;
        // as the input of this module works on edditor mode
        // when the system is started this code will optimize any path drown with the mouse
        if (circleObj.transform.gameObject.activeSelf)
        {
            len = path.positions.Length;
            float[] slowPath = new float[len];
            slowPath = path.positions;

            slowDown(ref slowPath, len);
            len = slowPath.Length;

            if (len > 0)
                len = (int)len / 4;
            //<<<<<<<<<<<<<<<<< Create descriptor
            descriptor = new Positions_Descriptor(slowPath);
        }
        else
        {
            gameObject.SetActive(false);
        }
    }

    void slowDown(ref float[] pos, int len)
    {
        float[] newPos = new float[len * multiplier];
        int counter = 0;
        for (int j = 0; j < len; j += 4)
        {
            // storing same value 2 time
            for (int i = 0; i < multiplier; i++)
            {
                newPos[counter++] = pos[j];
                newPos[counter++] = pos[j + 1];
                newPos[counter++] = pos[j + 2];
                newPos[counter++] = pos[j + 3];
            }
        }
        //pos = new float[len * speedIndexDec];
        pos = newPos;
    }
}
