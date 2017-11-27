/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.4
import QtPositioning            5.2
import QtQuick.Layouts          1.2
import QtQuick.Controls         1.4
import QtQuick.Dialogs          1.2
import QtQuick.Controls.Styles  1.4
import QtGraphicalEffects       1.0

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0

/// Video streaming page for Instrument Panel PageView
Item {
    width:              pageWidth
    height:             videoGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
    anchors.margins:    ScreenTools.defaultFontPixelWidth * 2
    anchors.centerIn:   parent

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost:     _activeVehicle ? _activeVehicle.connectionLost : false
    property var    _videoReceiver:         QGroundControl.videoManager.videoReceiver
    property bool   _recordingVideo:        _videoReceiver && _videoReceiver.recording
    property bool   _videoRunning:          _videoReceiver && _videoReceiver.videoRunning

    QGCPalette { id:qgcPal; colorGroupEnabled: parent.enabled }

    GridLayout {
        id:                 videoGrid
        columns:            2
        columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
        rowSpacing:         ScreenTools.defaultFontPixelHeight
        anchors.centerIn:   parent
        QGCLabel {
           text:            qsTr("Enable Stream")
           font.pointSize:  ScreenTools.smallFontPointSize
        }
        Switch {
            checked:        _videoRunning
            enabled:        _activeVehicle
            onClicked: {
                if(checked) {
                    _videoReceiver.start()
                } else {
                    _videoReceiver.stop()
                }
            }
            style:      SwitchStyle {
                groove:     Rectangle {
                    implicitWidth:  ScreenTools.defaultFontPixelWidth * 6
                    implicitHeight: ScreenTools.defaultFontPixelHeight
                    color:          control.checked ? qgcPal.colorGreen : qgcPal.colorGrey
                    radius:         3
                    border.color:   qgcPal.button
                    border.width:   1
                }
            }
        }
        QGCLabel {
           text:            qsTr("Stream Recording")
           font.pointSize:  ScreenTools.smallFontPointSize
           visible:         QGroundControl.settingsManager.videoSettings.showRecControl.rawValue
        }
        // Button to start/stop video recording
        Item {
            anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
            height:             ScreenTools.defaultFontPixelHeight * 2
            width:              height
            Layout.alignment:   Qt.AlignHCenter
            visible:            QGroundControl.settingsManager.videoSettings.showRecControl.rawValue

            Rectangle {
                id:                 recordBtnBackground
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                width:              height
                radius:             _recordingVideo ? 0 : height
                color:              _videoRunning ? "red" : "gray"
                SequentialAnimation on opacity {
                    running:        _recordingVideo
                    loops:          Animation.Infinite
                    PropertyAnimation { to: 0.5; duration: 500 }
                    PropertyAnimation { to: 1.0; duration: 500 }
                }
            }

            QGCColoredImage {
                anchors.top:                parent.top
                anchors.bottom:             parent.bottom
                anchors.horizontalCenter:   parent.horizontalCenter
                width:                      height * 0.625
                sourceSize.width:           width
                source:                     "/qmlimages/CameraIcon.svg"
                visible:                    recordBtnBackground.visible
                fillMode:                   Image.PreserveAspectFit
                color:                      "white"
            }

            MouseArea {
                anchors.fill:   parent
                enabled:        _videoRunning
                onClicked: {
                    if (_recordingVideo) {
                        _videoReceiver.stopRecording()
                        // reset blinking animation
                        recordBtnBackground.opacity = 1
                    } else {
                        _videoReceiver.startRecording()
                    }
                }
            }
        }
    }
}
