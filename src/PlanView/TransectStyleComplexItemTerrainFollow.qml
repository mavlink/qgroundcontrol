import QtQuick                      2.3
import QtQuick.Controls             1.2
import QtQuick.Layouts              1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

ColumnLayout {
    anchors.left:   parent.left
    anchors.right:  parent.right
    spacing:        _margin
    visible:        tabBar.currentIndex === 2

    property var    missionItem

    QGCCheckBox {
        id:         followsTerrainCheckBox
        text:       qsTr("Vehicle follows terrain")
        checked:    missionItem.followTerrain
        onClicked:  missionItem.followTerrain = checked

        Binding on checkedState {
            value: missionItem.followTerrain ? Qt.Checked : Qt.Unchecked
        }
    }

    GridLayout {
        Layout.fillWidth:   true
        columnSpacing:      _margin
        rowSpacing:         _margin
        columns:            2
        enabled:            followsTerrainCheckBox.checked

        QGCLabel { text: qsTr("Tolerance") }
        FactTextField {
            fact:               missionItem.terrainAdjustTolerance
            Layout.fillWidth:   true
        }

        QGCLabel { text: qsTr("Max Climb Rate") }
        FactTextField {
            fact:               missionItem.terrainAdjustMaxClimbRate
            Layout.fillWidth:   true
        }

        QGCLabel { text: qsTr("Max Descent Rate") }
        FactTextField {
            fact:               missionItem.terrainAdjustMaxDescentRate
            Layout.fillWidth:   true
        }
    }
}
