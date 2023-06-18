using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

[CreateNodeMenu(AEConsts.MENU_OPENMPD_PATH_UTILITIES_STRING + "Path Merger", AEConsts.MENU_OPENMPD_PATH_UTILITIES_OFFSET + (int)PathUtilities.PathMerger)]
// This node can be used to append two paths.
// This is done by either scaling them both linearly (each path completes in full, but faster),
// or by truncating the first after a certain amount of progress and continuting the second from part-way in
public class PathMergerNode : PathNode {

    [Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public PathNode firstPath = null;
	[Input(ShowBackingValue.Never, ConnectionType.Override, TypeConstraint.Inherited)] public PathNode secondPath = null;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public float percentage = 0.5f;
    [Input(ShowBackingValue.Unconnected, ConnectionType.Override, TypeConstraint.Inherited)] public bool scalePaths = true;

    public override Vector3 getPositionAtPercent(float p)
    {
        // Split the position requests based on percentage parameter
        // if scalePaths is true, sub-paths receive 0-100%
        // if not, then the paths are appended based on percentage parameter
        firstPath = GetInputValue<PathNode>("firstPath", this.firstPath);
        secondPath = GetInputValue<PathNode>("secondPath", this.secondPath);
        percentage = GetInputValue<float>("percentage", this.percentage);
        scalePaths = GetInputValue<bool>("scalePaths", this.scalePaths);

        if (p < percentage){
            float adjustedP = p;
            if(scalePaths) adjustedP = p/percentage;

            if(firstPath != null){
                return firstPath.getPositionAtPercent(adjustedP);
            }else{
                return Vector3.zero;
            }
        }else{
            float adjustedP = p;
            if(scalePaths) adjustedP = (p-percentage)/(1-percentage);

            if(secondPath != null){
                return secondPath.getPositionAtPercent(adjustedP);
            }else{
                return Vector3.zero;
            }
        }
        //return Vector3.zero;
    }

    protected override void OnDirtyUpdate(){
        firstPath = GetInputValue<PathNode>("firstPath", this.firstPath);
        secondPath = GetInputValue<PathNode>("secondPath", this.secondPath);
        percentage = GetInputValue<float>("percentage", this.percentage);
        scalePaths = GetInputValue<bool>("scalePaths", this.scalePaths);
    }}