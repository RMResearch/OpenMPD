using UnityEngine;
using System.Collections;

/**
    This class contains the high-level desription of the contents to render in a given frame.
    This includes the contentIds and the matirces they should use. 
    Please note that low-level aspects (local position and amplitudes per point/content) have
    been defined into OpenMPD_PresentationManager. 
*/
public class OpenMPD_RenderingUpdate
{
    protected const int maxPrimitives=32;//Get this from OpenMPD_PresentationManager.
    protected uint currentPrimitive;               
    public readonly uint[] primitiveIds;
    public readonly float[] matricesStart;
    public readonly float[] matricesEnd;
    public OpenMPD_RenderingUpdate() {
        currentPrimitive = 0;
        primitiveIds = new uint[maxPrimitives];
        matricesStart = new float [16* maxPrimitives];
        matricesEnd = new float[16* maxPrimitives];
    }
    public void UpdatePrimitive(uint primitiveId, Matrix4x4 mStart, Matrix4x4 mEnd) {
        //Add update for this content
        primitiveIds[currentPrimitive] = primitiveId;
        Matrix4x4 mStartRowMajor = mStart.transpose;
        Matrix4x4 mEndRowMajor = mEnd.transpose;
        
        for (int i = 0; i < 16; i++)
        {
            matricesStart[16 * currentPrimitive + i] = mStartRowMajor[i];
            matricesEnd[16 * currentPrimitive + i] = mEndRowMajor[i];
            //matricesStart[16 * currentPrimitive + i] = mStart[i];
            //matricesEnd[16 * currentPrimitive + i] = mEnd[i];
        }
        //Point to next Position
        currentPrimitive++;
    }

    public uint NumPrimitives() {
        return currentPrimitive;
    }
}
