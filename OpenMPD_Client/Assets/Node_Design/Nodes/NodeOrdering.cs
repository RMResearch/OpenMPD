using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XNode;

// AnimationEditorConstants
// This class holds consts for easy adjustment of the graph editor context menu (right click)
// The strings stored here are referenced by each node within that folder
// And the offset values determine the order of the listed items.
class AEConsts{
    public const int MENU_OFFSET_ALL = 0;
    public const string MENU_OPENMPD_STRING = "OpenMPD Objects/";
    public const int MENU_OPENMPD_OFFSET = MENU_OFFSET_ALL + 0;
    public const string MENU_OPENMPD_PATHS_STRING = "Paths/";
    public const int MENU_OPENMPD_PATHS_OFFSET = MENU_OFFSET_ALL + 10;
    public const string MENU_OPENMPD_PATH_UTILITIES_STRING = "Path Utilities/";
    public const int MENU_OPENMPD_PATH_UTILITIES_OFFSET = MENU_OFFSET_ALL + 20;
    public const string MENU_OPENMPD_SPEED_CONTROLLERS_STRING = "Speed Controllers/";
    public const int MENU_OPENMPD_SPEED_CONTROLLERS_OFFSET = MENU_OFFSET_ALL + 30;

    public const string MENU_PRIMITIVES_STRING = "Literals/";
    public const int MENU_PRIMITIVES_OFFSET = MENU_OFFSET_ALL + 100;
    public const string MENU_UTILITIES_STRING = "Utilities/";
    public const int MENU_UTILITIES_OFFSET = MENU_OFFSET_ALL + 200;

}

enum PathTypes{
    FixedPosition = 0,
    LinePath,
    CirclePath,
    ArcPath,
    OvalPath,
    CSVPath
}

enum PathUtilities{
    PositionAtPercentage = 0,
    PathMerger,
    PathRotatorNode
}