using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;

public class StateSwitch : MonoBehaviour
{
    bool doSwitch = false;
     // Use this for initialization
    void Start()
    {

    }

    // Update is called once per frame
    void Update()
    {
        if (doSwitch) {
            //0.Check if we need to run
            OpenMPD_PresentationManager rm = OpenMPD_PresentationManager.Instance();
            if (rm == null || !rm.IsRunning()) return;
            //1. Deactivate all children of parent node (other than ourselves).
            int numChild = transform.parent.childCount;
            for (int c = 0; c < numChild; c++)
            {
                Transform child = transform.parent.GetChild(c);
                if (child != this.transform)
                {
                    child.gameObject.SetActive(false);
                    PrimitiveStateAsset[] oldStates = transform.parent.GetChild(c).GetComponents<PrimitiveStateAsset>();
                    Array.Sort<PrimitiveStateAsset>(oldStates, new PrimitiveStateAsset.PSAComparer());
                    foreach (PrimitiveStateAsset pState in oldStates)
                    {
                        pState.LeaveState();
                    }
                }
            }
            //2. Activate us:
            bool allSuccessful = true;
            transform.gameObject.SetActive(true);
            PrimitiveStateAsset[] newStates = transform.GetComponents<PrimitiveStateAsset>();
            Array.Sort<PrimitiveStateAsset>(newStates, new PrimitiveStateAsset.PSAComparer());
            foreach (PrimitiveStateAsset pState in newStates)
            {
                Debug.Log("Queueing State " + pState.positionDescriptor.gameObject.name + " Order Num:"+ pState.orderNumber + "");
                allSuccessful &= pState.EnterState();                
            }

            if (allSuccessful)
            {
                OpenMPD_Wrapper.PrintMessage("STateSwitch -> requesting commit!");
                rm.RequestCommit();
            }
            doSwitch = !allSuccessful;
        }
    }

    private void OnEnable()
    {
        doSwitch = true;
    }
}
