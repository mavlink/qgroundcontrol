/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Item {
    height:                 calCol.height + ScreenTools.defaultFontPixelHeight * 2
    width:                  calCol.width  + ScreenTools.defaultFontPixelWidth  * 2
    Column {
        id:                 calCol
        spacing:            ScreenTools.defaultFontPixelHeight
        anchors.centerIn:   parent
        Item {
            height:         1
            width:          1
        }
        Row {
            spacing:            ScreenTools.defaultFontPixelWidth * 4
            anchors.horizontalCenter: parent.horizontalCenter
            //-----------------------------------------------------------------
            // Calibration
            Column {
                spacing:            ScreenTools.defaultFontPixelHeight
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    width:          Math.round(ScreenTools.defaultFontPixelWidth * 45)
                    height:         Math.round(width * 0.5)
                    radius:         ScreenTools.defaultFontPixelWidth * 2
                    color:          qgcPal.window
                    border.color:   qgcPal.text
                    border.width:   ScreenTools.defaultFontPixelWidth * 0.25
                    anchors.horizontalCenter: parent.horizontalCenter
                    property bool hasStickPositions: controller.stickPositions.length === 4
                    //---------------------------------------------------------
                    //-- Left Stick
                    Rectangle {
                        width:      parent.width * 0.25
                        height:     width
                        radius:     width * 0.5
                        color:      qgcPal.window
                        border.color: qgcPal.text
                        border.width: ScreenTools.defaultFontPixelWidth * 0.125
                        x:          (parent.width  * 0.25) - (width  * 0.5)
                        y:          (parent.height * 0.5)  - (height * 0.5)
                    }
                    Rectangle {
                        color:  qgcPal.colorGreen
                        width:  parent.width * 0.035
                        height: width
                        radius: width * 0.5
                        visible: parent.hasStickPositions
                        x:      (parent.width  * controller.stickPositions[0]) - (width  * 0.5)
                        y:      (parent.height * controller.stickPositions[1]) - (height * 0.5)
                    }
                    //---------------------------------------------------------
                    //-- Right Stick
                    Rectangle {
                        width:      parent.width * 0.25
                        height:     width
                        radius:     width * 0.5
                        color:      qgcPal.window
                        border.color: qgcPal.text
                        border.width: ScreenTools.defaultFontPixelWidth * 0.125
                        x:          (parent.width  * 0.75) - (width  * 0.5)
                        y:          (parent.height * 0.5)  - (height * 0.5)
                    }
                    Rectangle {
                        color:  qgcPal.colorGreen
                        width:  parent.width * 0.035
                        height: width
                        radius: width * 0.5
                        visible: parent.hasStickPositions
                        x:      (parent.width  * controller.stickPositions[2]) - (width  * 0.5)
                        y:      (parent.height * controller.stickPositions[3]) - (height * 0.5)
                    }
                }
            }
            //---------------------------------------------------------------------
            // Monitor
            Column {
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.verticalCenter: parent.verticalCenter
                Connections {
                    target: controller
                    onAxisValueChanged: {
                        if (axisMonitorRepeater.itemAt(axis)) {
                            axisMonitorRepeater.itemAt(axis).axis.axisValue = value
                        }
                    }
                    onAxisDeadbandChanged: {
                        if (axisMonitorRepeater.itemAt(axis)) {
                            axisMonitorRepeater.itemAt(axis).axis.deadbandValue = value
                        }
                    }
                }
                Repeater {
                    id:             axisMonitorRepeater
                    model:          _activeJoystick ? _activeJoystick.axisCount : 0
                    width:          parent.width
                    Row {
                        spacing:    5
                        anchors.horizontalCenter: parent.horizontalCenter
                        // Need this to get to loader from Connections above
                        property Item axis: theAxis
                        QGCLabel {
                            id:     axisLabel
                            text:   modelData
                        }
                        AxisMonitor {
                            id:                     theAxis
                            anchors.verticalCenter: axisLabel.verticalCenter
                            height:                 ScreenTools.defaultFontPixelHeight
                            width:                  200
                            narrowIndicator:        true
                            mapped:                 true
                            reversed:               false
                            MouseArea {
                                id:                 deadbandMouseArea
                                anchors.fill:       parent.item
                                enabled:            controller.deadbandToggle
                                preventStealing:    true
                                property real startX
                                property real direction
                                onPressed: {
                                    startX = mouseX
                                    direction = startX > width/2 ? 1 : -1
                                    parent.item.deadbandColor = "#3C6315"
                                }
                                onPositionChanged: {
                                    var mouseToDeadband = 32768/(width/2) // Factor to have deadband follow the mouse movement
                                    var newValue = parent.item.deadbandValue + direction*(mouseX - startX)*mouseToDeadband
                                    if ((newValue > 0) && (newValue <32768)){parent.item.deadbandValue=newValue;}
                                    startX = mouseX
                                }
                                onReleased: {
                                    controller.setDeadbandValue(modelData,parent.item.deadbandValue)
                                    parent.item.deadbandColor = "#8c161a"
                                }
                            }
                        }
                    }
                }
            }
        }
        // Command Buttons
        Row {
            spacing:            ScreenTools.defaultFontPixelWidth * 2
            visible:            _activeJoystick.requiresCalibration
            anchors.horizontalCenter: parent.horizontalCenter
            QGCButton {
                id:         skipButton
                text:       qsTr("Skip")
                enabled:    controller.calibrating ? controller.skipEnabled : false
                width:      ScreenTools.defaultFontPixelWidth * 10
                onClicked:  controller.skipButtonClicked()
            }
            QGCButton {
                text:       qsTr("Cancel")
                width:      ScreenTools.defaultFontPixelWidth * 10
                enabled:    controller.calibrating
                onClicked: {
                    if(controller.calibrating)
                        controller.cancelButtonClicked()
                }
            }
            QGCButton {
                id:         nextButton
                primary:    true
                enabled:    controller.calibrating ? controller.nextEnabled : true
                text:       controller.calibrating ? qsTr("Next") : qsTr("Start")
                width:      ScreenTools.defaultFontPixelWidth * 10
                onClicked:  controller.nextButtonClicked()
            }
        }
        // Status Text
        QGCLabel {
            text:           controller.statusText
            width:          parent.width * 0.8
            wrapMode:       Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}


