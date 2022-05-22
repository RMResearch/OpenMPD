using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Assets.Utilities._0.Testing;

public class LetterHandler : MonoBehaviour
{
    [Header("Configuration")]
    public bool startSequence = false;
    public int iterDelay = 100;// 

    [Header("State Objects")]
    public GameObject idle;
    public GameObject letterA;
    public GameObject letterC;
    public GameObject letterM;
    public GameObject colorHandler;

    [Header("Debbuging")]
    public bool fish = true;
    public bool isActive = false;
    // private
    public List<GameObject> letterList;
    [ShowOnly] public int iteration = 0;
    [ShowOnly] public int letterCount = 0;
    
    // Start is called before the first frame update
    void Start()
    {
        idle.SetActive(true);
        letterA.SetActive(false);
        letterC.SetActive(false);
        letterM.SetActive(false);

        letterList = new List<GameObject>();
        letterList.Add(idle);
        letterList.Add(letterA);
        letterList.Add(letterC);
        letterList.Add(letterM);
    }

    // Update is called once per frame
    void Update()
    {
        if(startSequence && !fish)
        {
            if (iteration >= iterDelay)
            {
                if (letterCount == 0)
                {
                    activateLetter(ref letterCount, "ColourConfigData_A_Samp-1100_step-30");
                    iteration = 0;
                    //letterCount++;
                }
                else if (letterCount == 1)
                {
                    activateLetter(ref letterCount, "ColourConfigData_C_Samp-1100_step-30");
                    iteration = 0;
                    //letterCount++;
                }
                else if (letterCount == 2)
                {
                    activateLetter(ref letterCount, "ColourConfigData_M_Samp-1100_step-30");
                    iteration = 0;
                    //letterCount=0;
                }
                else if (letterCount == 3)
                {
                    letterCount = -1;
                    activateLetter(ref letterCount, "ColourConfigData_M_Samp-1100_step-30");
                    iteration = 0;
                    letterCount = 0;
                }
            }
            else
            {
                iteration++;
            }
        }

        if (fish && !isActive)
        {
            int cont = 1;
            activateLetter(ref cont, "ColourConfigData_blueFish");
            isActive = true;
        }
    }

    void activateLetter(ref int index, string name)
    {
        letterList[++index].SetActive(true);
        colorHandler.GetComponent<ColourManager>().fileName = name;
        colorHandler.GetComponent<ColourManager>().colourMode = ColouringMode.Read;
        //colorHandler.GetComponent<ColourManager>().startRenderingColor = true;
    }
}
