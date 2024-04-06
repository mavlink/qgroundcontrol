/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 2.15
import QtQuick.Layouts  1.15
import QtQuick.Dialogs  1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    id:                 metFlightData
    color:              qgcPal.window
    radius:             ScreenTools.defaultFontPixelWidth / 2

    property real _toolsMargin:           ScreenTools.defaultFontPixelWidth
    property real _altMsgMinWidth:        ScreenTools.defaultFontPixelWidth * 10
    property bool _fileNameTouched:       false
    property real _fontSize:              ScreenTools.defaultFontPointSize
    property real _smallFontSize:         ScreenTools.defaultFontPointSize * 0.8
    MetFlightDataRecorderController { id: controller; }

    // make goToFile function available outside of this file
    function goToFile() {
        controller.goToFile()
    }

    Text {
        id: flightLabel
        text: "Flight:"
        font.pointSize: _fontSize
        color: qgcPal.text
        anchors.verticalCenter: flightInput.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: _toolsMargin
    }

    QGCTextField {
        id: flightInput
        anchors.top: parent.top
        anchors.left: flightLabel.right
        anchors.leftMargin: _toolsMargin
        anchors.topMargin: _toolsMargin
        width: 400
        showHelp: false
        placeholderText:  qsTr("Enter Flight Name")
        text: controller.flightFileName
        onTextChanged: {
            controller.flightFileName = text
            _fileNameTouched = true
        }
    }

    Text {
        id: flightNameError

        text: qsTr("Invalid Flight Name")
        color: qgcPal.colorRed
        visible: _fileNameTouched && !controller.flightNameValid
        font.pointSize: _fontSize
        anchors.verticalCenter: flightInput.verticalCenter
        anchors.left: flightInput.right

        anchors.leftMargin: _toolsMargin
        anchors.topMargin: _toolsMargin
    }

    // Ascent Lable
    Text {
        id: ascentLabel
        text: `${qsTr("Ascent")}: ${controller.ascentNumber}`
        font.pointSize: _fontSize
        color: qgcPal.text
        visible: controller.ascentNumber > 0
        anchors.verticalCenter: flightInput.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: _toolsMargin
        anchors.topMargin: _toolsMargin
    }

    // altitude message data grid
    Rectangle {
        color: qgcPal.windowShade
        anchors.top: flightInput.bottom
        anchors.left: parent.left
        anchors.topMargin: _toolsMargin * 2
        anchors.bottom: parent.bottom
        width: parent.width
        Layout.fillWidth: true

        GridLayout {
            id: almHeaders
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: _toolsMargin
            anchors.topMargin: _toolsMargin
            width: parent.width
            columns: 7
            rowSpacing: _toolsMargin
            columnSpacing: _toolsMargin

            QGCLabel {
                text: qsTr("Alt\n(m)")
                font.pointSize: _fontSize
                color: qgcPal.text
                Layout.minimumWidth: _altMsgMinWidth * 0.8
            }

            QGCLabel {
                text: qsTr("Time\n(s)")
                font.pointSize: _fontSize
                color: qgcPal.text
                Layout.minimumWidth: _altMsgMinWidth * 1.3
            }

            QGCLabel {
                text: qsTr("Press\n(mB)")
                font.pointSize: _fontSize
                color: qgcPal.text
                Layout.minimumWidth: _altMsgMinWidth
            }

            QGCLabel {
                text: qsTr("Temp\n(C)")
                font.pointSize: _fontSize
                color: qgcPal.text
                Layout.minimumWidth: _altMsgMinWidth
            }

            QGCLabel {
                text: qsTr("RelHum\n(%)")
                font.pointSize: _fontSize
                color: qgcPal.text
                Layout.minimumWidth: _altMsgMinWidth
            }

            QGCLabel {
                text: qsTr("WSpeed\n(m/s)")
                font.pointSize: _fontSize
                color: qgcPal.text
                Layout.minimumWidth: _altMsgMinWidth
            }

            QGCLabel {
                text: qsTr("WDir\n(deg)")
                font.pointSize: _fontSize
                color: qgcPal.text
                Layout.minimumWidth: _altMsgMinWidth * 0.9
            }
        }
        Flickable {
            id: altitudeFlickable
            anchors.top: almHeaders.bottom
            anchors.left: parent.left
            width: parent.width
            anchors.bottom: parent.bottom
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width
            contentHeight: almGrid.implicitHeight + 2 * _toolsMargin
            clip: true
            flickableDirection: Flickable.VerticalFlick

            MouseArea {
                anchors.fill: parent

                onWheel: {
                    altitudeFlickable.cancelFlick()
                    if (wheel.angleDelta.y > 0) {
                        altitudeFlickable.flick(0, 500)
                    } else {
                        altitudeFlickable.flick(0, -500)
                    }
                    wheel.accepted = true
                }
            }

            GridLayout {
                id: almGrid
                anchors.fill: parent
                anchors.topMargin: _toolsMargin
                anchors.bottomMargin: _toolsMargin
                anchors.leftMargin: _toolsMargin
                columns: 7
                rowSpacing: _toolsMargin
                columnSpacing: _toolsMargin

                Repeater {
                    model: controller.tempAltLevelMsgList.count
                    delegate: QGCLabel {
                            Layout.row:         index
                            Layout.column:      0
                            Layout.minimumWidth: _altMsgMinWidth * 0.8
                            text: controller.tempAltLevelMsgList.get(index).altitude
                            font.pointSize: _fontSize
                            color: qgcPal.text
                    }
                }

                Repeater {
                    model: controller.tempAltLevelMsgList.count
                    delegate: QGCLabel {
                            Layout.row:         index
                            Layout.column:      1
                            Layout.minimumWidth: _altMsgMinWidth * 1.3
                            text: controller.tempAltLevelMsgList.get(index).time
                            font.pointSize: _fontSize
                            color: qgcPal.text
                    }
                }

                Repeater {
                    model: controller.tempAltLevelMsgList.count
                    delegate: QGCLabel {
                            Layout.row:         index
                            Layout.column:      2
                            Layout.minimumWidth: _altMsgMinWidth
                            text: controller.tempAltLevelMsgList.get(index).pressure
                            font.pointSize: _fontSize
                            color: qgcPal.text
                    }
                }

                Repeater {
                    model: controller.tempAltLevelMsgList.count
                    delegate: QGCLabel {
                            Layout.row:         index
                            Layout.column:      3
                            Layout.minimumWidth: _altMsgMinWidth
                            text: controller.tempAltLevelMsgList.get(index).temperature
                            font.pointSize: _fontSize
                            color: qgcPal.text
                    }
                }

                Repeater {
                    model: controller.tempAltLevelMsgList.count
                    delegate: QGCLabel {
                            Layout.row:         index
                            Layout.column:      4
                            Layout.minimumWidth: _altMsgMinWidth
                            text: controller.tempAltLevelMsgList.get(index).relativeHumidity
                            font.pointSize: _fontSize
                            color: qgcPal.text
                    }
                }

                Repeater {
                    model: controller.tempAltLevelMsgList.count
                    delegate: QGCLabel {
                            Layout.row:         index
                            Layout.column:      5
                            Layout.minimumWidth: _altMsgMinWidth
                            text: controller.tempAltLevelMsgList.get(index).windSpeed
                            font.pointSize: _fontSize
                            color: qgcPal.text
                    }
                }

                Repeater {
                    model: controller.tempAltLevelMsgList.count
                    delegate: QGCLabel {
                            Layout.row:         index
                            Layout.column:      6
                            Layout.minimumWidth: _altMsgMinWidth * 0.9
                            text: controller.tempAltLevelMsgList.get(index).windDirection
                            font.pointSize: _fontSize
                            color: qgcPal.text
                    }
                }
            }

            ScrollBar.vertical: ScrollBar { }
        }
    }
}
