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

/** This class controls the global parameters related to the OpenMPD platform, independent of specific applications/scenes. 
 *  These include general aspects, such as the device type, arrangement, connection IDs, framerate supported (or preferred), solver.
 *  This class also provides general management of the runtime cycle, keeping track of the contents generated via the OpenMPD_ContextManager.
 *  - Client applications will deal mostly with the Context Manager, to create, update and animate their OpenMPD applications. 
 *  - Client applications will mostly interact with the Presentation Manager when starting/stopping the system, and this actually happens in 
 *    a transparent way (i.e., just by attaching this script to an object in the scene). */
public class OpenMPD_PresentationManager : MonoBehaviour
{
    [Separator("Board connection parameters (IDs):")]    
    public uint topBoardID = 6; //ID for top board. Set to 0, if only one board used.
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
    [ReadOnly] public bool running = false;
    public bool commit = true;
    public bool phaseOnly = true;
    public bool isHardwareSync = true;
    public bool forceSync = false;
    public bool verboseMode = false;
       
    Thread engineUpdater;
    [HideInInspector] public bool isCurrentIndexReadingActive = false;
        
    //Variable to manage content:
    List<PrimitiveGameObject> contents;
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
        //Build a High level update (just updating primitive transformation matrices, no changes to the scene)
        if (contents.Count != 0)
        {
            OpenMPD_RenderingUpdate update = BuildPBD_Update();
            SendPBD_Update(update);
        }
        //Send a commit if changes to the scene have happened
        if (commit)
        {
            CommitPBDChanges();
        }
    }

    void OnDestroy()
    {
        running = false;
        //1.Destroy all content-related stuff(primitives and descriptors)
        // 1.a Primitives
        foreach (PrimitiveGameObject c in contents)
            //Remove from engine
            OpenMPD_Wrapper.OpenMPD_CWrapper_releasePrimitive(primitiveManagerHandler, c.GetPrimitiveID());
        contents.Clear();
        OpenMPD_Wrapper.OpenMPD_CWrapper_commitUpdates(primitiveManagerHandler);
        OpenMPD_Wrapper.OpenMPD_CWrapper_update_HighLevel(primitiveManagerHandler, null, 0, null, null);
        Thread.Sleep(100);
        //1.b. Descriptors (the Sleep above makes sure no Primitive is using the descriptors by the time we destroy them)
        OpenMPD_ContextManager.Instance().RemoveAllDescriptors();
        //2. Stop the engine now that all resources have been destroyed.
        OpenMPD_Wrapper.OpenMPD_CWrapper_StopEngine();
        OpenMPD_Wrapper.OpenMPD_CWrapper_RegisterPrintFuncs(null, null, null);
        OpenMPD_Wrapper.OpenMPD_CWrapper_Release();
    }
    #endregion

    

    #region Utilities
    protected OpenMPD_PresentationManager()
    {
        running = false;
        contents = new List<PrimitiveGameObject>();
        primitiveManagerHandler = 0;
    }

    /**Returns true if the OpenMPD engine is still running. This method is useful as the running cycle can happen in a separate thread.*/
    public bool IsRunning()
    {
        return running;
    }

    /** This method configures the engine, loading the resources required (OpenCL shader, OpenGL context from Unity), creating solvers
     * , connecting to the device and setting the configuration specified by the client  */
    void SetupEngine()
    {
        if (topBoardID == 0 && bottomBoardID == 0)
        {
            Debug.LogError("Incorrect board IDs (zero). Provide valid IDs. If the device is not connected, simultation will still run. ");
            return;
        }
        //Setup, unless we had already done so.
        if (!IsRunning())
        {
            //1. Check if there is a GL_Plugin enabled attached to the levitator and if it is ready. In this case, wait until it is ready to provide us with the OpenGL context. 
            if (transform.GetComponents<GL_RenderingPlugin>().Length != 0
                && transform.GetComponents<GL_RenderingPlugin>()[0].isActiveAndEnabled
                && !GL_RenderingPlugin.VisualRenderedReady())
                return;
            //2. Proceed with initialization
            if(verboseMode)//See Messages, Warinings and Errors.
                OpenMPD_Wrapper.OpenMPD_CWrapper_RegisterPrintFuncs(OpenMPD_Wrapper.PrintMessage, OpenMPD_Wrapper.PrintWarning, OpenMPD_Wrapper.PrintError);
            else //See Warnings and Errors only
                OpenMPD_Wrapper.OpenMPD_CWrapper_RegisterPrintFuncs(null, OpenMPD_Wrapper.PrintWarning, OpenMPD_Wrapper.PrintError);
            OpenMPD_Wrapper.OpenMPD_CWrapper_Initialize();
            OpenMPD_Wrapper.OpenMPD_CWrapper_SetupEngine(memorySizeInBytes, GSPAT_Solver_Version, GL_RenderingPlugin.getOpenGLVisualRenderer());
            if (topBoardID == 0)
            {
                float[] boardLocation = new float[16] { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 };                
                primitiveManagerHandler = OpenMPD_Wrapper.OpenMPD_CWrapper_StartEngine_SingleBoard(GS_PAT_Divider, numGeometriesInParallel, bottomBoardID, boardLocation, forceSync);
            }
            else
                primitiveManagerHandler = OpenMPD_Wrapper.OpenMPD_CWrapper_StartEngine_TopBottom(GS_PAT_Divider, numGeometriesInParallel, topBoardID, bottomBoardID, forceSync);
            OpenMPD_Wrapper.OpenMPD_CWrapper_SetupPhaseOnly(phaseOnly);
            OpenMPD_Wrapper.OpenMPD_CWrapper_SetupHardwareSync(isHardwareSync);

            running = true;
        }
    }
    /**Returns the OpenMPD Presentation Manager defined for this Unity Scene. 
     * The method will return NULL if:
     *  - the manager is still not ready (e.g., OpenGL initialization).
     *  - no Presentation Manager was defined in the scene (i.e., this script must be attached to an object, typically LevitatorOrigin).
     */
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
    /**Helper method to select and configure the solver to use.
     * The current version is limited to the 3 basic solvers (Naive, IBP and GSPAT).
     * No parameter configuration is allowed.
     * TODO: Add support for advanced solvers (BEM) and allow parameter configuration. 
      */
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
    /**Helper method to select and configure the divider to use, to determine the update rate used by the device.
     * TODO: Only most common dividers included. Higher dividers (for slower rates) could be added, such as 5 (8000Hz), 8 (5000Hz), etc.
     */
    private void SelectFrameRateDevider(UpdateRateDivider upsDev)
    {
        switch (upsDev)
        {
            case UpdateRateDivider.by1://40khz
                GS_PAT_Divider = 1;
                break;
            case UpdateRateDivider.by2://20khz
                GS_PAT_Divider = 2;
                break;
            case UpdateRateDivider.by3://13.33khz
                GS_PAT_Divider = 3;
                break;
            case UpdateRateDivider.by4://10khz
            default:
                GS_PAT_Divider = 4;
                break;
            
        }
    }

    /**This method indicates the Presentation Manager that changes have been made to the scene which require a commit.
     * These can include defining new primitives, assigning new Descriptors to them, etc.*/
    public void RequestCommit()
    {
        commit = true;//We mark it. The message will be sent during the end of cyfle (Update).
    }
    /**Helper method internally invoked when a commit has been requested.*/
    private void CommitPBDChanges()
    {
        OpenMPD_Wrapper.OpenMPD_CWrapper_commitUpdates(primitiveManagerHandler);
        commit = false;
    }

    /**Traverse all active primitives and build a high level update for each of them. 
     * This method is invoked once per Unity cycle. Please note that forceSync might also tie the execution cycles of Unity and OpenMPD.*/
    OpenMPD_RenderingUpdate BuildPBD_Update()
    {
        OpenMPD_RenderingUpdate update = new OpenMPD_RenderingUpdate();
        PrimitiveGameObject[] _contents = contents.ToArray();
        for (int i = 0; i < _contents.Length; i++)
            if (_contents[i].enabled)
            {
                _contents[i].FillCurrentPBDUpdate(update);
            }
        return update;
    }
    /** Send a high level update to the OpenMPD engine. 
     * This method is invoked once per Unity cycle. Please note that forceSync might also tie the execution cycles of Unity and OpenMPD.
     */
    private void SendPBD_Update(OpenMPD_RenderingUpdate update)
    {
        OpenMPD_Wrapper.OpenMPD_CWrapper_update_HighLevel(primitiveManagerHandler
            , update.primitiveIds, update.NumPrimitives()
            , update.matricesStart, update.matricesEnd);
    }

    //Methods to keep track of Contents declared in the system.
    /**Returns a handler to the current OpenMPD system (understood as the device, but also the external DLL with its resources). */
    public long getCurrentEngineHandler()
    {
        return primitiveManagerHandler;
    }
    /**Returns the Unity node representing the location of our levitator. 
     * This is required as the position of the primitives will be made relative to this node. 
     * Either the Primitives or the Levitator node can be moved (e.g., see the GymbalPAT demo, in which the levitator rotated around the content).    
     */
    public Transform GetLevitatorNode()
    {
        return GameObject.Find("LevitatorOrigin").transform;
    }
    /**
     * This method defines a Primitive in OpenMPD, returning the associated Primitive ID. 
     * The primitive will also be part of the active scene and will be automatically updated in subsequent cycles. 
     * If the Primitive had already beed defined, this command is ignored. 
     * Please note that the default behaviour of a primitive automatically calls this method upon creation (see Primitive::Setup() method). 
     * Thus, the default assumption is that any Primitive object in the Unity Scene has a (low level) OpenMPD primitive in the engine. 
     * The client can by-pass this, calling RemoveContent after creation. This can be a way of having more than 32 Primitives ready in Unity, 
     * using them when needed by calling CreateContent on them (i.e., OpenMPD only allow 32 primitives at any point). 
     * This is an edge case, and we do not recommend this type of usage. 
     */
    public uint CreateContent(PrimitiveGameObject p)
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
    /**Enables or disableds a primitive. */
    public bool SetContentEnabled(PrimitiveGameObject p, bool enabled)
    {
        if (contents.Contains(p))
        {
            OpenMPD_Wrapper.OpenMPD_CWrapper_setPrimitiveEnabled(primitiveManagerHandler, p.GetPrimitiveID(), enabled);
            return true;
        }
        return false;
    }
    /**Removes a primitive from the active scene and from OpenMPD. 
     * Please note the Primitive object still exists, and the client can use it in the future again, by calling CreateContent.  */
    public void RemoveContent(PrimitiveGameObject p)
    {
        //Remove local
        if (contents.Contains(p))
            contents.Remove(p);
        //Remove from engine
        OpenMPD_Wrapper.OpenMPD_CWrapper_releasePrimitive(primitiveManagerHandler, p.GetPrimitiveID());
    }
    #endregion

}
