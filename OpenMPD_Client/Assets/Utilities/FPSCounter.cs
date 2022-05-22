using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class FPSCounter : MonoBehaviour
{

    public float avgFrameRate;
   

    public void Update()
    {
        float current = 0;
        current = (int)(1f / Time.unscaledDeltaTime);
        avgFrameRate =0.8f*avgFrameRate + 0.2f* current;
      
    }
}
