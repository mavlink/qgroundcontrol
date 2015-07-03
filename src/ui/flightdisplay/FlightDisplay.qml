/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief QGC Main Flight Display
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2

import QGroundControl.FlightControls 1.0
import QGroundControl.MavManager 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    id: root

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    property real roll:    isNaN(MavManager.roll)    ? 0 : MavManager.roll
    property real pitch:   isNaN(MavManager.pitch)   ? 0 : MavManager.pitch

    property bool showPitchIndicator:       true
    property bool showAttitudeIndicator:    true
    property bool showCompass:              true

    function getBool(value) {
        return value === '0' ? false : true;
    }

    function setBool(value) {
        return value ? "1" : "0";
    }

    function showHudInstruments() {
        return videoBackground.visible || mapBackground.visible;
    }

    Connections {
        target: flightDisplay
        onShowOptionsMenuChanged: {
            contextMenu.popup();
        }
    }

    Component.onCompleted:
    {
        mapBackground.visible               = getBool(flightDisplay.loadSetting("showMapBackground",        "0"));
        mapBackground.showWaypoints         = getBool(flightDisplay.loadSetting("mapShowWaypoints",         "0"));
        mapBackground.alwaysNorth           = getBool(flightDisplay.loadSetting("mapAlwaysPointsNorth",     "0"));
        videoBackground.visible             = getBool(flightDisplay.loadSetting("showVideoBackground",      "0"));
        showAttitudeIndicator               = getBool(flightDisplay.loadSetting("showAttitudeIndicator",    "1"));
        showPitchIndicator                  = getBool(flightDisplay.loadSetting("showPitchIndicator",       "1"));
        showCompass                         = getBool(flightDisplay.loadSetting("showCompass",              "1"));
        altitudeWidget.visible              = getBool(flightDisplay.loadSetting("showAltitudeWidget",       "1"));
        speedWidget.visible                 = getBool(flightDisplay.loadSetting("showSpeedWidget",          "1"));
        currentSpeed.showAirSpeed           = getBool(flightDisplay.loadSetting("showCurrentAirSpeed",      "1"));
        currentSpeed.showGroundSpeed        = getBool(flightDisplay.loadSetting("showCurrentGroundSpeed",   "1"));
        currentAltitude.showClimbRate       = getBool(flightDisplay.loadSetting("showCurrentClimbRate",     "1"));
        currentAltitude.showAltitude        = getBool(flightDisplay.loadSetting("showCurrentAltitude",      "1"));
        // Insert Map Type menu before separator
        contextMenu.insertItem(2, mapBackground.mapMenu);
        // Video or Map. Not both:
        if(mapBackground.visible && videoBackground.visible) {
            videoBackground.visible = false;
            flightDisplay.saveSetting("showVideoBackground", setBool(videoBackground.visible));
        }
    }

    Menu {
        id: contextMenu

        ExclusiveGroup { id: backgroundDisplay }

        MenuItem {
            text: "Map Background"
            checkable: true
            checked: mapBackground.visible
            exclusiveGroup: backgroundDisplay
            onTriggered:
            {
                mapBackground.visible = !mapBackground.visible;
                flightDisplay.saveSetting("showMapBackground", setBool(mapBackground.visible));
                videoBackground.visible = mapBackground.visible ? false : videoBackground.visible;
                flightDisplay.saveSetting("showVideoBackground", setBool(videoBackground.visible));
            }
        }

        MenuItem {
            text: "Map Show Waypoints"
            checkable: true
            checked: mapBackground.showWaypoints
            enabled: mapBackground.visible
            onTriggered:
            {
                mapBackground.showWaypoints = !mapBackground.showWaypoints;
                flightDisplay.saveSetting("mapShowWaypoints", setBool(mapBackground.showWaypoints));
            }
        }

        /*
        //-- Off until Qt 5.5.x, which fixes bug in 5.4.x
        MenuItem {
            text: "Map Always Points North"
            checkable: true
            checked: mapBackground.alwaysNorth
            onTriggered:
            {
                mapBackground.alwaysNorth = !mapBackground.alwaysNorth;
                flightDisplay.saveSetting("mapAlwaysPointsNorth", setBool(mapBackground.alwaysNorth));
            }
        }
        */

        MenuSeparator {}

        MenuItem {
            text: "Video Background"
            checkable: true
            checked: videoBackground.visible
            exclusiveGroup: backgroundDisplay
            onTriggered:
            {
                videoBackground.visible = !videoBackground.visible;
                flightDisplay.saveSetting("showVideoBackground", setBool(videoBackground.visible));
                mapBackground.visible = videoBackground.visible ? false : mapBackground.visible;
                flightDisplay.saveSetting("showMapBackground", setBool(mapBackground.visible));
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Pitch Indicator"
            checkable:  true
            checked:    showPitchIndicator
            onTriggered:
            {
                showPitchIndicator = !showPitchIndicator;
                flightDisplay.saveSetting("showPitchIndicator", setBool(showPitchIndicator));
            }
        }

        MenuItem {
            text: "Attitude Indicator"
            checkable:  true
            checked:    showAttitudeIndicator
            onTriggered:
            {
                showAttitudeIndicator = !showAttitudeIndicator;
                flightDisplay.saveSetting("showAttitudeIndicator", setBool(showAttitudeIndicator));
            }
        }

        MenuItem {
            text: "Compass"
            checkable: true
            checked: showCompass
            onTriggered:
            {
                showCompass = !showCompass;
                flightDisplay.saveSetting("showCompass", setBool(showCompass));
            }
        }

        MenuItem {
            text: "Altitude Indicator"
            checkable: true
            checked: altitudeWidget.visible
            onTriggered:
            {
                altitudeWidget.visible = !altitudeWidget.visible;
                flightDisplay.saveSetting("showAltitudeWidget", setBool(altitudeWidget.visible));
            }
        }

        MenuItem {
            text: "Current Altitude"
            checkable: true
            checked: currentAltitude.showAltitude
            onTriggered:
            {
                currentAltitude.showAltitude = !currentAltitude.showAltitude;
                flightDisplay.saveSetting("showCurrentAltitude", setBool(currentAltitude.showAltitude));
            }
        }

        MenuItem {
            text: "Current Climb Rate"
            checkable: true
            checked: currentAltitude.showClimbRate
            onTriggered:
            {
                currentAltitude.showClimbRate = !currentAltitude.showClimbRate;
                flightDisplay.saveSetting("showCurrentClimbRate", setBool(currentAltitude.showClimbRate));
            }
        }

        MenuItem {
            text: "Speed Indicator"
            checkable: true
            checked: speedWidget.visible
            onTriggered:
            {
                speedWidget.visible = !speedWidget.visible;
                flightDisplay.saveSetting("showSpeedWidget", setBool(speedWidget.visible));
            }
        }

        MenuItem {
            text: "Current Air Speed"
            checkable: true
            checked: currentSpeed.showAirSpeed
            onTriggered:
            {
                currentSpeed.showAirSpeed = !currentSpeed.showAirSpeed;
                flightDisplay.saveSetting("showCurrentAirSpeed", setBool(currentSpeed.showAirSpeed));
            }
        }

        MenuItem {
            text: "Current Ground Speed"
            checkable: true
            checked: currentSpeed.showGroundSpeed
            onTriggered:
            {
                currentSpeed.showGroundSpeed = !currentSpeed.showGroundSpeed;
                flightDisplay.saveSetting("showCurrentGroundSpeed", setBool(currentSpeed.showGroundSpeed));
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Restore Defaults"
            onTriggered:
            {
                showPitchIndicator = true;
                flightDisplay.saveSetting("showPitchIndicator", setBool(showPitchIndicator));
                showAttitudeIndicator = true;
                flightDisplay.saveSetting("showAttitudeIndicator", setBool(showAttitudeIndicator));
                showCompass = true;
                flightDisplay.saveSetting("showCompass", setBool(showCompass));
                altitudeWidget.visible = true;
                flightDisplay.saveSetting("showAltitudeWidget", setBool(altitudeWidget.visible));
                currentAltitude.showAltitude = true;
                flightDisplay.saveSetting("showCurrentAltitude", setBool(currentAltitude.showAltitude));
                currentAltitude.showClimbRate = true;
                flightDisplay.saveSetting("showCurrentClimbRate", setBool(currentAltitude.showClimbRate));
                speedWidget.visible = true;
                flightDisplay.saveSetting("showSpeedWidget", setBool(speedWidget.visible));
                currentSpeed.showAirSpeed = true;
                flightDisplay.saveSetting("showCurrentAirSpeed", setBool(currentSpeed.showAirSpeed));
                currentSpeed.showGroundSpeed = true;
                flightDisplay.saveSetting("showCurrentGroundSpeed", setBool(currentSpeed.showGroundSpeed));
                mapBackground.visible = false;
                flightDisplay.saveSetting("showMapBackground", setBool(mapBackground.visible));
                mapBackground.alwaysNorth = false;
                flightDisplay.saveSetting("mapAlwaysPointsNorth", setBool(mapBackground.alwaysNorth));
                mapBackground.showWaypoints = false
                flightDisplay.saveSetting("mapShowWaypoints", setBool(mapBackground.showWaypoints));
                videoBackground.visible = false;
                flightDisplay.saveSetting("showVideoBackground", setBool(videoBackground.visible));
            }
        }

    }

    QGCVideoBackground {
        id:                 videoBackground
        anchors.fill:       parent
        display:            videoDisplay
        receiver:           videoReceiver
        z:                  10
    }

    QGCMapBackground {
        id:                 mapBackground
        anchors.fill:       parent
        mapName:            'MainFlightDisplay'
        heading:            0 // isNaN(MavManager.heading) ? 0 : MavManager.heading
        latitude:           mapBackground.visible ? ((MavManager.latitude  === 0) ?   37.803784 : MavManager.latitude)  :   37.803784
        longitude:          mapBackground.visible ? ((MavManager.longitude === 0) ? -122.462276 : MavManager.longitude) : -122.462276
        readOnly:           true
      //interactive:        !MavManager.mavPresent
        z:                  10
    }

    QGCHudMessage {
        id:                 hudMessage
        y:                  ScreenTools.defaultFontPizelSize * (0.42)
        width:              (parent.width - 520 > 200) ? parent.width - 520 : 200
        height:             ScreenTools.defaultFontPizelSize * (2.5)
        anchors.horizontalCenter: parent.horizontalCenter
        z:                  mapBackground.z + 1
    }

    QGCCompassInstrument {
        id:                 compassInstrument
        y:                  ScreenTools.defaultFontPixelSize * (0.42)
        x:                  ScreenTools.defaultFontPixelSize * (7.1)
        size:               ScreenTools.defaultFontPixelSize * (13.3)
        heading:            isNaN(MavManager.heading) ? 0 : MavManager.heading
        visible:            showHudInstruments() && showCompass
        z:                  mapBackground.z + 2
        onResetRequested: {
            y               = ScreenTools.defaultFontPixelSize * (0.42)
            x               = ScreenTools.defaultFontPixelSize * (7.1)
            size            = ScreenTools.defaultFontPixelSize * (13.3)
            tForm.xScale    = 1
            tForm.yScale    = 1
        }
    }

    QGCAttitudeInstrument {
        id:                 attitudeInstrument
        y:                  ScreenTools.defaultFontPixelSize * (0.42)
        size:               ScreenTools.defaultFontPixelSize * (13.3)
        rollAngle:          roll
        pitchAngle:         pitch
        showPitch:          showPitchIndicator
        visible:            showHudInstruments() && showAttitudeIndicator
        anchors.right:      root.right
        anchors.rightMargin: ScreenTools.defaultFontPixelSize * (7.1)
        z:                  mapBackground.z + 2
        onResetRequested: {
            y                   = ScreenTools.defaultFontPixelSize * (0.42)
            anchors.right       = root.right
            anchors.rightMargin = ScreenTools.defaultFontPixelSize * (7.1)
            size                = ScreenTools.defaultFontPixelSize * (13.3)
            tForm.xScale        = 1
            tForm.yScale        = 1
        }
    }

    QGCArtificialHorizon {
        id:                 artificialHoriz
        anchors.fill:       parent
        rollAngle:          roll
        pitchAngle:         pitch
        visible:            !showHudInstruments()
    }

    QGCAttitudeWidget {
        id:                 attitudeWidget
        rollAngle:          roll
        pitchAngle:         pitch
        visible:            !showHudInstruments() && showAttitudeIndicator
        width:              ScreenTools.defaultFontPixelSize * (21.7)
        height:             ScreenTools.defaultFontPixelSize * (21.7)
        z:                  20
    }

    QGCPitchWidget {
        id:                 pitchWidget
        visible:            showPitchIndicator && !showHudInstruments()
        anchors.verticalCenter: parent.verticalCenter
        pitchAngle:         pitch
        rollAngle:          roll
        color:              Qt.rgba(0,0,0,0)
        size:               ScreenTools.defaultFontPixelSize * (10)
        z:                  30
    }

    QGCAltitudeWidget {
        id:                 altitudeWidget
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (5)
        height:             parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        altitude:           MavManager.altitudeWGS84
        z:                  30
    }

    QGCSpeedWidget {
        id:                 speedWidget
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (5)
        height:             parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        speed:              MavManager.groundSpeed
        z:                  40
    }

    QGCCurrentSpeed {
        id: currentSpeed
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        airspeed:           MavManager.airSpeed
        groundspeed:        MavManager.groundSpeed
        showAirSpeed:       true
        showGroundSpeed:    true
        visible:            (currentSpeed.showGroundSpeed || currentSpeed.showAirSpeed)
        z:                  50
    }

    QGCCurrentAltitude {
        id: currentAltitude
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        altitude:           MavManager.altitudeWGS84
        vertZ:              MavManager.climbRate
        showAltitude:       true
        showClimbRate:      true
        visible:            (currentAltitude.showAltitude || currentAltitude.showClimbRate)
        z:                  60
    }

    QGCCompass {
        id:                 compassIndicator
        y:                  root.height * 0.7
        x:                  root.width  * 0.5 - ScreenTools.defaultFontPixelSize * (5)
        width:              ScreenTools.defaultFontPixelSize * (10)
        height:             ScreenTools.defaultFontPixelSize * (10)
        heading:            isNaN(MavManager.heading) ? 0 : MavManager.heading
        visible:            !showHudInstruments() && showCompass
        z:                  70
    }

    //- Context Menu
    MouseArea {
        anchors.fill: parent
        z: 1000
        acceptedButtons: Qt.RightButton
        onClicked: {
            if (mouse.button == Qt.RightButton)
            {
                contextMenu.popup();
            }
        }
    }
}
