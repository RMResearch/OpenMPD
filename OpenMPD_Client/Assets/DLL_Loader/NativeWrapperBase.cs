using UnityEngine;
using System.Runtime.InteropServices;
using System;
using System.Reflection;

namespace NativeAccess
{
    public class DLLAutoAssignAttribute : Attribute
    {

    }

    [ExecuteInEditMode]
    public abstract class NativeWrapperBase : MonoBehaviour
    {
        public const long ERROR_NOT_FOUND = 0x887A0002;
        public static string DIRECTORY_PREFIX = "";//@"C:\Users\Diego\Desktop\New Unity Project\";

        //private bool active = false;
        [Header("General Plugin configuration:")]
        public bool active = false;
        public bool activateOnStart = true;
        public int debugLevel = 3;

        public delegate bool BoolFunc();
        public delegate void Command();

        internal BoolFunc isInitialized;
        internal Command release;

        internal delegate void PrintFunc(string s);
        internal delegate void RegisterPrintFuncs(PrintFunc printMessage, PrintFunc printWarning, PrintFunc printError);

        public abstract string GetPluginName();
        protected virtual void PostInit() { }

        protected string printPrefix = "Plugin";

        internal long loadedPtr = 0;
        internal IntPtr libPtr;

        protected string prefix = null;

        protected virtual string defaultPrefix => null;

        protected virtual string registerPrintFuncsName => @"RegisterPrintFuncs";
        protected virtual string isInitializedFuncName => @"IsInitialized";        
        protected virtual string initFuncName => @"Initialize";
        protected virtual string releaseFuncName => @"Release";
    
        private  string PluginString(string s)
        {
           /* if (printPrefix == null)
                return s;
            else
                return "[" + printPrefix + "] " + s;*/
            return s;
        }

        private void PrintMessage(string s)
        {
            Debug.Log(PluginString(s));
        }

        private void PrintWarning(string s)
        {
            Debug.LogWarning(PluginString(s));
        }

        private  void PrintError(string s)
        {
           // Debug.LogError(PluginString(s));
            Debug.LogWarning(PluginString(s));
        }

        internal bool isRunning
        {
            get
            {
                return isLoaded;
            }
        }

        public virtual bool interfaceOk
        {
            get
            {
                return true;
            }
        }

        protected virtual string GetPluginPath()
        {
            return "NativeAccess/x64/Debug/";
        }

       /* protected virtual void OnEnable()
        {            
           Activate();         
        }

        protected virtual void OnDisable() {
            Deactivate();
        }*/
        protected virtual void Start()
        {
            if (activateOnStart && Application.isPlaying)//We want it to reload every time we launch
            {
                if (active)//It stayed on (maybe user clicking Active), so we deallocate previous instance
                    Deactivate();
                Activate();//Whatever happenend, we get a fresh loading of the DLL.
            }
        }
        protected virtual void OnApplicationQuit()
        {
            if (activateOnStart && active)//Deactivate on quit... only if it was active
                Deactivate();
        }
        protected virtual void Update()
        {
            if(!active && isLoaded)
            {
                Deactivate();
            }
            if(active && !isLoaded)
            {
                Activate();
            }
        }
        

        public bool isLoaded
        {
            get
            {
                return loadedPtr > 0;
            }
        }

        internal void PrintDebug(string msg, int level)
        {
            if (debugLevel >= level)
                print(msg);
        }

        internal void PrintDebug(string msg)
        {
            PrintDebug(msg, 1);
        }

        internal void PrintError(string msg, long code)
        {
            Debug.LogError(msg+": " + String.Format("{0:x}", code));
        }

        protected string prefixOutput => prefix == null ||prefix.Equals("") ? "" : "[" + prefix + "]";

        public static void DirFN(ref string fn)
        {
            if (fn == null)
                return;
            fn = fn.Trim();
            if (fn.Equals(""))
                return;
            if (!fn.EndsWith("/") && !fn.EndsWith("\\"))
                fn += "/";
        }

        public static string DirFN(string fn)
        {
            DirFN(ref fn);
            return fn;
        }

        internal Delegate GetFunction(string name, Type t, bool print = true) 
        {
            IntPtr funcAddr = IntPtr.Zero;
            if (prefix != null)
                funcAddr = Kernel.GetProcAddress(libPtr, prefix + name);
            if (funcAddr == IntPtr.Zero)
                funcAddr = Kernel.GetProcAddress(libPtr, name);
            if (funcAddr != IntPtr.Zero)
            {
                if (print)
                {
                    PrintDebug("[" + GetPluginName() + "] Function found: '" + prefixOutput + name + "'", 3);
                }
                return Marshal.GetDelegateForFunctionPointer(funcAddr, t);
            }
            else
            {
                if (print)
                {
                    Debug.LogError("[" + GetPluginName() + "] Function not found: '" + prefixOutput + name + "'");
                }
                return null;
            }
        }

        internal Command GetCommand(string name, bool printError = true)
        {
            return (Command)GetFunction(name, typeof(Command), printError);
        }

        public bool Activate()
        {
			if (!interfaceOk)
				return false;
            active = true;
            if (libPtr == IntPtr.Zero)
            {
                if (loadedPtr > 0)
                {
                    PrintDebug("RELOAD");
                    libPtr = new IntPtr(loadedPtr);
                }
                else
                {
                    prefix = defaultPrefix;
                    PrintDebug("LOAD");
                    string directory = DirFN(DIRECTORY_PREFIX + GetPluginPath());
                    string filename = directory + GetPluginName() + ".dll";
                    //Directory.SetCurrentDirectory(directory);     //I might need this for GSPAT (external dependencies).
                    libPtr = Kernel.LoadLibrary(filename);
                    loadedPtr = libPtr.ToInt64();
                    if (libPtr==IntPtr.Zero)
                    {
                        Debug.LogError("Could not load '"+ filename + "'");
                        active = false;
                        return false;
                    }
                    //Loads the "isInitialized" method 
                    isInitialized = (BoolFunc)GetFunction(isInitializedFuncName, typeof(BoolFunc), false);
                                        
                    //Should we have  a field so that the derived class can declare the name of its register print function?
                    RegisterPrintFuncs registerPrintFuncs = (RegisterPrintFuncs)GetFunction(registerPrintFuncsName, typeof(RegisterPrintFuncs), debugLevel >= 3);
                    if (registerPrintFuncs != null)
                    {
                        PrintDebug("Found 'RegisterPrintFuncs'", 2);
                        registerPrintFuncs(PrintMessage, PrintWarning, PrintError);
                    }

                    release = GetCommand(releaseFuncName, debugLevel >= 3);
                    if (release != null)
                        PrintDebug("Found 'Release'", 2);

                    //Check if we need to re-initialize it:
                    if (isInitialized == null || isInitialized() == false)
                    {   //We need to initialize the DLL:
                        BoolFunc initialize = (BoolFunc)GetFunction(initFuncName, typeof(BoolFunc), debugLevel >= 3);

                        if (initialize != null)
                        {
                            PrintDebug("Found 'Initialize'", 2);
                            if (!initialize())
                            {
                                PrintDebug("Initialization failed", 0);
                                Deactivate();
                                return false;
                            }
                        }
                        else
                            PrintDebug("No 'Initialize' found", 3);
                    }
                    //Assign methods specific to the DLL (these must be declared as fields in the derived class)
                    Type t = GetType();
                    foreach (FieldInfo field in t.GetFields(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance))
                    {
                        if (field.DeclaringType.IsSubclassOf(typeof(NativeWrapperBase)))
                        {
                            /*if (field.Name.StartsWith("ex_"))//What does this prefix stand for? 
                            {
                                object func = GetFunction(field.Name.Substring(3), field.FieldType, debugLevel >= 0);
                                field.SetValue(this, func);
                            }
                            else if (autoAssignAll && typeof(Delegate).IsAssignableFrom(field.FieldType))
                            {
                                object func = GetFunction(field.Name, field.FieldType, debugLevel >= 0);
                                field.SetValue(this, func);
                            }
                            else */
                            if (prefix != null /*&& field.Name.StartsWith(prefix)*/)
                            {
                                //object func = GetFunction(field.Name.Substring(prefix.Length), field.FieldType, debugLevel >= 0);
                                object func = GetFunction(field.Name, field.FieldType, debugLevel >= 0);
                                field.SetValue(this, func);
                            }
                            else
                            {
                                Attribute[] attributes = Attribute.GetCustomAttributes(field);
                                for (int i = 0; i < attributes.Length; i++)
                                {
                                    if (attributes[i] is DLLAutoAssignAttribute)
                                    {
                                        object func = GetFunction(field.FieldType.Name, field.FieldType, debugLevel >= 0);
                                        field.SetValue(this, func);
                                    }
                                }
                            }
                        }
                    }

                    PostInit();
                }
            }
            return true;
        }

        public virtual void Deactivate()
        {
            if (!active)
                return;
            active = false;
            if (!interfaceOk)
				return;
            if (loadedPtr <= 0)
                return;
            loadedPtr = 0;
            if(release!=null)
            {
                PrintDebug("CALL RELEASE", 1);
                release();
            }
            PrintDebug("FREE LIBRARY "+GetPluginName(), 1);
            Kernel.FreeLibrary(libPtr);
            libPtr = IntPtr.Zero;
        }

       
    }
}
