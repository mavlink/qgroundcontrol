/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models

import QGroundControl

import QGroundControl.Controls

import QGroundControl.FlightDisplay
import QGroundControl.FlightMap




// To implement a custom overlay copy this code to your own control in your custom code source. Then override the
// FlyViewCustomLayer.qml resource with your own qml. See the custom example and documentation for details.
Item {
    id: _root

    property var parentToolInsets               // These insets tell you what screen real estate is available for positioning the controls in your overlay
    property var totalToolInsets:   _toolInsets // These are the insets for your custom overlay additions
    property var mapControl

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle

    // Path overlay toggle controls
    Rectangle {
        id:                     pathControlPanel
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.margins:        ScreenTools.defaultFontPixelWidth
        width:                  pathControlColumn.width + ScreenTools.defaultFontPixelWidth * 2
        height:                 pathControlColumn.height + ScreenTools.defaultFontPixelWidth * 2
        radius:                 ScreenTools.defaultFontPixelWidth * 0.5
        color:                  Qt.rgba(0, 0, 0, 0.7)
        visible:                _activeVehicle

        Column {
            id:                 pathControlColumn
            anchors.centerIn:   parent
            spacing:            ScreenTools.defaultFontPixelWidth * 0.5

            QGCLabel {
                text:               qsTr("Path Overlays")
                color:              "white"
                font.pointSize:     ScreenTools.smallFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }

            QGCCheckBox {
                text:               qsTr("GPS Path")
                checked:            _activeVehicle ? _activeVehicle.gpsPathPoints.enabled : false
                onClicked:          if (_activeVehicle) _activeVehicle.gpsPathPoints.enabled = checked
                
                Rectangle {
                    width:          ScreenTools.defaultFontPixelWidth * 1.5
                    height:         ScreenTools.defaultFontPixelWidth * 0.5
                    color:          "#00E04B"
                    anchors.left:   parent.right
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            QGCCheckBox {
                text:               qsTr("Odometry Path")
                checked:            _activeVehicle ? _activeVehicle.odometryPathPoints.enabled : false
                onClicked:          if (_activeVehicle) _activeVehicle.odometryPathPoints.enabled = checked
                
                Rectangle {
                    width:          ScreenTools.defaultFontPixelWidth * 1.5
                    height:         ScreenTools.defaultFontPixelWidth * 0.5
                    color:          "#536DFF"
                    anchors.left:   parent.right
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    // since this file is a placeholder for the custom layer in a standard build, we will just pass through the parent insets
    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset + (pathControlPanel.visible ? pathControlPanel.width + ScreenTools.defaultFontPixelWidth * 2 : 0)
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset + (pathControlPanel.visible ? pathControlPanel.height + ScreenTools.defaultFontPixelWidth * 2 : 0)
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }
}
