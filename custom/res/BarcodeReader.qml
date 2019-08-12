/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Layouts              1.11
import QtQuick.Window               2.0
import QtMultimedia                 5.5

import QZXing                       2.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

import CustomQuickInterface       1.0

Rectangle {
    id:                 _root
    color:              qgcPal.window
    anchors.fill:       parent

    property string lastTag: ""

    Camera {
        id:             camera
        focus {
            focusMode:      CameraFocus.FocusContinuous
            focusPointMode: CameraFocus.FocusPointAuto
        }
    }

    VideoOutput  {
        id:             videoOutput
        source:         camera
        width:          parent.width  * 0.75
        height:         parent.height * 0.75
        fillMode:       VideoOutput.PreserveAspectFit
        filters:        [ zxingFilter ]
        anchors.centerIn:   parent
        autoOrientation:    true
        MouseArea {
            anchors.fill: parent
            onClicked: {
                camera.focus.customFocusPoint   = Qt.point(mouse.x / width,  mouse.y / height);
                camera.focus.focusMode          = CameraFocus.FocusMacro;
                camera.focus.focusPointMode     = CameraFocus.FocusPointCustom;
            }
        }
        Rectangle {
            id:         captureZone
            color:      "red"
            opacity:    0.2
            width:      parent.width  / 2
            height:     parent.height / 2
            anchors.centerIn: parent
        }
    }

    SoundEffect {
        id:                 playSound
        source:             "/custom/wav/beep.wav"
    }

    SoundEffect {
        id:                 playError
        source:             "/custom/wav/boop.wav"
    }

    Rectangle {
        width:              batteryCol.width  + (ScreenTools.defaultFontPixelWidth  * 2)
        height:             batteryCol.height + (ScreenTools.defaultFontPixelHeight * 2)
        color:              Qt.rgba(0,0,0,0.5)
        radius:             ScreenTools.defaultFontPixelWidth * 0.5
        visible:            CustomQuickInterface.batteries.length > 0
        border.width:       1
        border.color:       Qt.rgba(1,1,1,0.25)
        anchors.right:      parent.right
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter: parent.verticalCenter
        Column {
            id:             batteryCol
            spacing:        ScreenTools.defaultFontPixelHeight
            anchors.centerIn: parent
            QGCLabel {
                text:       qsTr("Batteries")
                color:      "white"
                width:      ScreenTools.defaultFontPixelWidth * 20
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Rectangle {
                width:      parent.width
                height:     1
                color:      Qt.rgba(1,1,1,0.25)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Repeater {
                model:      CustomQuickInterface.batteries
                QGCLabel {
                    text:   modelData
                    color:  "white"
                    font.pointSize: ScreenTools.smallFontPointSize
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            Rectangle {
                width:      parent.width
                height:     1
                color:      Qt.rgba(1,1,1,0.25)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Button {
                text:       qsTr("Reset")
                onClicked:  {
                    _root.lastTag = ""
                    CustomQuickInterface.resetBatteries()
                }
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }


    QZXingFilter  {
        id:             zxingFilter
        captureRect: {
            // setup bindings
            videoOutput.contentRect;
            videoOutput.sourceRect;
            return videoOutput.mapRectToSource(videoOutput.mapNormalizedRectToItem(Qt.rect(0.25, 0.25, 0.5, 0.5)));
        }
        decoder {
            //enabledDecoders: QZXing.DecoderFormat_UPC_A | QZXing.DecoderFormat_UPC_E | QZXing.DecoderFormat_QR_CODE
            enabledDecoders: QZXing.DecoderFormat_QR_CODE
            onTagFound: {
                console.log(tag + " | " + decoder.foundedFormat() + " | " + decoder.charSet());
                if(_root.lastTag != tag) {
                    _root.lastTag = tag
                    if(CustomQuickInterface.addBatteryScan(tag)) {
                        playSound.play()
                    } else {
                        playError.play()
                    }
                }
            }
            tryHarder: false
        }

        onDecodingStarted: {
            //console.log("started");
        }

        property int    framesDecoded: 0
        property real   timePerFrameDecode: 0

        onDecodingFinished: {
           timePerFrameDecode = (decodeTime + framesDecoded * timePerFrameDecode) / (framesDecoded + 1);
           framesDecoded++;
            if(succeeded) {
                console.log("frame finished: " + succeeded, decodeTime, timePerFrameDecode, framesDecoded);
            }
        }
    }

}
