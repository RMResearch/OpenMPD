using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UpdateImage : MonoBehaviour
{
    public Texture m_MainTexture;
    public bool updateTexture = false;
    // Start is called before the first frame update
    void Start()
    {
        UpdateTexture();               
    }

    // Update is called once per frame
    void Update()
    {
        if (updateTexture)
        {
            UpdateTexture();
            updateTexture = false;
        }
           
    }

    private void UpdateTexture()
    {
        int childCount = transform.childCount;
        List<Renderer> renderers = new List<Renderer>();

        for (int i = 0; i < childCount; i++)
        {
            renderers.Add(transform.GetChild(i).GetComponent<Renderer>());
            Renderer rend = transform.GetChild(i).GetComponent<Renderer>();
            //Set the Texture you assign in the Inspector as the main texture (Or Albedo)
            rend.material.SetTexture("_MainTex", m_MainTexture);
        }
    }
}
