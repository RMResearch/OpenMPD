using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_SPEED_CONTROLLERS_STRING + "Easing Speed Controller", AEConsts.MENU_OPENMPD_SPEED_CONTROLLERS_OFFSET + 1)]
public class EasingSpeedControllerNode : SpeedControllerNode
{
    [Input] public AnimationCurve easingFunc = AnimationCurve.Linear(0,0,1,1);
    [Range(0.00001f, float.MaxValue)]
    [Input] public float totalTimeS = 1;
    public override float[] GenerateSamplePercentages()
    {
        totalTimeS = GetInputValue<float>("totalTimeS", this.totalTimeS);
        float fps = 10000;
        if (OpenMPD_PresentationManager.Instance() != null) fps = OpenMPD_PresentationManager.Instance().ResultingFPS;
        int samplePointCount = Mathf.Max(Mathf.RoundToInt(totalTimeS * fps), 1);
        float[] samplePercentages = new float[samplePointCount];
        for (int i = 0; i < samplePointCount; i++){
            samplePercentages[i] = easingFunc.Evaluate((float)i /samplePointCount);
        }
        return samplePercentages;
    }

    protected override void OnDirtyUpdate(){
        totalTimeS = GetInputValue<float>("totalTimeS", this.totalTimeS);
    }

    // Use this for initialization
    protected override void Init() {
		base.Init();
	}
}