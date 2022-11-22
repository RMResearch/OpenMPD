using UnityEngine;
using System.Collections;
using System.IO.Ports;
using System.Text;
using System.Threading;
using System.Collections.Generic;
using MyBox;
using System;
using System.Diagnostics;
using System.Threading;
using Debug = UnityEngine.Debug;

public class GimbalController : MonoBehaviour
{
    [HideInInspector]
    Stopwatch stopWatch; // Used for precise timing
    [Separator("Gimbal Game Objects")]
    public GameObject outer;
    public GameObject inner;
    public bool applyRotations = true;

    [Separator("Communication Port")]
    private SerialPort comPortDevice;
    public string serialPortName = "COM16";
    public double baudRate = 115200;
    [ShowOnly] public bool connected = false;

    private Thread receiveThread;
    private volatile bool stopThread = false;
    private volatile static StringBuilder serialReceiveBuffer = new StringBuilder(100);

    [Separator("Properties")]
    public Status status;
    [System.Serializable]
    public class Status
    {
        [HideInInspector]
        public Mutex modificationMutex = new Mutex();
        [Separator("Position and Speed")]
        // The system powers on with 0 steps rotation
        // Can be polled from the device with 'g' command
        // Can be reset using the '0' command
        public int currentStepsOuter = 0;
        public int currentStepsInner = 0;
        // Steps / second
        public float currentSpeedInner = 0;
        public float currentSpeedOuter = 0;
        
        public bool isMovingOuter = false;
        public bool isMovingInner = false;
        // -1 or +1 when moving
        public int movingDirectionOuter = 0;
        public int movingDirectionInner = 0;

        [Separator("Timing and Prediction")]
        // Not needed, used for analysis of position update round-trip time
        public int posInternalTimestamp = 0; // MCU timestamp value
        [HideInInspector]
        public int posInternalTimestampDelta = 0; // Difference between MCU timestamp values
        [HideInInspector]
        public float posInternalTimestampDeltaSmoothed = 0; // Leaky integrator smoothed timestmap value (x * old) + ((1-x) * new)
        [HideInInspector]
        public float posInternalTimestampDeltaSmoothingFactor = 0.999f; // x in above equation
        
        public float fixedAdjustmentDelayS = 0.001f; // When predicting, add some additional constant delay

        public long startTime = DateTime.Now.Ticks; // t0
        [HideInInspector]
        public long currentAngleTimestamp = 0;
        public float currentAngleTimestampS = 0; // Seconds since t0 for last received data
        public float statusRefreshDelayS = 0.01f; // Max seconds since last timestamp before polling
        [HideInInspector]
        public Stopwatch sinceUpdateTimer;
        [HideInInspector]
        public Stopwatch sincePollTimer;
        [HideInInspector]
        public long lastPollTimestamp = 0; // Seconds since t0 for last data request
        public double lastPollTimestampS = 0; // Seconds since t0 for last data request
        public bool autoPoll = true; // Auto send data update requests

        [Separator("Debug")]
        public float innerError; // Debug
        public int counter = 0;
        public bool printErrorOnUpdate = false;

        public float GetSecondsSinceRefresh()
        {
            return (float)sinceUpdateTimer.Elapsed.TotalSeconds;
        }
        public float GetSecondsSincePoll()
        {
            return (float)sincePollTimer.Elapsed.TotalSeconds;
        }

        public float PredictCurrentStepsOuter()
        {
            // Predict current steps based on previous read value + (speed * dir * time)
            float outerAdjustmentSteps = movingDirectionOuter * currentSpeedOuter * (Mathf.Min(GetSecondsSinceRefresh(), 1) + fixedAdjustmentDelayS);
            return currentStepsOuter + outerAdjustmentSteps;
        }

        public float PredictCurrentStepsInner()
        {
            // Predict current steps based on previous read value + (speed * dir * time)
            //Debug.Log(GetSecondsSinceRefresh());
            float innerAdjustmentSteps = movingDirectionInner * currentSpeedInner * (Mathf.Min(GetSecondsSinceRefresh(), 1) + fixedAdjustmentDelayS);
            return currentStepsInner + innerAdjustmentSteps;
        }
    }

    public Configuration config;
    [System.Serializable]
    public class Configuration
    {
        [Separator("Precision and Speed")]
        // Currently set to the default script values, can be changed through calibration, or on the fly
        // Can be updated from device by sending 'D' in combo
        public int stepsPerRevolution = 200;

        public int stepperOuterMaxSpeed = 3200;
        public int stepperInnerMaxSpeed = 3200;
        public int stepperOuterMaxAccel = 1280;
        public int stepperInnerMaxAccel = 1280;
        public int stepperOuterMaxPullIn = 320;
        public int stepperOuterMaxPullOut = 320;
        public int stepperInnerMaxPullIn = 320;
        public int stepperInnerMaxPullOut = 320;

        public int ustepsPerRevOuter = 6400;
        public int ustepsPerRevInner = 6400;

        [Separator("Timing and Prediction")]

        public long startTime = DateTime.Now.Ticks;
        public long currentConfigTimestamp = 0;
        public float currentConfigTimestampS = 0;
        public float configRefreshDelayS = 5f;
        public long lastPollTimestamp = 0;
        public float lastPollTimestampS = 0;
        public bool autoPoll = false;

        public float GetSecondsSinceRefresh()
        {
            return (float)(((float)(DateTime.Now.Ticks - startTime) / TimeSpan.TicksPerSecond) - currentConfigTimestampS); ;
        }
        public float GetSecondsSincePoll()
        {
            return (float)(((float)(DateTime.Now.Ticks - startTime) / TimeSpan.TicksPerSecond) - lastPollTimestampS); ;
        }

        public float StepsToDegreesOuter(float steps)
        {
            return 360 * (steps / ustepsPerRevOuter);
        }

        public float StepsToDegreesInner(float steps)
        {
            return 360 * (steps / ustepsPerRevInner);
        }

        public float DegreesToStepsOuter(float degrees)
        {
            return (degrees / 360) * ustepsPerRevOuter;
        }

        public float DegreesToStepsInner(float degrees)
        {
            return (degrees / 360) * ustepsPerRevInner;
        }
    }


    #region SerialPortFunctions

    public void SerialReceiveThreadLoop() // TODO: Add receive config function
    {
        while (!stopThread && comPortDevice.IsOpen)
        {
            if (comPortDevice.BytesToRead > 0)
            {
                bool containsNewline = false;
                do
                {
                    containsNewline = false;
                    // If there are new bytes, add them all to the buffer
                    if (comPortDevice.BytesToRead > 0)
                    {
                        string indata = comPortDevice.ReadExisting();
                        serialReceiveBuffer.Append(indata);
                    }

                    // Scan the buffer in search of new line characters (message terminator)
                    int newlinePos = 0;
                    for (int i = 0; i < serialReceiveBuffer.Length; i++)
                    {
                        if (serialReceiveBuffer[i] == '\n')
                        {
                            containsNewline = true;
                            newlinePos = i + 1;
                            break;
                        }
                    }

                    // If we found at least one message
                    if (containsNewline)
                    {
                        // Extract and parse the message
                        string response = serialReceiveBuffer.ToString().Substring(0, newlinePos);
                        //Debug.Log("Response of length: " + response.Length + ": " + response);
                        if (response.StartsWith("Pos"))
                        {
                            string[] values = response.Substring(4, response.Length - 7).Split(',');
                            if (values.Length >= 5)
                            {
                                status.modificationMutex.WaitOne();
                                int lastStepsInner = status.currentStepsInner;
                                float lastPredictedStepsInner = status.PredictCurrentStepsInner();
                                status.currentStepsInner = int.Parse(values[0]);
                                status.isMovingInner = lastStepsInner != status.currentStepsInner;
                                if (lastStepsInner < status.currentStepsInner)
                                {
                                    status.movingDirectionInner = 1;
                                }
                                else if (lastStepsInner > status.currentStepsInner)
                                {
                                    status.movingDirectionInner = -1;
                                }
                                else
                                {
                                    status.movingDirectionInner = 0;
                                }
                                // Debug
                                status.innerError = GetAngleDegreesInner() - config.StepsToDegreesInner(lastPredictedStepsInner);

                                int lastStepsOuter = status.currentStepsOuter;
                                status.currentStepsOuter = int.Parse(values[1]);
                                status.isMovingOuter = lastStepsOuter != status.currentStepsOuter;
                                if (lastStepsOuter < status.currentStepsOuter)
                                {
                                    status.movingDirectionOuter = 1;
                                }
                                else if (lastStepsOuter > status.currentStepsOuter)
                                {
                                    status.movingDirectionOuter = -1;
                                }
                                else
                                {
                                    status.movingDirectionOuter = 0;
                                }


                                status.currentSpeedInner = float.Parse(values[2]);
                                status.currentSpeedOuter = float.Parse(values[3]);
                                int lastInternalTimestamp = status.posInternalTimestamp;
                                status.posInternalTimestamp = int.Parse(values[4]);
                                status.posInternalTimestampDelta = status.posInternalTimestamp - lastInternalTimestamp;
                                if (status.posInternalTimestampDelta < status.statusRefreshDelayS * 1000 * 2)
                                {
                                    if (status.posInternalTimestampDeltaSmoothed == 0) status.posInternalTimestampDeltaSmoothed = status.posInternalTimestampDelta;
                                    status.posInternalTimestampDeltaSmoothed = (status.posInternalTimestampDeltaSmoothingFactor * status.posInternalTimestampDeltaSmoothed) + ((1 - status.posInternalTimestampDeltaSmoothingFactor) * status.posInternalTimestampDelta);
                                }
                                status.currentAngleTimestamp = DateTime.Now.Ticks;
                                status.sinceUpdateTimer.Restart();
                                status.currentAngleTimestampS = (float)new TimeSpan(status.currentAngleTimestamp - status.startTime).TotalSeconds;
                                //status.counter = status.counter + 1;
                                if (status.printErrorOnUpdate)
                                {
                                    Debug.Log("Angle Err: " + status.currentStepsInner + ", " + status.posInternalTimestampDelta + ", " + status.currentSpeedInner + ", " + previousRotationInner);
                                }
                                status.modificationMutex.ReleaseMutex();
                            }
                            //Debug.Log(values);
                        }

                        // Trim the receive buffer to remove the processed message
                        serialReceiveBuffer.Remove(0, newlinePos);
                    }
                }
                // If we found a message, continue to check the remainder of the buffer for messages,
                while (containsNewline);
                // otherwise leave the loop and wait for new bytes to arrive
            }
        }
    }

    private bool InitializeCommunication()
    {
        comPortDevice = new SerialPort(serialPortName, (int)baudRate);
        comPortDevice.Parity = Parity.None;
        comPortDevice.StopBits = StopBits.One;
        comPortDevice.DataBits = 8;
        comPortDevice.ReadTimeout = 5000;
        comPortDevice.Handshake = Handshake.None;
        comPortDevice.RtsEnable = true;
        comPortDevice.DtrEnable = true;
        comPortDevice.Open();
        if (comPortDevice.IsOpen)
        {
            receiveThread = new Thread(SerialReceiveThreadLoop);
            receiveThread.Start();
        }
        else
        {
            Debug.LogError("Gimbal Connection Failed");
            Debug.Log(SerialPort.GetPortNames());
        }
        connected = comPortDevice.IsOpen;
        return connected;
    }
    void CloseSerialPort()
    {
        if (comPortDevice.IsOpen)
        {
            stopThread = true;
            receiveThread.Join();
            comPortDevice.Close();
        }
    }

    void SendDataToGimbal(float data)
    {
        if (comPortDevice.IsOpen)
            comPortDevice.Write(data.ToString());
    }

    public void SendStringToGimbal(string data)
    {
        if (comPortDevice.IsOpen)
        {
            comPortDevice.Write(data);
        }

    }

    public void RotateBy(float innerDegrees, float outerDegrees)
    {
        if (innerDegrees != 0 && outerDegrees != 0)
        {
            SendStringToGimbal("r,b," + ((int)config.DegreesToStepsInner(innerDegrees)).ToString() + "," + ((int)config.DegreesToStepsOuter(outerDegrees)).ToString() + ";");
        }
        else if (innerDegrees == 0 && outerDegrees != 0)
        {
            SendStringToGimbal("r,o," + ((int)config.DegreesToStepsOuter(outerDegrees)).ToString() + ";");
        }
        else if (innerDegrees != 0 && outerDegrees == 0)
        {
            SendStringToGimbal("r,i," + ((int)config.DegreesToStepsInner(innerDegrees)).ToString() + ";");
        }
    }

    public void RotateTo(float innerDegrees, float outerDegrees)
    {
        SendStringToGimbal("a,b," + ((int)config.DegreesToStepsInner(innerDegrees)).ToString() + "," + ((int)config.DegreesToStepsOuter(outerDegrees)).ToString() + ";");
    }

    [ButtonMethod]
    public void RequestStatusUpdate()
    {
        SendStringToGimbal("g,b;");
    }

    [ButtonMethod]
    public void RequestConfigUpdate()
    {
        SendStringToGimbal("D");
    }

    [ButtonMethod]
    public void SetAsHome()
    {
        SendStringToGimbal("0,b;");
        status.currentStepsInner = 0;
        status.currentStepsOuter = 0;
        previousRotationInner = 0;
        previousRotationOuter = 0;
    }
    #endregion

    #region GimbalFunctions

    // Returns how many degrees of rotation have been applied to the outer gimbal relative to '0'
    // No upper or lower bound, Mod returned value with 360 to convert to single rotation
    public float GetAngleDegreesOuter()
    {
        return config.StepsToDegreesOuter(status.currentStepsOuter);
    }

    public float GetPredictedAngleDegreesOuter()
    {
        return config.StepsToDegreesOuter(status.PredictCurrentStepsOuter());
    }

    // Returns how many degrees of rotation have been applied to the outer gimbal relative to '0'
    // No upper or lower bound, Mod returned value with 360 to convert to single rotation
    public float GetAngleDegreesInner()
    {
        return config.StepsToDegreesInner(status.currentStepsInner);
    }

    public float GetPredictedAngleDegreesInner()
    {
        return config.StepsToDegreesInner(status.PredictCurrentStepsInner());
    }

    public bool isRotating()
    {
        return status.isMovingInner | status.isMovingOuter;
    }

    #endregion

    #region UnityFunctions

    private void Start()
    {
        stopWatch = Stopwatch.StartNew();
        status.sincePollTimer = Stopwatch.StartNew();
        status.sinceUpdateTimer = Stopwatch.StartNew();
        InitializeCommunication();
        if (connected)
        {
            config.autoPoll = true;
            SetAsHome();
        }
    }

    private void OnDestroy()
    {
        CloseSerialPort();
    }

    [HideInInspector]
    // Stores the previous rotation value for interpolating, necessary when unpredicted changes in received angle occur
    public float previousRotationInner = 0;
    [HideInInspector]
    public float previousRotationOuter = 0;
    // Used for debug
    [HideInInspector]
    public float diffOuterBetweenInterpAndDesired = 0;
    [HideInInspector]
    public float diffInnerBetweenInterpAndDesired = 0;
    [HideInInspector]
    public float smoothedInnerDelta;
    [HideInInspector]
    public float smoothedOuterDelta;
    public float innerDeltaSmoothing = 0.9f;
    public float innerSpeedSmoothing = 0.003f;
    public float outerDeltaSmoothing = 0.9f;
    public float outerSpeedSmoothing = 0.003f;

    float lastAngle;
    void Update()
    {
        // If autoPoll and time passed, request new status or config
        if (status.autoPoll && status.GetSecondsSinceRefresh() > status.statusRefreshDelayS && status.GetSecondsSincePoll() > status.statusRefreshDelayS)
        {
            RequestStatusUpdate();
            status.lastPollTimestamp = DateTime.Now.Ticks;
            status.lastPollTimestampS = (float)new TimeSpan(status.lastPollTimestamp - status.startTime).TotalSeconds;
            status.sincePollTimer.Restart();
        }
        if (config.autoPoll && config.GetSecondsSinceRefresh() > config.configRefreshDelayS && config.GetSecondsSincePoll() > config.configRefreshDelayS)
        {
            RequestConfigUpdate();
            config.lastPollTimestamp = DateTime.Now.Ticks;
            config.lastPollTimestampS = (float)new TimeSpan(config.lastPollTimestamp - config.startTime).TotalSeconds;
        }

        // If rotations should be applied to selected gimbal game objects
        if (applyRotations)
        {
            status.modificationMutex.WaitOne();
            float msecSinceRefresh = status.GetSecondsSinceRefresh() * 1000;
            float interpDuration = Mathf.Max(status.posInternalTimestampDeltaSmoothed, 1);
            float interpFraction = msecSinceRefresh / interpDuration;
            if (float.IsNaN(previousRotationOuter)){
                previousRotationOuter = 0;
            }
            float desiredAngleOuter = GetPredictedAngleDegreesOuter();
            float newAngleOuter = Mathf.Lerp(previousRotationOuter, desiredAngleOuter, interpFraction); // By the next update we should be back on track
            float desiredOuterDelta = newAngleOuter - previousRotationOuter;
            float outerAdjustedSmoothing = (status.currentSpeedOuter / config.stepperOuterMaxSpeed) * outerSpeedSmoothing;
            smoothedOuterDelta = ((1 - (outerDeltaSmoothing + outerAdjustedSmoothing)) * desiredOuterDelta) + ((outerDeltaSmoothing + outerAdjustedSmoothing) * smoothedOuterDelta);
            newAngleOuter = previousRotationOuter + desiredOuterDelta;
            outer.transform.localRotation = Quaternion.AngleAxis(newAngleOuter, new Vector3(1, 0, 0));
            previousRotationOuter = newAngleOuter;
            diffOuterBetweenInterpAndDesired = desiredAngleOuter - newAngleOuter;

            if (float.IsNaN(previousRotationInner)){
                previousRotationInner = 0;
            }
            float desiredAngleInner = GetPredictedAngleDegreesInner();
            float newAngleInner = Mathf.Lerp(previousRotationInner, desiredAngleInner, interpFraction); // By the next update we should be back on track
            float desiredInnerDelta = newAngleInner - previousRotationInner;
            float innerAdjustedSmoothing = (status.currentSpeedInner / config.stepperInnerMaxSpeed) * innerSpeedSmoothing;
            smoothedInnerDelta = ((1 - (innerDeltaSmoothing + innerAdjustedSmoothing)) * desiredInnerDelta) + ((innerDeltaSmoothing + innerAdjustedSmoothing) * smoothedInnerDelta);
            newAngleInner = previousRotationInner + smoothedInnerDelta;
            inner.transform.localRotation = Quaternion.AngleAxis(newAngleInner, new Vector3(0, 1, 0));
            if (status.printErrorOnUpdate)
            {
                Debug.Log("New Angle: " + GetAngleDegreesInner() + ", " + desiredAngleInner + ", " + previousRotationInner + ", " + msecSinceRefresh + ", " + interpDuration  + ", " + interpFraction + ", " + newAngleInner + ", " + Time.realtimeSinceStartup);
            }
            previousRotationInner = newAngleInner;
            diffInnerBetweenInterpAndDesired = desiredAngleInner - newAngleInner;
            status.modificationMutex.ReleaseMutex();
        }
    }

    [ButtonMethod]
    public void TestSpinInner()
    {
        RotateBy(360,0);
    }

    [ButtonMethod]
    public void TestSpinOuter()
    {
        RotateBy(0, 360);
    }

    #endregion
}

