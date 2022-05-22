using System.Collections;
using System.Threading;
using UnityEngine;

public class ForceFramerate : MonoBehaviour
{
    //public int framerate = 200;
    public int targetUPS=350;
    public int vsync = 0; 

    void Start()
    {
        QualitySettings.vSyncCount = vsync;
        Application.targetFrameRate = targetUPS;
    }


    private void Update()
    {
        QualitySettings.vSyncCount = vsync;
        Application.targetFrameRate = targetUPS;
    }
}
