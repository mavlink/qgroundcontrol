/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls
import QtPositioning
import QtLocation

import QGroundControl
import QGroundControl.FlightMap
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.Controllers
import QGroundControl.ShapeFileHelper
import QGroundControl.UTMSP
import QGroundControl.FlightDisplay
import QGroundControl.MultiVehicleManager

Item {
    id: _root
    // Activation window for UTM Adapter
    property date   targetTimestamp : new Date(activationStartTimestamp)
    property date   localTimestamp  : new Date(targetTimestamp.getTime() - 4 * 60 * 60 * 1000)
    property date   currentTime     : new Date()
    property bool   _utmspEnabled   : QGroundControl.utmspSupported
    property string activationStartTimestamp
    property bool   activationApproval
    property string flightID
    property string timeDifference
    property bool   activationErrorFlag

    signal activationTriggered(bool value)

    onActivationApprovalChanged: {
        if(activationApproval === true){
            approvetag.visible = true
            activatetag.visible = false
            activationBar.visible = true
            activationTriggered(false)
            displayActivationTabTimer.start()
        }
        else{
            activationBar.visible = false
        }
    }

    Timer {
        id: displayActivationTabTimer
        interval: 1000
        running:  activationApproval
        repeat:   activationApproval
        onTriggered: {
            currentTime = new Date()
            var diff = localTimestamp - currentTime;
            if(diff <= 0) {
                timeDifference = "0 hours 0 minutes 0 seconds"
                running = false;
                var activationErrorFlag = QGroundControl.utmspManager.utmspVehicle.activationFlag
                if(activationErrorFlag === true){
                    QGroundControl.utmspManager.utmspVehicle.loadTelemetryFlag(true)
                    approvetag.visible = false
                    activatetag.visible = true
                    activationTriggered(true)
                    displayActivationTabTimer.stop()
                    hideTimer.start()
                }else{
                    approvetag.visible = false
                    failtag.visible = true
                }
            } else {
                var hours = Math.floor(diff / 3600000)
                var minutes = Math.floor((diff % 3600000) / 60000)
                var seconds = Math.floor((diff % 60000) / 1000)
                timeDifference = hours + " hours " + minutes + " minutes " + seconds + " seconds"
            }
        }
    }

    Timer {
        id:          hideTimer
        interval:    5000
        running:     false
        onTriggered: activationBar.visible = false
    }

    Rectangle {
        id:             activationBar
        color:          qgcPal.textFieldText
        border.color:   qgcPal.textFieldText
        width:          ScreenTools.defaultFontPixelWidth * 83.33
        height:         ScreenTools.defaultFontPixelHeight * 8.33
        anchors.left:   parent.left
        anchors.bottom: parent.bottom
        opacity:        0.7
        visible:        false
        radius:         ScreenTools.defaultFontPixelWidth * 0.833

        QGCColoredImage {
            id:         closeButton
            width:      ScreenTools.defaultFontPixelWidth * 5
            height:     ScreenTools.defaultFontPixelWidth * 5
            x:          460
            y:          5
            source:     "/res/XDelete.svg"
            fillMode:   Image.PreserveAspectFit
            color:      qgcPal.text
        }
        QGCMouseArea {
            fillItem:   closeButton
            onClicked:  {
                activationBar.visible = false
                activationApproval = false
            }
        }

        Column{
            spacing:          ScreenTools.defaultFontPixelWidth * 1.667
            anchors.centerIn: parent
            Text{
                text:                    "Remaining time for activation!!!"
                color:                    qgcPal.buttonText
                font.pixelSize:           ScreenTools.defaultFontPixelWidth * 2.5
                font.bold:                true
                horizontalAlignment:      Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                visible:                  true
            }

            Text {
                text:                     timeDifference
                color:                    qgcPal.buttonText
                font.pixelSize:           ScreenTools.defaultFontPixelWidth * 3.667
                font.bold:                true
                horizontalAlignment:      Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Row{
                spacing:                  ScreenTools.defaultFontPixelWidth * 0.5
                anchors.horizontalCenter: parent.horizontalCenter

                Text {
                    text:                "FLIGHT ID: "
                    color:               qgcPal.buttonText
                    font.pixelSize:      ScreenTools.defaultFontPixelWidth * 2.5
                    font.bold:           true
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text:                flightID
                    color:               qgcPal.buttonText
                    font.pixelSize:      ScreenTools.defaultFontPixelWidth * 2.5
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Row{
                spacing:                  ScreenTools.defaultFontPixelWidth * 0.5
                anchors.horizontalCenter: parent.horizontalCenter

                Text {
                    text:                "STATUS: "
                    color:               qgcPal.buttonText
                    font.pixelSize:      ScreenTools.defaultFontPixelWidth * 2.5
                    font.bold:           true
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    id:                  approvetag
                    text:                "Approved"
                    color:               qgcPal.colorOrange
                    font.pixelSize:      ScreenTools.defaultFontPixelWidth * 3
                    horizontalAlignment: Text.AlignHCenter
                    visible:             true
                    font.bold:           true
                }

                Text {
                    id:                  activatetag
                    text:                "Activated"
                    color:               qgcPal.colorGreen
                    font.pixelSize:      ScreenTools.defaultFontPixelWidth * 3
                    horizontalAlignment: Text.AlignHCenter
                    visible:             false
                    font.bold:           true
                }
                Text {
                    id:                  failtag
                    text:                "Activation Failed"
                    color:               qgcPal.colorRed
                    font.pixelSize:      ScreenTools.defaultFontPixelWidth * 3
                    horizontalAlignment: Text.AlignHCenter
                    visible:             false
                    font.bold:           true
                }
            }
        }
    }
}
