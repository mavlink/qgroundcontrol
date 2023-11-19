/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                          2.11
import QtQuick.Layouts                  1.2
import QtQuick.Dialogs                  1.2
import QtQuick.Extras                   1.4
import QtQuick.Controls                 2.15
import QtPositioning                    5.2
import QtLocation                       5.12
import QtGraphicalEffects               1.0

import QGroundControl                       1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.ShapeFileHelper       1.0
import QGroundControl.UTMSP                 1.0
import QGroundControl.FlightDisplay         1.0
import QGroundControl.MultiVehicleManager   1.0

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

    Timer {
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
        y:              ScreenTools.defaultFontPixelHeight * 10
        anchors.right:  parent.right
        opacity:        0.9
        visible:        activationApproval
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
