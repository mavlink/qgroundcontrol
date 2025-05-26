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


Rectangle {
    id: _root
    width: ScreenTools.defaultFontPixelHeight * 5.5
    height: ScreenTools.defaultFontPixelHeight * 3.2
    radius: 3
    anchors.left:   parent.left
    anchors.bottom: parent.bottom
    anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.3
    anchors.leftMargin: ScreenTools.defaultFontPixelHeight * 4.75
    color:  qgcPal.window
    opacity: 0.8
    clip: true

    property bool displayStatus: UTMSPStateStorage.indicatorDisplayStatus
    property var overlay
    property var indicatorTopText: ["No mission exist",
                                    "You've logged in successfully!",
                                    "      Your Flight Plan is Approved!",
                                    "Activation Time",
                                    "Activated Successfully!",
                                    "You've been logged out",
                                    "You've deleted the flight plan",
                                    "Hold Tight!",
                                    "Mission Completed"


                                    ]
    property var indicatorBottomText: ["Login to Create a UTM Mission",
                                       "Create a UTM Mission",
                                       "  Proceed to upload flight plan to vehcile",
                                       UTMSPStateStorage.indicatorActivationTime,
                                       "You are allowed to fly...",
                                       "Login to Create a UTM Mission",
                                       "Create a UTM Mission",
                                       "Your Drone is on Mission"
                                    ]
    property int currentNotificationIndex: UTMSPStateStorage.currentNotificationIndex

    QGCMouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: overlay.visible = true
        onExited: overlay.visible = false

    }

    Column {
        spacing: 5
        anchors.centerIn: parent

        Text {
            id: topNotificationText
            color: "white"
            visible: false
            font.bold: true
            font.pointSize: 10
            x: 10
            text : indicatorTopText[currentNotificationIndex]
        }

        Text {
            id: bottomNotificationText
            color: "white"
            visible: false
            x: 10
            text : indicatorBottomText[currentNotificationIndex]
        }
    }

    QGCButton {
        id: toggleButton
        height: ScreenTools.defaultFontPixelHeight * 3.2
        width: 25

        Text {
            text: _root.width > ScreenTools.defaultFontPixelHeight * 5.5 ? "<<" : ">>"
            font.bold: true
            color: "white"
            anchors.right: parent.right
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }

        anchors.right: parent.right
        onClicked: {
            if (_root.width === ScreenTools.defaultFontPixelHeight * 5.5) {
                expandAnimation.start();
                topNotificationText.visible = true
                bottomNotificationText.visible = true
            } else {
                collapseAnimation.start();
                topNotificationText.visible = false
                bottomNotificationText.visible = false
            }
        }
    }

    onDisplayStatusChanged: {
        if(displayStatus === true){
            topNotificationText.visible = true
            bottomNotificationText.visible = true
            expandAnimation.start();
        }
    }



    Timer {
        id: autoCollapseTimer
        interval: 5000
        repeat: false

        onTriggered: {
            if(UTMSPStateStorage.indicatorApprovedStatus === true){
                UTMSPStateStorage.indicatorDisplayStatus = true
            }

            else{
                collapseAnimation.start();
                UTMSPStateStorage.indicatorDisplayStatus = false
                topNotificationText.visible = false;
                bottomNotificationText.visible = false;
            }

        }
    }


    PropertyAnimation {
        id: expandAnimation
        target: _root
        property: "width"
        to: ScreenTools.defaultFontPixelHeight * 20
        duration: 1000
        onStopped: {
            if (_root.width > ScreenTools.defaultFontPixelHeight * 5.5) {
                topNotificationText.visible = true;
                bottomNotificationText.visible = true;
                autoCollapseTimer.start();
            }
        }
    }

    PropertyAnimation {
        id: collapseAnimation
        target: _root
        property: "width"
        to: ScreenTools.defaultFontPixelHeight * 5.5
        duration: 1000
    }

}


