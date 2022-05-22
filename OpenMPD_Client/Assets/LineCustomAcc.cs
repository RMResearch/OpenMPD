using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public enum OptiPathShape
{
    Circle, Square, Fish, Cardioid, noShape
}

public enum SpeedTestType
{
    Vertical, verticalInv, Horizontal, HorizontalInv
}
public class LineCustomAcc : PositionDescriptorAsset
{
    [Header("DescriptorVariables")]
    [ShowOnly] public int numSamples = 0;
    [ShowOnly] public uint descriptorID = 0;
    public Vector3 initialPos;

    [Header("SpeedTest")]
    public bool isSpeedTest = false;
    public SpeedTestType test = SpeedTestType.Vertical;
    public Vector3 start;
    public Vector3 end;
    public float Acc0 = 400; // increments of 10cm/s^2
    [ShowOnly] public bool isWayBack = false;
    [ShowOnly] public int curFPS_Divider = 4;

    // loca variables
    float acc0 = 400;
    float vel0 = 0.1f;

    // Start is called before the first frame update
    void Start()
    {

    }

    // Update is called once per frame
    void Update()
    {
        if (OpenMPD_PresentationManager.Instance())
        {
            if (isSpeedTest)
            {
                SetSpeedTestMode(test);
                float dt = (1.0f * curFPS_Divider) / 40000;
                Vector3[] resultPositions;
                if (!isWayBack)
                    resultPositions = createLinearTest(start, end, 0, acc0, dt);
                else
                    resultPositions = createLinearTest(start, end, vel0, 1, dt);
                descriptor = new Positions_Descriptor(resultPositions);
                numSamples = resultPositions.Length;
                descriptorID = descriptor.positionsDescriptorID;
                isSpeedTest = false;
            }
        }
    }

    void SetSpeedTestMode(SpeedTestType STest)
    {
        switch (STest)
        {
            case SpeedTestType.Vertical:
                start = new Vector3(0, 0.04f, 0);
                end = new Vector3(0, -0.06f, 0);
                acc0 = Acc0;
                isWayBack = false;
                break;
            case SpeedTestType.verticalInv:
                start = new Vector3(0, -0.06f, 0);
                end = new Vector3(0, 0.04f, 0);
                acc0 = Acc0;
                isWayBack = true;
                break;
            case SpeedTestType.Horizontal:
                start = new Vector3(-0.05f, 0, 0);
                end = new Vector3(0.05f, 0, 0);
                acc0 = Acc0;
                isWayBack = false;
                break;
            case SpeedTestType.HorizontalInv:
                start = new Vector3(0.05f, 0, 0);
                end = new Vector3(-0.05f, 0, 0);
                acc0 = Acc0;
                isWayBack = true;
                break;
        }
    }

    Vector3[] createLinearTest(Vector3 start, Vector3 end, float v0, float a0, float dt)
    {
        List<Vector3> positions = new List<Vector3>();
        //0. Get unitary vector in direction of motion
        Vector3 A = start;
        Vector3 B = end;
        Vector3 midPoint = (A + B) / 2.0f;

        Vector3 direction = B - A;
        direction = direction.normalized;

        //1. Declare state: 
        Vector3 p_t = A;
        float v_t = v0, a_t = a0;
        //2. Accelerate part: Accelerate until we reach mid point (dot product becomes negative).
        while (Vector3.Dot(direction, (midPoint - p_t)) > 0)
        {
            positions.Add(p_t);
            v_t += a0 * dt;
            p_t += v_t * direction * dt;
        }
        //3. Decelerate part (until we reach B).
        while (Vector3.Dot(direction, (B - p_t)) > 0)
        {
            positions.Add(p_t);
            v_t -= a0 * dt;
            if (v_t < 0.1f) v_t = 0.1f;//minimum speed 0.1m/s (otherwise, v could become zero, and this loop would never end... this only affects last steps in the test (low speeds)
            p_t += v_t * direction * dt;
        }
        positions.Add(B);
        return positions.ToArray();
    }


}
