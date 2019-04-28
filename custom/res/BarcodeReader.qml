/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.4
import QtQuick.Window   2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts  1.1
import QtMultimedia     5.5

import QZXing           2.3

import QGroundControl                       1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:                 _root
    color:              qgcPal.window
    anchors.fill:       parent

    property int    detectedTags: 0
    property string lastTag: ""

    Rectangle {
        id:             bgRect
        color:          "gray"
        anchors.fill:   videoOutput
    }

    Text {
        id:             text1
        wrapMode:       Text.Wrap
        font.pixelSize: 20
        anchors.top:    parent.top
        anchors.left:   parent.left
        z:              50
        text:           "Tags detected: " + detectedTags
        color:          "white"
    }

    Text {
        id:             fps
        font.pixelSize: 20
        anchors.top:    parent.top
        anchors.right:  parent.right
        z:              50
        text:           (1000 / zxingFilter.timePerFrameDecode).toFixed(0) + "fps"
        color:          "white"
    }

    Camera {
        id:             camera
        focus {
            focusMode:  CameraFocus.FocusContinuous
            focusPointMode: CameraFocus.FocusPointAuto
        }
    }

    VideoOutput  {
        id:             videoOutput
        source:         camera
        anchors.top:    text1.bottom
        anchors.bottom: text2.top
        anchors.left:   parent.left
        anchors.right:  parent.right
        autoOrientation: true
        fillMode:       VideoOutput.Stretch
        filters:        [ zxingFilter ]
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
            width:      parent.width / 2
            height:     parent.height / 2
            anchors.centerIn: parent
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
            enabledDecoders: QZXing.DecoderFormat_EAN_13 | QZXing.DecoderFormat_CODE_39 | QZXing.DecoderFormat_QR_CODE
            onTagFound: {
                console.log(tag + " | " + decoder.foundedFormat() + " | " + decoder.charSet());
                _root.detectedTags++;
                _root.lastTag = tag;
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

    Text {
        id:             text2
        wrapMode:       Text.Wrap
        font.pixelSize: 20
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        z:              50
        text:           "Last tag: " + lastTag
        color:          "white"
    }

    Rectangle {
        width:          optCol.width  + (ScreenTools.defaultFontPixelWidth  * 2)
        height:         optCol.height + (ScreenTools.defaultFontPixelHeight * 2)
        color:          "gray"
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        Column {
            id:             optCol
            spacing:        10
            anchors.centerIn: parent
            ListView {
                width:        ScreenTools.defaultFontPixelWidth  * 10
                height:       ScreenTools.defaultFontPixelHeight * 5
                model:        QtMultimedia.availableCameras
                delegate: Text {
                  text: modelData.displayName
                  MouseArea {
                      anchors.fill: parent
                      onClicked:    camera.deviceId = modelData.deviceId
                  }
                }
            }
            Switch {
                text:       "Autofocus"
                checked:    camera.focus.focusMode === CameraFocus.FocusContinuous
                onCheckedChanged: {
                    if (checked) {
                        camera.focus.focusMode      = CameraFocus.FocusContinuous
                        camera.focus.focusPointMode = CameraFocus.FocusPointAuto
                    } else {
                        camera.focus.focusPointMode = CameraFocus.FocusPointCustom
                        camera.focus.customFocusPoint = Qt.point(0.5,  0.5)
                    }
                }
                font.family:    'Monospace'
                font.pixelSize: Screen.pixelDensity * 5
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

}
