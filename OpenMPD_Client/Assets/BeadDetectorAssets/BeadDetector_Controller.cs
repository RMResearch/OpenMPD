using Levitation;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using MyBox;

public class BeadDetector_Controller : MonoBehaviour
{
    //Public interface (control from Unity editor)
    [Header("Device to use:")]
    public int DeviceID = 0;
    [Header("Coordinates In Image:")]
   
    public Vector3 topLeft = new Vector3(0.086f, 0.033f, 0.086f);
    public Vector3 botLeft = new Vector3(0.086f, 0.033f, 0.086f);
    public Vector3 botRight = new Vector3(0.086f, 0.033f, 0.086f);
    public Vector3 topRight = new Vector3(0.086f, 0.033f, 0.086f);

    [Header("Detection Configuration:")]
     public    int pixelsPerMeter = 5000;
     public    int threshold = 100;
     public    int erodeDilate = 2;
     public    float sphericity = 0.4f;
     public    float minRadiusInMeters = 0.001f;
     public    float maxRadiusInMeters = 0.005f;
     public    bool visualize = false;
    [Header("Detection Status:")]
    public bool printResults = true;
    public float[] detectedPositions;
    [SerializeField] private int numBeadsToDetect = 0;
    [SerializeField] private bool init = false;
    [SerializeField] private bool calibrated = false;
    [SerializeField] private bool detect = false;
      
    //Internal variables:
    protected BeadDetector_DLL_Loader detector;
    long instancePointer = 0;

    public bool isActive() {
        return detector && detector.isActiveAndEnabled;
    }
    public void Start()
    {
        init = false;
        calibrated = false;
        //detecting = false;

        // get the particles on the base
        numBeadsToDetect = getNumParticles();
        // initialize the array to store the particles position
        detectedPositions = new float[numBeadsToDetect*3];
    }
    public void Update()
    {
        if (!detector)
            createDetectorInstance();
        if (!detector.isActiveAndEnabled)
            return;
        if(!isInit())
            initCamera();
        if (!isCalibrated())
            calibrateDetector();
        if (detect)
        {
            int numBeads = detectBeads();
            if (numBeads != 0)
            {
                float[] beadPositions = new float[3 * numBeads];
                beadPositions = getCurrentBeadPositions();

            }
        }
    }

    // Initializes pointer to Detector Object

    public void createDetectorInstance()
    {
        detector = BeadDetector_DLL_Loader.getInstance();
    }

    // Initializes Detector object itself.

    public void initCamera()
    {
        float[] p1 = { topLeft.x, topLeft.y, topLeft.z };
        float[] p2 = { botLeft.x, botLeft.y, botLeft.z };
        float[] p3 = { botRight.x, botRight.y, botRight.z };
        float[] p4 = { topRight.x, topRight.y, topRight.z };
        instancePointer = detector.createInstance(DeviceID, p1, p2, p3, p4, pixelsPerMeter, threshold, erodeDilate, sphericity, minRadiusInMeters, maxRadiusInMeters, visualize);
        if (instancePointer != 0)
        {
            init = true;
        }
    }

    // Opens a window for detector calibration.
    // When the window opens, user selects four corners (in any order) of the stage to act as the detection area.

    public void calibrateDetector()
    {
        detector.calibrateDetector(instancePointer);
        calibrated = true;
    }

    //First, updates the size of the position buffer to accomodate the current number of beads that need to be detected.
    //This calls the image processing functions in the DLL - detects and stores the data within the DLL.

    public int detectBeads()
    {
        if (calibrated)
        {
            numBeadsToDetect = detector.detectBeads(instancePointer);
            detectedPositions = new float[numBeadsToDetect * 3];
        }
        else
        {
            Debug.Log("Detector not calibrated.");
        }
        return numBeadsToDetect;
    }

    //Retreives the data currently stored in the DLL for the positions of the beads.
    public float[] getCurrentBeadPositions()
    {
        detector.getCurrentBeadPositions(instancePointer, detectedPositions);
        if (printResults)
        {
            for (int i = 0; i < detectedPositions.Length; i += 3)
            {
                Debug.Log("[x: " + detectedPositions[i] + "y: " + detectedPositions[i + 1] + "z: " + detectedPositions[i + 2]+"]\n");
            }
        }
        return detectedPositions;
    }

    private int getNumParticles()
    {
        return GameObject.FindGameObjectsWithTag("primitive").Length;
    }

    public bool isInit()
    {
        return init;
    }

    public bool isCalibrated()
    {
        return calibrated;
    }
}
