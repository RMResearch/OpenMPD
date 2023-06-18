using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UpdateParticleAnimation : MonoBehaviour
{
    [Header("Configuration")]
    public int animStep = 100;
    int stepThres = 100;
   
    [Header("Particles List")]
    public List<GameObject> particles;
    private int elemNumPrim;
    private int prevAnimStep = 100;

    // Start is called before the first frame update
    void Start()
    {
         InitializeParticles();
    }

    // Update is called once per frame
    void Update()
    {
         UpdateVirtualPrimitives();
    }

    void UpdateVirtualPrimitives()
    {
        if (elemNumPrim == 0)
        {
            Debug.Log("The number of primitives does not correspond to the defined descriptors in UpdateFixPosDescriptors.cs");
            return;
        }

        for (int i = 0; i < particles.Count; i++)
        {
            PrimitiveGameObject primID = particles[i].GetComponent<PrimitiveGameObject>();
            if (primID.GetPrimitiveID() == 0)
                return;

            PrimitiveAnimation anim = particles[i].transform.GetChild(0).GetComponent<PrimitiveAnimation>();
            uint startingIndex = OpenMPD_ContextManager .Instance().GetPBD_Position_Staring_Index(primID.GetPrimitiveID());

            uint posID = OpenMPD_ContextManager .Instance().GetPositionsDescriptorID(primID.GetPrimitiveID());
            if (anim.posID != posID || startingIndex != anim.startingIndex || animStep != prevAnimStep)
            {
                anim.posID = posID;
                // particleAnimation crpts handles the difference between coordinate systems (from OpenGL to Unity)
                anim.positions = (OpenMPD_ContextManager .Instance().GetPositionsDescriptor(primID.GetPrimitiveID())).positions;
                anim.startingIndex = startingIndex;
                anim.isNewUpdate = true;
                anim.increment = animStep;
                anim.threshod = stepThres;
                
            }
        }
        prevAnimStep = animStep;
    }

    void InitializeParticles()
    {
        particles.Clear();
        PrimitiveGameObject[] children = GetComponentsInChildren<PrimitiveGameObject>();

        // iterate over to enable the  "PrimitiveAnimation" script on it
        foreach (PrimitiveGameObject child in children)
        {
            // this script it's disabled by default, then we enable it to visualize the particle path
            child.gameObject.transform.GetChild(0).GetComponent<PrimitiveAnimation>().enabled = true;
            particles.Add(child.gameObject);
        }
        // update the number of primitives/particles
        elemNumPrim = particles.Count;
    }
}
