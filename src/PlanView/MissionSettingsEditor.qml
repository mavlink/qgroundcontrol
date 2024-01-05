import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
import QGroundControl.Controls
import QGroundControl.FactControls

// Editor for Mission Settings
Rectangle {
    id:                 valuesRect
    width:              availableWidth
    height:             valuesColumn.height + (_margin * 2)
    color:              QGroundControl.globalPalette.windowShadeDark
    visible:            missionItem.isCurrentItem
    radius:             _radius

    property var    _masterControler:   masterController
    property var    _missionController: _masterControler.missionController

    readonly property real _margin: ScreenTools.defaultFontPixelWidth / 2

    ColumnLayout {
        id:                 valuesColumn
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        spacing:            _margin

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth

            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("Altitude")
            }
            FactTextField {
                Layout.fillWidth:   true
                fact:               missionItem.plannedHomePositionAltitude
            }
        }

        QGCLabel {
            Layout.fillWidth:       true
            wrapMode:               Text.WordWrap
            font.pointSize:         ScreenTools.smallFontPointSize
            text:                   qsTr("Actual position set by vehicle at flight time.")
            horizontalAlignment:    Text.AlignHCenter
        }

        QGCButton {
            Layout.alignment:   Qt.AlignHCenter
            text:               qsTr("Set To Map Center")
            onClicked:          missionItem.coordinate = map.center
        }
    }
}
