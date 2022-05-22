using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PrimitiveAnimation : MonoBehaviour
{
    //4 8 12 16 20 40 80 160 
    [HideInInspector]
    public int threshod = 100;
    [HideInInspector]
    public int increment = 100;
    [HideInInspector]
    public float[] positions;
    [HideInInspector]
    public float[] positionsLocal;    
    [HideInInspector]
    public uint startingIndex;
    [HideInInspector]
    public bool isNewUpdate = false;
    [HideInInspector]
    public bool useFixUpdate = true;
    [HideInInspector]
    public uint posID = 0;
    
    private bool itWasIni = true;
    private int index=0;
    private int initIndex = 0;
   
    private void FixedUpdate()
    {
        if(useFixUpdate)
            UpdateParticle();
    }

    // Update is called once per frame
    void Update()
    {
        if(!useFixUpdate)
            UpdateParticle();
    }

    void UpdateParticle()
    {
        if (positions.Length == 0 || (startingIndex*4) > positions.Length)
            return;
        initIndex = (int)startingIndex * 4;

        if (itWasIni && isNewUpdate)
        {
            index = initIndex;
            positionsLocal = positions;
            isNewUpdate = false;
            itWasIni = false;
        }

        // checking the new updates and the index of the path
        if (isNewUpdate && (index >= initIndex + threshod || index <= initIndex - threshod))
        {
            index = initIndex;
            positionsLocal = positions;
            isNewUpdate = false;
        }

        // generating the new position inverting z to convert from right to left hand cordinate system        
        // update position
        Vector3 newPos = new Vector3(positionsLocal[index], positionsLocal[index + 1] * -1, /*-1 * */ positionsLocal[index + 2]);
        this.transform.localPosition = newPos;

        // check the index of the next update
        if ((index + (increment * 4)) < positionsLocal.Length)
            index += increment * 4;
        else if (positionsLocal.Length > 4 || (index + (increment * 4)) > positionsLocal.Length)
            index = 0;

        if (positionsLocal.Length == 4)
            itWasIni = true;
    }
}
