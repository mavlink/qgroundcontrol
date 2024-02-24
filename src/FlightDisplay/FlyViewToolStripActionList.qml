/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQml.Models

import QGroundControl
import QGroundControl.Controls

ToolStripActionList {
    id: _root

    signal displayPreFlightChecklist
    property bool   _viewer3DEnabled:        QGroundControl.settingsManager.viewer3DSettings.enabled.rawValue

    model: [
        ToolStripAction {
            text:           qsTr("Plan")
            iconSource:     "/qmlimages/Plan.svg"
            onTriggered:{
                mainWindow.showPlanView()
                mapIcon.showFlyMap()
            }
        },
        ToolStripAction {
            property bool _is3DViewOpen: viewer3DWindow.isOpen

            id: mapIcon
            visible: _viewer3DEnabled
            text:           qsTr("3D View")
            iconSource:     "/qmlimages/Viewer3D/City3DMapIcon.svg"
            onTriggered:{
                if(_is3DViewOpen === false){
                    viewer3DWindow.open()
                }else{
                    viewer3DWindow.close()
                }
            }

            on_Is3DViewOpenChanged: {
                if(_is3DViewOpen === true){
                    mapIcon.iconSource =     "/qmlimages/PaperPlane.svg"
                    text=           qsTr("Fly")
                }else{
                    viewer3DWindow.close()
                    iconSource =     "/qmlimages/Viewer3D/City3DMapIcon.svg"
                    text =           qsTr("3D View")
                }
            }
        },
        PreFlightCheckListShowAction { onTriggered: displayPreFlightChecklist() },
        GuidedActionTakeoff { },
        GuidedActionLand { },
        GuidedActionRTL { },
        GuidedActionPause { },
        GuidedActionActionList { },
        GuidedActionGripper { }
    ]
}
