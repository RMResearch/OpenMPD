using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AnimationSequence : MonoBehaviour
{
    public List<GameObject> sequence;
    public bool startSequence;    
    public int delay = 120;

    int index = 1, counter=0;
    bool resetSequence;

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        if (resetSequence) { 
            index = 1;
            resetSequence = false;
        }

        if (startSequence && counter > delay)
            defineSequence();
        else if(startSequence)
            counter++;
    }

    void defineSequence()
    {
        if (index < sequence.Count) {
            sequence[index].SetActive(true);
            index++;
        }
        else {
            startSequence = false;
            resetSequence = true;
        }
            
        counter = 0;
    }

    //IEnumerator defineSequence()
    //{
    //    if (index < sequence.Count)  {
    //        sequence[index].SetActive(true);
    //        index++;
    //        yield return new WaitForSecondsRealtime(1);
    //    }
    //    else
    //        startSequene = false;       
    //}
}
