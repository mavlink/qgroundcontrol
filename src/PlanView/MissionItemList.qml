import QtQuick

import QGroundControl
import QGroundControl.Controls

/// List of the mission's visual items. Selecting a row makes it the current plan view item.
QGCListView {
    id: root

    required property var missionController

    signal itemActivated

    objectName:     "planPanel_missionList"
    model:          missionController.visualItems
    spacing:        ScreenTools.defaultFontPixelHeight * 0.1
    clip:           true
    currentIndex:   missionController.currentPlanViewVIIndex

    onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

    delegate: MissionItemRow {
        required property var model

        width:  root.width
        object: model.object

        onClicked: {
            if (!mainWindow.allowViewSwitch()) {
                return
            }
            root.missionController.setCurrentPlanViewSeqNum(object.sequenceNumber, false)
            root.itemActivated()
        }
    }
}
