/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.12
import QtQuick.Layouts              1.12

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

Rectangle {
    height:     mainLayout.height + (_margins * 2)
    visible:    false
    color:      qgcPal.window

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property real   _margins:       ScreenTools.defaultFontPixelWidth / 2
    property real   _totalBlocks:   _activeVehicle ? _activeVehicle.terrain.blocksPending.rawValue + _activeVehicle.terrain.blocksLoaded.rawValue : 0
    property real   _blocksLoaded:  _activeVehicle ? _activeVehicle.terrain.blocksLoaded.rawValue : 0
    property real   _blocksPending: _activeVehicle ? _activeVehicle.terrain.blocksPending.rawValue : 0
    property real   _pctComplete:   _activeVehicle && _totalBlocks ? _blocksLoaded / _totalBlocks : 0

    on_BlocksPendingChanged: {
        if (_blocksPending == 0) {
            // UI doesn't go away immediately
            visibilityTimer.restart()
        } else {
            visible = true
            visibilityTimer.stop()
        }
    }

    on_BlocksLoadedChanged: {
        if (_blocksLoaded != 0) {
            // This causes the progress indicator to display even if it starts out as complete
            visible = true
            if (_blocksPending == 0) {
                visibilityTimer.restart()
            }
        }
    }

    Timer {
        id:             visibilityTimer
        interval:       30 * 1000
        onTriggered:    parent.visible = false
    }

    QGCPalette { id: qgcPal }

    ColumnLayout {
        id:                 mainLayout
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margins

        QGCLabel {
            Layout.alignment:   Qt.AlignHCenter
            text:               qsTr("Terrain Load Progress")
            font.pointSize:     ScreenTools.smallFontPointSize
        }

        Rectangle {
            Layout.fillWidth:   true
            height:             ScreenTools.defaultFontPixelHeight
            color:              "transparent"
            border.color:       "green"

            Rectangle {
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                color:          "green"
                width:         parent.width * _pctComplete

                QGCLabel {
                    anchors.centerIn:   parent
                    text:               qsTr("Done")
                    visible:            _blocksPending == 0
                }
            }
        }
    }
}
