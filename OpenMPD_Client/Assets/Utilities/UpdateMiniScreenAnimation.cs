using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UpdateMiniScreenAnimation : MonoBehaviour
{
    public GameObject miniScreen;
    public float rotAngle = 0.5f;
    public int speed = 10;
    public bool startRotation = false;
    [ShowOnly] public string parentName="";

    private Quaternion initialRot;
    // Start is called before the first frame update
    void Start()
    {
        InitializeMiniScreen(false);
    }

    private void Update()
    {
        if (startRotation)
        {
            //applying rotation
            miniScreen.transform.Rotate(0, speed * Time.deltaTime, 0); //rotates 50 degrees per second around z axis
        }
    }
    private void OnDisable()
    {
        miniScreen.transform.rotation = initialRot;
        InitializeMiniScreen(true);
    }
    private void OnEnable()
    {
        initialRot = miniScreen.transform.rotation;
        InitializeMiniScreen(false);
    }

    void InitializeMiniScreen(bool state)
    {
        parentName = miniScreen.transform.parent.name;
        miniScreen.transform.parent.GetComponent<UpdateParticleAnimation>().enabled = state;
        miniScreen.GetComponentInChildren<MiniScreenDysplayPos>().enabled = state;
        PrimitiveGameObject[] list = miniScreen.GetComponentsInChildren<PrimitiveGameObject>();
        foreach (PrimitiveGameObject prim in list)
            prim.transform.GetChild(0).GetComponent<PrimitiveAnimation>().enabled = state;
    }
}


