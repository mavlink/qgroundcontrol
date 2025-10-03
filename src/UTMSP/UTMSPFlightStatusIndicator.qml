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

import QGroundControl.Controls

import QGroundControl.FactControls


import QGroundControl.UTMSP
import QGroundControl.FlightDisplay


//Indicator

Rectangle {
    id: statusCircle
    width:  ScreenTools.defaultFontPixelHeight * 7.5
    height: ScreenTools.defaultFontPixelHeight * 7.5
    radius: width / 2
    color:  qgcPal.window
    anchors.left:   parent.left
    anchors.bottom: parent.bottom
    anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.3
    anchors.leftMargin: ScreenTools.defaultFontPixelHeight * 1.25
    visible: true

    property var ledImages: ["qrc:/qml/QGroundControl/UTMSP/images/yellow_led.png",
                             "qrc:/qml/QGroundControl/UTMSP/images/orange_led.png",
                             "qrc:/qml/QGroundControl/UTMSP/images/pale_green.png",
                             "qrc:/qml/QGroundControl/UTMSP/images/parrot_green.png",
                             "qrc:/qml/QGroundControl/UTMSP/images/green_led.png"]

    property var statusBarColor: ["#bcc21b",
                                  "#f0a351",
                                  "#2edbc1",
                                  "#9cbf43",
                                  "#31870f" ]

    property var flightState: ["IDLE",
                               "APPROVED",
                               "ACTIVATED",
                               "ON MISSION",
                               "COMPLETED"]

    property var progressPercentage: [0, 0.25, 0.50, 0.75, 1]
    property int currentIndex: UTMSPStateStorage.currentStateIndex


    Rectangle {
        width:  statusCircle.width
        height: statusCircle.height
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        radius: width / 2
        color:  qgcPal.window
        border.color: "white"
        border.width: 3
        opacity: .5
    }

    Text{
        text: "Flight Status"
        color: "white"
        y: 30
        font.bold: true
        font.pointSize: 8
        anchors.horizontalCenter: parent.horizontalCenter
    }


    Rectangle {
        width:  ScreenTools.defaultFontPixelHeight * 5.5
        height: ScreenTools.defaultFontPixelHeight * 1.5
        color:  statusBarColor[currentIndex]
        border.color: "white"
        radius: 10
        x: 17
        y: 50
        opacity: 0.9

        Text {
            text: flightState[currentIndex]
            color: "white"
            x: 17
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 9
        }
    }

    Rectangle {
        width: 50
        height: 10
        radius: 8
        color: "#2f2f2f"
        x: 32
        y: 90
        border.color: "white"

        Rectangle {
            width: parent.width * progressPercentage[currentIndex]
            height: parent.height
            radius: parent.radius
            color: statusBarColor[currentIndex]
        }
    }

    Image {
        width:  10
        height: 10
        source: ledImages[currentIndex]
        x: 87
        y: 90
    }
}
