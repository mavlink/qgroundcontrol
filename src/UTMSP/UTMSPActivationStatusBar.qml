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
            activationTriggered(false)
            displayActivationTabTimer.start()
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
                    activationTriggered(true)
                    UTMSPStateStorage.indicatorActivatedStatus = true
                    displayActivationTabTimer.stop()
                    UTMSPStateStorage.currentStateIndex = 2
                    UTMSPStateStorage.currentNotificationIndex = 4
                    UTMSPStateStorage.indicatorDisplayStatus = false
                }else{
                    failtag.visible = true
                }
            } else {
                var hours = Math.floor(diff / 3600000)
                var minutes = Math.floor((diff % 3600000) / 60000)
                var seconds = Math.floor((diff % 60000) / 1000)
                timeDifference = hours + " hours " + minutes + " minutes " + seconds + " seconds"
                UTMSPStateStorage.indicatorActivationTime = timeDifference.toString()
                UTMSPStateStorage.currentNotificationIndex = 3
            }
        }
    }

    //TODO: Create a dynamic real time mission progress bar
    Canvas {
        anchors.fill: parent

        onPaint: {
            var centerX = ScreenTools.defaultFontPixelHeight * 7.5 /2 + ScreenTools.defaultFontPixelHeight * 1.25
            var centerY = parent.height - ScreenTools.defaultFontPixelHeight * 7.5 /2 - ScreenTools.defaultFontPixelHeight * 0.3
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = qgcPal.window
            ctx.lineWidth = ScreenTools.defaultFontPixelHeight * 0.75
            ctx.beginPath()

            var attitudeRadius = 78
            var zeroAttitudeRadians = 2.18166
            var maxRadians = 0

            ctx.arc(centerX, centerY, attitudeRadius, zeroAttitudeRadians, maxRadians)
            ctx.stroke()
        }
    }

    //TODO: same as above
    Canvas {
        anchors.fill: parent

        onPaint: {
            var centerX = ScreenTools.defaultFontPixelHeight * 7.5 /2 + ScreenTools.defaultFontPixelHeight * 1.25
            var centerY = parent.height - ScreenTools.defaultFontPixelHeight * 7.5 /2 - ScreenTools.defaultFontPixelHeight * 0.3
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = qgcPal.window
            ctx.lineWidth = ScreenTools.defaultFontPixelHeight * 0.75
            ctx.beginPath()

            var attitudeRadius = 78
            var zeroAttitudeRadians = 2.18166
            var maxRadians = 0

            ctx.arc(centerX, centerY, attitudeRadius, zeroAttitudeRadians, maxRadians)   // TODO: Create a variable values
            ctx.stroke()
        }
    }

    //TODO: Same as above
    Canvas {
        anchors.fill: parent

        onPaint: {
            var centerX = ScreenTools.defaultFontPixelHeight * 7.5 /2 + ScreenTools.defaultFontPixelHeight * 1.25
            var centerY = parent.height - ScreenTools.defaultFontPixelHeight * 7.5 /2 - ScreenTools.defaultFontPixelHeight * 0.3
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = qgcPal.text
            ctx.lineWidth = 2
            ctx.beginPath()

            var attitudeRadius = 84
            var angleRadians = 2.18166

            var outerX = centerX + attitudeRadius * Math.cos(angleRadians)
            var outerY = centerY + attitudeRadius * Math.sin(angleRadians)

            var tickMarkLength = 13
            var innerX = centerX + (attitudeRadius - tickMarkLength) * Math.cos(angleRadians)
            var innerY = centerY + (attitudeRadius - tickMarkLength) * Math.sin(angleRadians)

            ctx.moveTo(outerX, outerY)
            ctx.lineTo(innerX, innerY)
            ctx.stroke()
        }
    }

    UTMSPNotificationSlider{
        id: notificationslider
        overlay: overlayRect
    }

    Rectangle{
        id: overlayRect
        width: notificationslider.width + 20
        height: 40
        radius: 3
        color:  qgcPal.window
        opacity: 0.8
        anchors.left:   parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight * 3.75
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 3.4
        visible: false


        Column{
            spacing: 5
            anchors.verticalCenter: parent.verticalCenter
            Row {
                x: 100
                Text{
                    text: "Serial-Number: "
                    color: "white"
                    font.pointSize: 7
                    font.bold: true
                }

                Text{
                    text: UTMSPStateStorage.serialNumber
                    color: "white"
                    font.pointSize: 7
                }
            }


            Row {
                x: 100
                Text{
                    text: "FlightID: "
                    color: "white"
                    font.pointSize: 7
                    font.bold: true
                }

                Text{
                    text: UTMSPStateStorage.flightID
                    color: "white"
                    font.pointSize: 7
                }
            }

        }
    }

    UTMSPFlightStatusIndicator {
    //TODO: add conformance notification
    }

}
