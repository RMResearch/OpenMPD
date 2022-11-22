using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;

namespace Assets.Utilities._0.Testing
{
    public class ColourManager : MonoBehaviour
    {
        #region Variables
        [Header("Configuration")]        
        public GameObject particle;

        [Header("Coloring")]
        public Color baseColor = Color.green;
        [Range(0.0f, 0.005f)]
        public float sphereSize = 0.001f;
        public int step = 30;
        

        [Header("ForShader")]
        public Color shaderColor = Color.cyan;
        [Range(0.0001f, 0.03f)]
        public float diskSize = 0.01f;
               

        [Header("Read Write")]
        public ColouringMode colourMode = ColouringMode.none;
        public string fileName;

        [Header("Testing")]
        [Range(5, 100)]
        public int windowSize = 10;
        public bool startRenderingColor = false;

        List<Vector3> selectedPos = new List<Vector3>();
        List<Color32> selectedCol = new List<Color32>();
        List<Vector3> localPosList = new List<Vector3>();
        List<Color32> localColList = new List<Color32>();

        // status
        bool isConfigured = false;
        uint posDescID = 0;
        uint prevID = 0;
        int prevStep = 0;

        #region Shader variables
        Color _pointTint = new Color(0.5f, 0.5f, 0.5f, 1);

        [Header("Shaders")]
        public Shader _pointShader = null;
        public Shader _diskShader = null;

        [Header("line Param")]
        public Material lineMat;

        // status
        bool isRenderBufferReady = false;

        // main compute buffer
        public ComputeBuffer sourceBuffer;

        // local materials
        private Material _pointMaterial;
        private Material _diskMaterial;

        struct Point
        {
            public Vector3 position;
            public uint color;
        }

        Point[] _pointData;

        public const int elementSize = sizeof(float) * 4;
        #endregion

        [HideInInspector]
        public float[] positions;
        // list to spawn spheres to show the current rendering path
        List<GameObject> sphereslist = new List<GameObject>();

        // list to store the downsized pos from hte rendering path
        List<Vector3> positionsList = new List<Vector3>();

        // list to store pos and color data per segment
        List<List<Vector3>> listOfPosList = new List<List<Vector3>>();
        List<List<Color32>> listOfColList = new List<List<Color32>>();

        // visualization y offset, 
        float yOffset = 0.12f;

        // variables to read and write data
        ReadWriteData wr = new ReadWriteData();
        string data = "";
        // Number of points.
        public int pointCount
        {
            get { return _pointData.Length; }
        }
        #endregion

        //bool isColourBufferFull = false;
        

        // Update is called once per frame
        void Update()
        {
            // deactivate partice visualization and collider
            //DeactivateParticleRendering(particle);

            switch (colourMode)
            {
                case ColouringMode.Read:
                    ReadDataFromFile();
                    SendDataToShader();
                    break;
                case ColouringMode.Write:
                    WriteDataToFile();
                    break;
                case ColouringMode.ClearAll:
                    ClearAllData();
                    break;
                case ColouringMode.SendToShader:
                    SendDataToShader();
                    //isColourBufferFull = true;
                    break;
                case ColouringMode.none:
                    UpdatePositions_Visualization();

                    // recognize mouse-obj collitions and update lists
                    if (Input.GetMouseButtonDown(0))
                        CheckMouseObjCollitions();
                    break;
            }

            if(/*isColourBufferFull && */startRenderingColor)
            {
                StartDynamicColouring();
            }


            colourMode = ColouringMode.none;
        }

        private void StartDynamicColouring()
        {
            int posIndex = OpenMPD_PresentationManager.Instance().currentPosIndex;
            filloutBuffers((int)(posIndex/step), windowSize);
            SendDataToShader();
            //isColourBufferFull = true;
        }

        private void filloutBuffers(int index, int range)
        {
            // clear the lists
            selectedPos.Clear();
            selectedCol.Clear();
            // initialize the arrays to use
            Vector3[] posArray = new Vector3[range];
            Color32[] colArray = new Color32[range];
            // copy the data window to be send to the shader
            int minIndex = mod((int)(index - (range / 2)), localPosList.Count-1);
            int maxIndex = mod((int)(index + (range / 2)), localPosList.Count-1);

            if (minIndex > maxIndex)
            {
                int newIndex = 0;
                int size = (localPosList.Count - 1) - minIndex;
                // copy from min index to total list size
                localPosList.CopyTo(minIndex, posArray, newIndex, size);
                localColList.CopyTo(minIndex, colArray, newIndex, size);
                newIndex = (localColList.Count - 1) - minIndex;
                // copy from 0 to maxIndex
                localPosList.CopyTo(0, posArray, newIndex, maxIndex); //localPosList.Count - minIndex
                localColList.CopyTo(0, colArray, newIndex, maxIndex);
            }
            else
            {
                localPosList.CopyTo(minIndex, posArray, 0, range);
                localColList.CopyTo(minIndex, colArray, 0, range);
            }

                // send the data back to a list
            selectedPos = new List<Vector3>(posArray);
            selectedCol = new List<Color32>(colArray);

            Color32 transpBlack = new Color32(0, 0, 0, 0);

            for (int i = 0; i < selectedCol.Count; i++)
            {
                if (selectedCol[i].Equals(transpBlack))
                {
                    selectedCol.RemoveAt(i);
                    selectedPos.RemoveAt(i);
                    i--;
                }
            }


        }

        private void UpdatePosColData()
        {
            //localPosList = selectedPos;
            //localColList = selectedCol;
            Color32[] colArray = new Color32[selectedCol.Count];
            Vector3[] posArray = new Vector3[selectedPos.Count];

            selectedPos.CopyTo(0, posArray, 0, selectedPos.Count);
            selectedCol.CopyTo(0, colArray, 0, selectedCol.Count);

            localPosList = new List<Vector3>(posArray);
            localColList = new List<Color32>(colArray);
            // check if the current position belong to the list of positions, if so, add the colour to the correspondent index //<<<<<<<<<<<<<<<<<<---------------------------------------------------------
            //for (int i=0; i < selectedPos.Count; i++){
            //    if (localPosList.Contains(selectedPos[i]))
            //        localColList[localPosList.IndexOf(selectedPos[i])] = selectedCol[i];
            //}   
        }

        private void InitializeBuffers(List<Vector3> pos)
        {
            localPosList.Clear();
            localColList.Clear();

            for(int i=0; i< pos.Count; i++)
            {
                localPosList.Add(pos[i]);
                localColList.Add(Color.black);
            }
        }

        private int mod(int x, int m)
        {
            if (x >= 0 && x < m)
                return x;
            else if (x < 0)
                return m + x;
            else
                return x - m;

            //int r = x % m;
            //return r < 0 ? r + m : r;
        }


        #region engine calls
        private void OnDestroy()
        {
            //shaderHandler.OnDisableLocal();

            if (_pointMaterial != null)
            {
                if (Application.isPlaying)
                {
                    Destroy(_pointMaterial);
                    Destroy(_diskMaterial);
                }
                else
                {
                    DestroyImmediate(_pointMaterial);
                    DestroyImmediate(_diskMaterial);
                }
            }
        }

        private void OnDisable()
        {
            //shaderHandler.OnDisableLocal();

            if (sourceBuffer != null)
            {
                sourceBuffer.Release();
                sourceBuffer = null;
            }
        }
               
        private void OnRenderObject()
        {
            // We need a source data or an externally given buffer.
            if (!isRenderBufferReady && sourceBuffer == null) return;

            // Check the camera condition.
            var camera = Camera.current;
            //if ((camera.cullingMask & (1 << gameObject.layer)) == 0) return;
            if (camera.name == "Preview Scene Camera") return;

            // TODO: Do view frustum culling here.

            // Lazy initialization
            if (_pointMaterial == null)
            {
                _pointMaterial = new Material(_pointShader);
                _pointMaterial.hideFlags = HideFlags.DontSave;
                _pointMaterial.EnableKeyword("_COMPUTE_BUFFER");

                _diskMaterial = new Material(_diskShader);
                _diskMaterial.hideFlags = HideFlags.DontSave;
                _diskMaterial.EnableKeyword("_COMPUTE_BUFFER");
            }
            float[] positions = new float[10000];
            ComputeBuffer compBuff = new ComputeBuffer(positions.Length / 4, sizeof(float) * 4);
            compBuff.SetData(positions);
            // Use the external buffer if given any.
            var pointBuffer = sourceBuffer;

            if (diskSize == 0)
            {
                _pointMaterial.SetPass(0);
                _pointMaterial.SetColor("_Tint", _pointTint);
                _pointMaterial.SetMatrix("_Transform", transform.localToWorldMatrix);
                _pointMaterial.SetBuffer("_PointBuffer", pointBuffer);
                Graphics.DrawProceduralNow(MeshTopology.Points, pointBuffer.count, 1);
            }
            else
            {
                _diskMaterial.SetPass(0);
                _diskMaterial.SetColor("_Tint", _pointTint);
                _diskMaterial.SetMatrix("_Transform", transform.localToWorldMatrix);
                _diskMaterial.SetBuffer("_PointBuffer", pointBuffer);
                _diskMaterial.SetFloat("_PointSize", diskSize);
                Graphics.DrawProceduralNow(MeshTopology.Points, pointBuffer.count, 1);
            }
        }

        private void OnPostRender()
        {
            // to draw lines using GL calls
            //shaderHandler.OnPostRenderLocal();
        }

        private void OnDrawGizmos()
        {
            // to draw lines using GL calls
            //shaderHandler.OnDrawGizmosLocal();
        }
        #endregion

        #region Utilities

        void SendDataToShader()
        {
            if(selectedPos.Count>0 && selectedCol.Count>0)
            {
                clearSpheresList(ref sphereslist);

                // initialize and the variables to pass to the shader
                Initialize(selectedPos, selectedCol);
                sourceBuffer = new ComputeBuffer(pointCount, elementSize);
                sourceBuffer.SetData(_pointData);
                isRenderBufferReady = true;

                // delete the spheres
                clearSpheresList(ref sphereslist);
            }
            else
            {
                ClearAllData();
            }
        }
        
        void ClearAllData()
        {
            if (sourceBuffer != null)
            {
                sourceBuffer.Release();
                sourceBuffer = null;
            }
            ClearSelectedList();
            // delete the spheres
            clearSpheresList(ref sphereslist);
            isRenderBufferReady = false;
            //isColourBufferFull = false;
        }

        bool WriteDataToFile()
        {
            if (localPosList.Count > 0 && localColList.Count > 0)// (selectedPos.Count > 0 && selectedCol.Count > 0)
            {
                //WriteConfurationData("ColourConfigData", selectedPos.Count.ToString() + "," + selectedCol.Count.ToString(),
                //    selectedPos, "Positions", selectedCol, "Colours");
                WriteConfurationData("ColourConfigData", localPosList.Count.ToString() + "," + localColList.Count.ToString(),
                   localPosList, "Positions", localColList, "Colours");
                return true;
            }
            else
                return false;
        }

        void ReadDataFromFile()
        {
#if UNITY_EDITOR
            data = wr.ReadColorDataFromFile(fileName);
            GetPosColFromData(data, ref selectedPos, ref selectedCol);
            UpdatePosColData();
#endif
        }

        void UpdatePositions_Visualization()
        {
            if (CheckPrimitiveDescriptorChanges())
                isConfigured = false;

            if (!isConfigured)
            {
                // clear previpus visualization state
                clearSpheresList(ref sphereslist);
                // get the positions from the current descriptor
                GetPositionFromPrimitive(ref positions);

                // generate a dictionary to store the downsampled positons
                // downsample positions
                DownSampleArray(positions, ref positionsList, step);

                // initialize the values for position and colour buffers                                                          //<<<<<<<<<<<<<<<<<<---------------------------------------------------------
                InitializeBuffers(positionsList);
                //downLenght = positionsList.Count;

                //SpawnColoredSpheres(positionsList, baseColor, sphereSize, ref sphereslist);
                isConfigured = true;
                startRenderingColor = false;
            }
        }

        void WriteConfurationData(string title, string header, List<Vector3> pos, string mPos, List<Color32> col,string mCol )
        {
#if UNITY_EDITOR
            wr.AddStringToDataToPrint(header);
            wr.AddListVec3ToDataToPrint(pos, pos.Count, mPos);
            wr.AddListColorToDataToPrint(col, col.Count, mCol);
            wr.WriteDataToFile(title);
#endif
        }
       
        void CheckMouseObjCollitions()
        {
            // define the ray properties
            Ray ray = Camera.main.ScreenPointToRay(Input.mousePosition);
            RaycastHit hit;

            //  check out for any collition
            if (Physics.Raycast(ray, out hit))
            {
                // get collitions pos
                Vector3 touchedPos = hit.collider.transform.position;
                if (positionsList.Contains(touchedPos))
                {
                    // update object colour base on shaderColour
                    hit.collider.transform.GetComponent<Renderer>().material.SetColor("_Color", shaderColor);

                    //<<< chec if the selected sphere is already in the list
                    if (!selectedPos.Contains(touchedPos))
                        AddToSelectedLists(touchedPos, shaderColor);

                    // check if the current position belong to the list of positions, if so, add the colour to the correspondent index //<<<<<<<<<<<<<<<<<<---------------------------------------------------------
                    if (localPosList.Contains(touchedPos))
                        localColList[localPosList.IndexOf(touchedPos)] = shaderColor;
                }
            }
        }

        void AddToSelectedLists(Vector3 pos, Color col)
        {
            selectedPos.Add(pos);
            selectedCol.Add(col);
        }

        void DownSampleArray(float[] inputArray, ref List<Vector3> posList, int step)
        {
            posList.Clear();
            List<Vector3> poslocal = new List<Vector3>();
            // get the full lsit of positions
            GetListFromArray(inputArray, ref poslocal);

            // down sample the list
            for (int i = 0; i < poslocal.Count; i += step)
            {
                Vector3 newPos = poslocal[i];
                if (!posList.Contains(new Vector3(newPos.x, newPos.y, newPos.z))) 
                    posList.Add(newPos);
            }
        }

        private void GetListFromArray(float[] arr, ref List<Vector3> pos)
        {
            for (int i = 0; i < arr.Length; i = (i + 4))
            {
                pos.Add(new Vector3(arr[i], (arr[i + 1]*-1) + yOffset, arr[i + 2] * -1));
            }
        }

        private void GetPosColFromData(string datain, ref List<Vector3> pos, ref List<Color32> col)
        {
            pos.Clear();
            col.Clear();
            List<Vector3> localpos = new List<Vector3>();
            List<Color32> localcol = new List<Color32>();
            string[] lines = datain.Split("\n"[0]);
            
            string[] lineData;
            lineData = (lines[0].Trim()).Split(","[0]);
            
            int posNum = 0, colNum = 0; 
            int.TryParse(lineData[0], out posNum);
            int.TryParse(lineData[1], out colNum);
           

            for (int i=1; i < lines.Length; i++)
            {
                lineData = (lines[i].Trim()).Split(","[0]);
                
                if (lineData.Length == 4)
                {
                    float xx = 0, yy = 0, zz = 0;
                    float.TryParse(lineData[0], out xx);
                    float.TryParse(lineData[1], out yy);
                    float.TryParse(lineData[2], out zz);
                    Vector3 aux = new Vector3(xx, yy, zz);
                    localpos.Add(aux);
                }
                else if (lineData.Length == 5)
                {
                    byte rr = 0, gg = 0, bb = 0, aa = 0;
                    byte.TryParse(lineData[0], out rr);
                    byte.TryParse(lineData[1], out gg);
                    byte.TryParse(lineData[2], out bb);
                    byte.TryParse(lineData[2], out aa);
                    Color32 aux = new Color32(rr, gg, bb, aa);
                    localcol.Add(aux);
                }
                else
                    Debug.Log("Reader: <<<<< Error size >>>>>");

            }
            Debug.Log("Reader: <<<<< Pos & Col Lists Uploaded Successfully>>>>>");
            // send the data back to a list
            pos = new List<Vector3>(localpos);
            col = new List<Color32>(localcol);

            Color32 transpBlack = new Color32(0, 0, 0, 0);

            for (int i = 0; i < selectedCol.Count; i++)
            {
                if (col[i].Equals(transpBlack))
                {
                    col.RemoveAt(i);
                    pos.RemoveAt(i);
                    i--;
                }
            }
        }

        private void SpawnColoredSpheres(List<Vector3> localPositions, Color color, float size, ref List<GameObject> sList)
        {
            for (int i = 0; i < localPositions.Count; i++)
            {
                CreateSphereAt(localPositions[i], color, size, ref sList);
            }
        }

        private static void CreateSphereAt(Vector3 point, Color color, float size, ref List<GameObject> sList)
        {
            GameObject sphere = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            sphere.transform.position = point;
            sphere.transform.localScale = new Vector3(size, size, size);

            // assigning hte new color to the sphere
            Material curMat = sphere.GetComponent<Renderer>().material;
            Material newMat = new Material(curMat);
            newMat.SetColor("_Color", color);
            sphere.GetComponent<Renderer>().material = newMat;
            sList.Add(sphere);
        }

        private void clearSpheresList(ref List<GameObject> spheres)
        {
            if (spheres.Count > 0)
            {
                for (int i = spheres.Count - 1; i >= 0; i--)
                {
                    Destroy(spheres[i]);
                }
            }

            spheres.Clear();
        }

        private void ClearSelectedList()
        {
            selectedPos.Clear();
            selectedCol.Clear();
        }

        private bool CheckPrimitiveDescriptorChanges()
        {
            bool state = false;
            posDescID = GetPosDescriptorID();

            if (posDescID != 0)
            {
                if (posDescID != prevID || step != prevStep)
                {
                    state = true;
                    prevID = posDescID;
                    prevStep = step;
                }
            }
            return state;
        }

        void GetPositionFromPrimitive(ref float[] positions)
        {
            Primitive primID = particle.GetComponent<Primitive>();

            //read positions from descriptor
            positions = (OpenMPD_ContextManager .Instance().GetPositionsDescriptor(primID.GetPrimitiveID())).positions;
            //arrayLenght = positions.Length / 4;
            posDescID = primID.GetPositionsDescriptorID();
        }

        uint GetPosDescriptorID()
        {
            Primitive primID = particle.GetComponent<Primitive>();            
            return primID.GetPositionsDescriptorID();
        }

        void DeactivateParticleRendering(GameObject obj)
        {
            obj.GetComponentInChildren<SphereCollider>().enabled = false;
            obj.GetComponentInChildren<MeshRenderer>().enabled = false;
        }
        #endregion

        #region utilitites Shader
        private void RenderLines(List<Vector3> samp, List<Color32> col)
        {
            if (!ValidateInput(samp, col))
                return;

            GL.Begin(GL.LINES);
            lineMat.SetPass(0);
            for (int i = 0; i < samp.Count - 1; i++)
            {
                GL.Color(col[i]);
                GL.Vertex(samp[i]);
                GL.Color(col[i + 1]);
                GL.Vertex(samp[i + 1]);
            }
            GL.End();
        }

        private bool ValidateInput(List<Vector3> pos, List<Color32> col)
        {
            return (pos != null && col != null && pos.Count == col.Count);
        }

        static uint EncodeColor(Color c)
        {
            const float kMaxBrightness = 16;

            var y = Mathf.Max(Mathf.Max(c.r, c.g), c.b);
            y = Mathf.Clamp(Mathf.Ceil(y * 255 / kMaxBrightness), 1, 255);

            var rgb = new Vector3(c.r, c.g, c.b);
            rgb *= 255 * 255 / (y * kMaxBrightness);

            return ((uint)rgb.x) |
                   ((uint)rgb.y << 8) |
                   ((uint)rgb.z << 16) |
                   ((uint)y << 24);
        }

        private void Initialize(List<Vector3> positions, List<Color32> colors)
        {
            _pointData = new Point[positions.Count];
            for (var i = 0; i < _pointData.Length; i++)
            {
                _pointData[i] = new Point
                {
                    position = positions[i],
                    color = EncodeColor(colors[i])
                };
            }
        }

        // Use this for initialization
        public List<Vector3> GetCirclePosArray(int samples, float maxRandHeight, float height, float radius)
        {
            //Determine how many smaples per second we will need:
            List<Vector3> positions = new List<Vector3>();

            //Fill circle: 
            for (int s = 0; s < samples; s++)
            {
                float angle = 2 * (Mathf.PI * s) / samples;
                positions.Add(new Vector3(radius * Mathf.Cos(angle), UnityEngine.Random.Range(0f, maxRandHeight) + height, radius * Mathf.Sin(angle)));
            }

            return positions;
        }

        public List<Color32> GetRandColorArray(int samples)
        {
            //Determine how many smaples per second we will need:
            List<Color32> colors = new List<Color32>();

            //Fill circle: 
            for (int s = 0; s < samples; s++)
            {
                int r = UnityEngine.Random.Range(0, 255), g = UnityEngine.Random.Range(0, 255), b = UnityEngine.Random.Range(0, 255), a = 255;
                colors.Add(new Color32((byte)r, (byte)g, (byte)g, (byte)a));
            }

            return colors;
        }

        #endregion

        #region to delete
        bool ConfigureShader(Shader pointS, Shader diskS, Material lineM, Color tint, float diskSSize)
        {
            //shaderHandler._pointShader = pointS;
            //shaderHandler._diskShader = diskS;
            //shaderHandler.lineMat = lineM;
            //shaderHandler._pointTint = tint;
            //shaderHandler.diskSize = diskSSize;
            return true;
        }

        void UpdateShader(List<Vector3> sPos, List<Color32> sCol)
        {
            //shaderHandler.UpdateLocal(sPos, sCol);
        }

        bool GetIndexFromTouch(Vector3 pos, Dictionary<Vector3, int> dic, ref int index)
        {
            bool status = dic.TryGetValue(pos, out index);
            if (!status)
                Debug.LogError("The Dict doesn't have the value the key");
            return status;
        }

        bool isInDictionary(Vector3 pos, Dictionary<Vector3, int> dic)
        {
            int index;
            bool status = dic.TryGetValue(pos, out index);
            if (!status)
                Debug.LogError("The Dict doesn't have the value the key");
            return status;
        }


        private void AddToShaderList(List<Vector3> posIn, List<Color32> colIn, ref List<Vector3> posOut, ref List<Color32> colOut)
        {
            // store the current selected list (pos and colours)
            List<Vector3> temporalPos = new List<Vector3>(posOut);
            posOut.AddRange(temporalPos);

            List<Color32> temporalCol = new List<Color32>(colOut);
            colOut.AddRange(temporalCol);

            //shaderArrayLengh = posOut.Count;
        }

        private void removeSelectedFromMainlist(ref List<Vector3> mainList, List<Vector3> secondaryList)
        {
            for (int i = 0; i < selectedPos.Count; i++)
            {
                if (mainList.Contains(secondaryList[i]))
                    mainList.Remove(secondaryList[i]);
            }
        }

        private void ConcatenateArrays(List<List<Vector3>> posListIn, List<List<Color32>> colListIn, ref List<Vector3> posListOut, ref List<Color32> colListOut)
        {
            for (int i = 0; i < posListIn.Count; i++)
            {
                posListOut.AddRange(posListIn[i]);
                colListOut.AddRange(colListIn[i]);
            }
            //shaderArrayLengh = posListOut.Count;
        }
        private void ClearShaderList()
        {
            listOfColList.Clear();
            listOfPosList.Clear();
        }
        #endregion
    }
}