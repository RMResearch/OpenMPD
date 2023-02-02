using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using MyBox;

public class SpeedTestManager : MonoBehaviour
{
    [Header("State Components")]
    public GameObject verticalRun;
    public GameObject verticalBack;
    public GameObject HorizontalRun;
    public GameObject HorizontalBack;

    [Separator("Descriptors Components")]
    public GameObject verticalDesRun;
    public GameObject HorizontalDesRun;

    [Separator("Update")]
    public bool updateChanges = false;
    public int VerticalAcc = 400;
    public int HorizontalAcc = 50;

    // local variables
    //ReadDescriptorFromFile vdr;
    //ReadDescriptorFromFile hdr;
    int prevVertAcc = 0;
    int prevHorAcc = 0;
    LineCustomAcc vdr;
    LineCustomAcc hdr;

    bool allConfigured = false;

    [ButtonMethod]
    private string VerticalSpeedTest()
    {
        string message = "null";
        if (verticalRun.activeSelf)
        {
            verticalBack.SetActive(true);
            message = "Vertical speed test state: Run";
        }
        else
        {
            verticalRun.SetActive(true);
            message = "Vertical speed test state: Way Back";
        }

        return message;
    }
    [ButtonMethod]
    private string IncreaseVerticalAcceleration()
    {
        string message = "null";
        if (allConfigured)
        {
            VerticalAcc += 10;
            vdr.Acc0 = VerticalAcc;
            prevVertAcc = VerticalAcc;
            vdr.isSpeedTest = true;
            message = "Vertical acceleration increased from " + (VerticalAcc-10).ToString() + " to "+ VerticalAcc.ToString();
        }
        else  {
            message = "Some componens are missing, this option is not valid";
        }

        return message;
    }
    [ButtonMethod]
    private string DecreaseVerticalAcceleration()
    {
        string message = "null";
        if (allConfigured)
        {
            VerticalAcc -= 10;

            if (VerticalAcc <= 0)
                VerticalAcc = 10;

            vdr.Acc0 = VerticalAcc;
            prevVertAcc = VerticalAcc;
            vdr.isSpeedTest = true;
            message = "Vertical acceleration decreased from " + (VerticalAcc + 10).ToString() + " to " + VerticalAcc.ToString();
        }
        else
        {
            message = "Some componens are missing, this option is not valid";
        }

        return message;
    }

    [ButtonMethod]
    private string HorizontalSpeedTest()
    {
        string message = "null";
        if(allConfigured)
        {
            if (HorizontalRun.activeSelf) {
                HorizontalBack.SetActive(true);
                message = "Horizontal speed test state: Run";
            }
            else {
                HorizontalRun.SetActive(true);
                message = "Horizontal Speed test state: Way Back";
            }
        }
        else  {
            message = "Some componens are missing, this option is not valid";
        }

        return message;
    }
    [ButtonMethod]
    private string IncreaseHorizontalAcceleration()
    {
        string message = "null";
        if (allConfigured)
        {
            HorizontalAcc += 10;
            hdr.Acc0 = HorizontalAcc;
            prevHorAcc = HorizontalAcc;
            hdr.isSpeedTest = true;
            message = "Vertical acceleration increased from " + (HorizontalAcc - 10).ToString() + " to " + HorizontalAcc.ToString();
        }
        else
        {
            message = "Some componens are missing, this option is not valid";
        }

        return message;
    }
    [ButtonMethod]
    private string DecreaseHorizontalAcceleration()
    {
        string message = "null";
        if (allConfigured)
        {
            HorizontalAcc -= 10;

            if (HorizontalAcc <= 0)
                HorizontalAcc = 10;

            hdr.Acc0 = HorizontalAcc;
            prevHorAcc = HorizontalAcc;
            hdr.isSpeedTest = true;
            message = "Vertical acceleration decreased from " + (HorizontalAcc + 10).ToString() + " to " + HorizontalAcc.ToString();
        }
        else
        {
            message = "Some componens are missing, this option is not valid";
        }

        return message;
    }
    // Start is called before the first frame update
    void Start()
    {
        allConfigured = IsAllConfigured();
    }

    // Update is called once per frame
    void Update()
    {
        if (VerticalAcc != prevVertAcc || HorizontalAcc != prevHorAcc)
        {
            if (VerticalAcc <= 0)
                VerticalAcc = 10;

            vdr.Acc0 = VerticalAcc;
            vdr.isSpeedTest = true;
            prevVertAcc = VerticalAcc;
        }

        if (HorizontalAcc != prevHorAcc)
        {
            if (HorizontalAcc <= 0)
                HorizontalAcc = 10;

            hdr.Acc0 = HorizontalAcc;
            hdr.isSpeedTest = true;
            prevHorAcc = HorizontalAcc;
        }

        if (updateChanges)  {
            allConfigured = IsAllConfigured();
            updateChanges = false;
        }
    }

    private bool IsAllConfigured()
    {
        bool status = true;
        if (verticalDesRun != null)
            vdr = verticalDesRun.GetComponent<LineCustomAcc>(); //verticalDesRun.GetComponent<ReadDescriptorFromFile>();//
        else
            status = false;

        if (HorizontalDesRun != null)
            hdr = HorizontalDesRun.GetComponent<LineCustomAcc>();//HorizontalDesRun.GetComponent<ReadDescriptorFromFile>();
        else
            status = false;

       return status;
    }
}
