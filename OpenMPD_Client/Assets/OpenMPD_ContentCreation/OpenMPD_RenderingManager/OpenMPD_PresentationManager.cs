using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.Threading;
using MyBox;

public enum SolverType
{
    Naive, IBP, GSPAT
}

public enum UpdateRateDivider
{
    by1, by2, by3, by4
}

public class OpenMPD_PresentationManager : MonoBehaviour
{
    [Separator("Board connection parameters (IDs):")]    
    public uint topBoardID = 6;
    public uint bottomBoardID = 3;       
    public SolverType solver = SolverType.GSPAT;
    [ReadOnly] public int GSPAT_Solver_Version = 0;
    [ReadOnly] public uint boardType = 0;

    bool connected;
    //Parameters controlling rendering pipeline
    [Separator("Pipeline Control Parameters:")]
    public UpdateRateDivider divider = UpdateRateDivider.by4;
    [ReadOnly] public byte GS_PAT_Divider = 4;
    [ReadOnly] public float ResultingFPS = 40000f / 4;
    [ReadOnly] public uint numGeometriesInParallel = 32;
    [ReadOnly] public static uint maxContents = 32;
    [ReadOnly] public uint memorySizeInBytes = 2000000;
    [ReadOnly] public int currentPosIndex;

    //Variables to access/control the board
    [Separator("Connected and running:")]
    [ReadOnly] public bool status = false;
    public bool tickThisToCommit = true;
    public bool phaseOnly = true;
    public bool isHardwareSync = true;
    public bool forceSync = false;
       
    Thread engineUpdater;
    [HideInInspector] public bool isCurrentIndexReadingActive = false;
        
    //Variable to manage content:
    List<Primitive> contents;
    private long primitiveManagerHandler;//Handler to use the C++ wrapper.

    #region Native mothods
    // Use this for initialization
    void Start()
    {
        SelectSolver(solver);
        SelectFrameRateDevider(divider);
        Instance();
        ResultingFPS = 40000f / GS_PAT_Divider;//Keep this field updated
    }

    // Update is called once per frame
    void Update()
    {
        ResultingFPS = 40000f / GS_PAT_Divider;//Keep this field updated
        if (!IsRunning())
            return;
        if (tickThisToCommit)
        {
            CommitPBDChanges();
        }
        //Debug.Log("Current target framerate: "+ Application.targetFrameRate);

    }

    void OnDestroy()
    {
        //0. Stop the engineUpdater thread (Notify stop and wait).
        status = false;
        while (engineUpdater.IsAlive)
            Thread.Sleep(1);

        //1.Destroy all content-related stuff(primitives and descriptors)
        foreach (Primitive c in contents)
            //Remove from engine
            OpenMPD_Wrapper.OpenMPD_CWrapper_releasePrimitive(primitiveManagerHandler, c.GetPrimitiveID());
        contents.Clear();

        OpenMPD_ContextManager .Instance().RemoveAllDescriptors();
        OpenMPD_Wrapper.OpenMPD_CWrapper_commitUpdates(primitiveManagerHandler);
        OpenMPD_Wrapper.OpenMPD_CWrapper_update_HighLevel(primitiveManagerHandler, null, 0, null, null);
        Thread.Sleep(100);
        //1. Stop the engine
        OpenMPD_Wrapper.OpenMPD_CWrapper_StopEngine();
        OpenMPD_Wrapper.OpenMPD_CWrapper_RegisterPrintFuncs(null, null, null);
        OpenMPD_Wrapper.OpenMPD_CWrapper_Release();
    }
    #endregion

    #region Threads
    /** This thread is in charge or reading the current status of the scene and sending it to the engine
    *  The thread is created when the system is ready to run (see SetupEngine()) and stays active until the 
    *  application is destroyed (see OnDestroy);
   **/
    private void engineUpdater_Thread()
    {
        while (IsRunning())
        {
            if (contents.Count != 0)
            {
                OpenMPD_RenderingUpdate update = BuildPBD_Update();
                SendPBD_Update(update);
            }
        }
    }
    #endregion

    #region Utilities
    protected OpenMPD_PresentationManager()
    {
        status = false;
        contents = new List<Primitive>();
        primitiveManagerHandler = 0;
    }
    public bool ActivateCurrentIndexReading()
    {
        return (isCurrentIndexReadingActive = true);
    }

    public bool IsRunning()
    {
        return status;
    }

    void SetupEngine()
    {
        if (!IsRunning())
        {
            //1. Check if there is a GL_Pluing enabled attache to the levitator and it is ready.
            if (transform.GetComponents<GL_RenderingPlugin>().Length != 0
                && transform.GetComponents<GL_RenderingPlugin>()[0].isActiveAndEnabled
                && !GL_RenderingPlugin.VisualRenderedReady())
                return;
            //2. Proceed with initialization
            OpenMPD_Wrapper.OpenMPD_CWrapper_RegisterPrintFuncs(OpenMPD_Wrapper.PrintMessage, OpenMPD_Wrapper.PrintWarning, OpenMPD_Wrapper.PrintError);
            //OpenMPD_Wrapper.OpenMPD_CWrapper_RegisterPrintFuncs(null, null, null);
            OpenMPD_Wrapper.OpenMPD_CWrapper_Initialize();
            OpenMPD_Wrapper.OpenMPD_CWrapper_SetupEngine(memorySizeInBytes, GSPAT_Solver_Version, GL_RenderingPlugin.getOpenGLVisualRenderer());
            primitiveManagerHandler = OpenMPD_Wrapper.OpenMPD_CWrapper_StartEngine(GS_PAT_Divider, numGeometriesInParallel, topBoardID, bottomBoardID, forceSync);
            OpenMPD_Wrapper.OpenMPD_CWrapper_SetupPhaseOnly(phaseOnly);
            OpenMPD_Wrapper.OpenMPD_CWrapper_SetupHardwareSync(isHardwareSync);

            //OpenMPD_CWrapper_SetupHardwareSync
            status = true;
            engineUpdater = new Thread(engineUpdater_Thread);
            engineUpdater.Start();
        }
    }

    public static OpenMPD_PresentationManager Instance()
    {
        //Find it and make sure it has been already Setup (otherwise, return NULL).
        OpenMPD_PresentationManager _instance = FindObjectOfType<OpenMPD_PresentationManager>();
        if (_instance != null)
            _instance.SetupEngine();
        if (_instance != null && _instance.IsRunning())
            return _instance;
        return null;
    }

    private void SelectSolver(SolverType solv)
    {
        switch (solv)
        {
            case SolverType.Naive:
                GSPAT_Solver_Version = 0;
                break;
            case SolverType.IBP:
                GSPAT_Solver_Version = 1;
                break;
            case SolverType.GSPAT:
                GSPAT_Solver_Version = 2;
                break;
            default:
                GSPAT_Solver_Version = (int)solv;
                break;
        }
    }

    private void SelectFrameRateDevider(UpdateRateDivider upsDev)
    {
        switch (upsDev)
        {
            case UpdateRateDivider.by1: //40khz
                GS_PAT_Divider = 1;
                break;
            case UpdateRateDivider.by2://20khz
                GS_PAT_Divider = 2;
                break;
            case UpdateRateDivider.by3://13.33khz
                GS_PAT_Divider = 3;
                break;
            case UpdateRateDivider.by4://10khz
                GS_PAT_Divider = 4;
                break;
            default:
                break;
        }
    }

    //Method to indicate the Rendering Manager that changes have been made requiring commit.
    public void RequestCommit()
    {
        tickThisToCommit = true;
    }

    private void CommitPBDChanges()
    {
        OpenMPD_Wrapper.OpenMPD_CWrapper_commitUpdates(primitiveManagerHandler);
        tickThisToCommit = false;
    }

    OpenMPD_RenderingUpdate BuildPBD_Update()
    {
        OpenMPD_RenderingUpdate update = new OpenMPD_RenderingUpdate();
        Primitive[] _contents = contents.ToArray();
        ////foreach (Primitive c in _contents)
        for (int i = 0; i < _contents.Length; i++)
            if (_contents[i].PrimitiveEnabled())
            {
                _contents[i].FillCurrentPBDUpdate(update);
            }
        return update;
    }

    private void SendPBD_Update(OpenMPD_RenderingUpdate update)
    {
        OpenMPD_Wrapper.OpenMPD_CWrapper_update_HighLevel(primitiveManagerHandler
            , update.primitiveIds, update.NumPrimitives()
            , update.matricesStart, update.matricesEnd);
    }

    //Methods to keep track of Contents declared in the system.
    public long getCurrentEngineHandler()
    {
        return primitiveManagerHandler;
    }

    public Transform GetLevitatorNode()
    {
        return GameObject.Find("LevitatorOrigin").transform;
    }

    public uint CreateContent(Primitive p)
    {
        //Check that it was not created already
        if (contents.Contains(p))
            return 0;
        //Create locally
        contents.Add(p);
        //Create in C++ engine
        uint primitiveID = OpenMPD_Wrapper.OpenMPD_CWrapper_declarePrimitive(primitiveManagerHandler
                                , OpenMPD_ContextManager .Instance().GetDefaultPositionsDescriptor()
                                , OpenMPD_ContextManager .Instance().GetDefaultAmplitudesDescriptor());
        return primitiveID;
    }

    public bool SetContentEnabled(Primitive p, bool enabled)
    {
        if (contents.Contains(p))
        {
            OpenMPD_Wrapper.OpenMPD_CWrapper_setPrimitiveEnabled(primitiveManagerHandler, p.GetPrimitiveID(), enabled);
            return true;
        }
        return false;
    }

    public void RemoveContent(Primitive p)
    {
        //Remove local
        if (contents.Contains(p))
            contents.Remove(p);
        //Remove from engine
        OpenMPD_Wrapper.OpenMPD_CWrapper_releasePrimitive(primitiveManagerHandler, p.GetPrimitiveID());
    }
    #endregion

}
