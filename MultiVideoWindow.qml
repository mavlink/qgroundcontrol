import QtQuick                  2.4
import QtPositioning            5.2
import QtQuick.Layouts          1.2
import QtQuick.Controls         1.4
import QtQuick.Dialogs          1.2
import QtGraphicalEffects       1.0

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0

import org.freedesktop.gstreamer.GLVideoItem 1.0

ApplicationWindow {
    id:             multiVideoWindow
    title:          "Multi-Video Context"
    width:          1280
    height:         720
    minimumWidth:   width
    minimumHeight:  height
    maximumWidth:   width
    maximumHeight:  height
    visible:        true

    Item {
        anchors.fill: parent
        Rectangle {
            width: parent.width/2
            height: parent.height/2
            x: 0
            y: 0
            color: "#000000"
        }
        GstGLVideoItem {
            objectName: "videoContent0"
            width: parent.width/2
            height: parent.height/2
            x: 0
            y: 0
            property var receiver
        }
    }

    Item {
        anchors.fill: parent
        Rectangle {
            width: parent.width/2
            height: parent.height/2
            x: parent.width/2
            y: 0
            color: "#000000"
        }
        GstGLVideoItem {
            objectName: "videoContent1"
            width: parent.width/2
            height: parent.height/2
            x: parent.width/2
            y: 0
            property var receiver
        }
    }

    Item {
        anchors.fill: parent
        Rectangle {
            width: parent.width/2
            height: parent.height/2
            x: parent.width/4
            y: parent.height/2
            color: "#000000"
        }
        GstGLVideoItem {
            objectName: "videoContent2"
            width: parent.width/2
            height: parent.height/2
            x: parent.width/4
            y: parent.height/2
            property var receiver
        }
    }

    // Recording button
    Item {
        Layout.alignment:   Qt.AlignHCenter
        width:              ScreenTools.defaultFontPixelWidth * 6
        height:             width
        id:                 recordingRect

        Rectangle {
            color: Qt.rgba(0,0,0,1)
            width: parent.width
            height: time.height
            anchors.centerIn: parent
        }

        // Start recording button
        Rectangle {
            id: circle
            anchors.centerIn:   parent
            width:              parent.width * 0.5
            height:             width
            radius:             width * 0.75
            color:              Qt.rgba(1,0,0,1)
        }

        // Stop recording button
        Rectangle {
            id: square
            anchors.centerIn:   parent
            width:              parent.width * 0.5
            height:             width
            color:              Qt.rgba(1,1,1,1)
            visible:            !circle.visible
        }

        function beginRecording() {
            recordingRect.passedTime = new Date();
            recordingRect.passedMs = 0;
            timer.running = true;
            QGroundControl.videoManager.startMultiCamRecording();
        }

        function endRecording() {
            recordingRect.passedTime = new Date();
            recordingRect.passedMs = 0;
            time.text = recordingRect.timeItemFormat(recordingRect.passedMs);
            timer.running = false;
            QGroundControl.videoManager.stopMultiCamRecording();
        }

        // Actual button
        MouseArea {
            anchors.fill:   parent
            enabled:        true
            onClicked: function() {
                // Toggle: yes it is hacky, but it works
                if (circle.visible) {
                    circle.visible = false;
                    recordingRect.beginRecording();
                } else {
                    circle.visible = true;
                    recordingRect.endRecording();
                }
            }
        }

        property date passedTime: new Date()
        property int passedMs: 0

        function pad(num, len) {
            var result = num.toString();
            while (result.length < len) {
                result = "0" + result;
            }
            return result;
        }

        function timeItemFormat(num) {
            var result = "";
            var milliseconds = Math.floor(num % 1000);
            var seconds = Math.floor(num / 1000 % 60);
            var minutes = Math.floor(num / 1000 / 60);
            result = pad(minutes, 2) + ":" + pad(seconds, 2) + ":" + pad(milliseconds, 3);
            return result;
        }

        Timer {
            id: timer
            interval: 10
            running: false
            repeat: true
            onTriggered: function() {
                recordingRect.passedMs += new Date() - recordingRect.passedTime;
                recordingRect.passedTime = new Date();
                time.text = recordingRect.timeItemFormat(recordingRect.passedMs);
            }
        }

        Rectangle {
            color: "white"
            anchors.verticalCenter: circle.verticalCenter
            anchors.left: circle.right
            anchors.leftMargin: 10
            Text {
                id: time
                horizontalAlignment: Text.AlignHCenter
                anchors.verticalCenter: circle.verticalCenter
                anchors.left: circle.right
                anchors.leftMargin: 10
                text:  "00:00:00"
                color: "black"
                font.family: ScreenTools.demiboldFontFamily
                font.pixelSize: 20
            }
            width: childrenRect.width
            height: childrenRect.height
            x: childrenRect.x
            y: childrenRect.y
        }
    }
}
