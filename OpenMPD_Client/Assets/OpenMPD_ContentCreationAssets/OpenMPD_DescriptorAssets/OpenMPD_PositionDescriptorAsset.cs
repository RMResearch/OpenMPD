using UnityEngine;
using System.Collections;

public abstract class PositionDescriptorAsset : MonoBehaviour
{
    protected Positions_Descriptor descriptor;
    public uint GetDescriptorID() { return descriptor.positionsDescriptorID; }
    // Use this for initialization
    void Start()
    {

    }

    // Update is called once per frame
    void Update()
    {

    }
}
