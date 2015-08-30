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

import QGroundControl.FlightMap 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    id: root

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    property var activeVehicle: multiVehicleManager.activeVehicle

    readonly property real defaultLatitude:         37.803784
    readonly property real defaultLongitude:        -122.462276
    readonly property real defaultRoll:             0
    readonly property real defaultPitch:            0
    readonly property real defaultHeading:          0
    readonly property real defaultAltitudeWGS84:    0
    readonly property real defaultGroundSpeed:      0
    readonly property real defaultAirSpeed:         0
    readonly property real defaultClimbRate:        0

    property real roll:             activeVehicle ? (isNaN(activeVehicle.roll) ? defaultRoll : activeVehicle.roll) : defaultRoll
    property real pitch:            activeVehicle ? (isNaN(activeVehicle.pitch) ? defaultPitch : activeVehicle.pitch) : defaultPitch
    property real latitude:         activeVehicle ? ((activeVehicle.latitude  === 0) ? defaultLatitude : activeVehicle.latitude) : defaultLatitude
    property real longitude:        activeVehicle ? ((activeVehicle.longitude === 0) ? defaultlongitude : activeVehicle.longitude) : defaultLongitude
    property real heading:          activeVehicle ? (isNaN(activeVehicle.heading) ? defaultHeading : activeVehicle.heading) : defaultHeading
    property real altitudeWGS84:    activeVehicle ? activeVehicle.altitudeWGS84 : defaultAltitudeWGS84
    property real groundSpeed:      activeVehicle ? activeVehicle.groundSpeed : defaultGroundSpeed
    property real airSpeed:         activeVehicle ? activeVehicle.airSpeed : defaultAirSpeed
    property real climbRate:        activeVehicle ? activeVehicle.climbRate : defaultClimbRate

    property bool showPitchIndicator:       true

    function getBool(value) {
        return value === '0' ? false : true;
    }

    function setBool(value) {
        return value ? "1" : "0";
    }

    function enforceExclusiveOption(setWidget, alternateWidget, defaultSetting, alternateSetting) {
        setWidget.visible = !setWidget.visible;
        flightDisplay.saveSetting(defaultSetting, setBool(setWidget.visible));
        alternateWidget.visible = setWidget.visible ? false : alternateWidget.visible;
        flightDisplay.saveSetting(alternateSetting, setBool(alternateWidget.visible));
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
        mapBackground.alwaysNorth           = getBool(flightDisplay.loadSetting("mapAlwaysPointsNorth",     "0"));
        videoBackground.visible             = getBool(flightDisplay.loadSetting("showVideoBackground",      "0"));
        showPitchIndicator                  = getBool(flightDisplay.loadSetting("showPitchIndicator",       "1"));
        compassWidget.visible               = getBool(flightDisplay.loadSetting("showCompassWidget",        "0"));
        compassHUD.visible                  = getBool(flightDisplay.loadSetting("showCompassHUD",           "1"));
        attitudeWidget.visible              = getBool(flightDisplay.loadSetting("showAttitudeWidget",       "0"));
        attitudeHUD.visible                 = getBool(flightDisplay.loadSetting("showAttitudeHUD",          "1"));
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
        // Compass HUD or Widget. Not both:
        if(compassWidget.visible && compassHUD.visible) {
            compassWidget.visible = false;
            flightDisplay.saveSetting("showCompassWidget", setBool(compassWidget.visible));
        }
        // Attitude HUD or Widget. Not both:
        if(attitudeWidget.visible && attitudeHUD.visible) {
            attitudeWidget.visible = false;
            flightDisplay.saveSetting("showAttitudeWidget", setBool(attitudeWidget.visible));
        }
        // Disable video if we don't have support for it
        if(!flightDisplay.hasVideo) {
            videoBackground.visible = false;
            flightDisplay.saveSetting("showVideoBackground", setBool(videoBackground.visible));
        }
        // Enable/Disable menu accordingly
        videoMenu.enabled = flightDisplay.hasVideo;
    }

    Menu {
        id: contextMenu

        MenuItem {
            text: "Map Background"
            checkable: true
            checked: mapBackground.visible
            onTriggered:
            {
                enforceExclusiveOption(mapBackground, videoBackground, "showMapBackground", "showVideoBackground");
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
            id: videoMenu
            text: "Video Background"
            checkable: true
            checked: videoBackground.visible
            onTriggered:
            {
                enforceExclusiveOption(videoBackground, mapBackground, "showVideoBackground", "showMapBackground");
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Attitude Widget"
            checkable:  true
            checked:    attitudeWidget.visible
            onTriggered:
            {
                enforceExclusiveOption(attitudeWidget, attitudeHUD, "showAttitudeWidget", "showAttitudeHUD");
            }
        }

        MenuItem {
            text: "Attitude HUD"
            checkable:  true
            checked:    attitudeHUD.visible
            onTriggered:
            {
                enforceExclusiveOption(attitudeHUD, attitudeWidget, "showAttitudeHUD", "showAttitudeWidget");
            }
        }

        MenuItem {
            text: "Pitch Indicator"
            checkable:  true
            checked:    showPitchIndicator
            enabled:    attitudeHUD.visible || attitudeWidget.visible
            onTriggered:
            {
                showPitchIndicator = !showPitchIndicator;
                flightDisplay.saveSetting("showPitchIndicator", setBool(showPitchIndicator));
            }
        }

        MenuItem {
            text: "Compass Widget"
            checkable: true
            checked: compassWidget.visible
            onTriggered:
            {
                enforceExclusiveOption(compassWidget, compassHUD, "showCompassWidget", "showCompassHUD");
            }
        }

        MenuItem {
            text: "Compass HUD"
            checkable: true
            checked: compassHUD.visible
            onTriggered:
            {
                enforceExclusiveOption(compassHUD, compassWidget, "showCompassHUD", "showCompassWidget");
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
                attitudeWidget.visible = false;
                flightDisplay.saveSetting("showAttitudeWidget", setBool(attitudeWidget.visible));
                attitudeHUD.visible = true;
                flightDisplay.saveSetting("showAttitudeHUD", setBool(attitudeHUD.visible));
                compassWidget.visible = false
                flightDisplay.saveSetting("showCompassWidget", setBool(compassWidget.visible));
                compassHUD.visible = true
                flightDisplay.saveSetting("showCompassHUD", setBool(compassHUD.visible));
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

    // Video and Map backgrounds are exclusive. If one is enabled the other is disabled.

    QGCVideoBackground {
        id:                 videoBackground
        anchors.fill:       parent
        display:            videoDisplay
        receiver:           videoReceiver
        z:                  10
    }

    FlightMap {
        id:                 mapBackground
        anchors.fill:       parent
        mapName:            'MainFlightDisplay'
        latitude:           mapBackground.visible ? root.latitude : root.defaultLatitude
        longitude:          mapBackground.visible ? root.longitude : root.defaultLongitude
        readOnly:           true
        z:                  10
    }

    // Floating (Top Left) Compass Widget

    QGCCompassWidget {
        id:                 compassWidget
        y:                  ScreenTools.defaultFontPixelSize * (0.42)
        x:                  ScreenTools.defaultFontPixelSize * (7.1)
        size:               ScreenTools.defaultFontPixelSize * (13.3)
        heading:            root.heading
        z:                  mapBackground.z + 2
        onResetRequested: {
            y               = ScreenTools.defaultFontPixelSize * (0.42)
            x               = ScreenTools.defaultFontPixelSize * (7.1)
            size            = ScreenTools.defaultFontPixelSize * (13.3)
            tForm.xScale    = 1
            tForm.yScale    = 1
        }
    }

    // HUD (lower middle) Compass

    QGCCompassHUD {
        id:                 compassHUD
        y:                  root.height * 0.7
        x:                  root.width  * 0.5 - ScreenTools.defaultFontPixelSize * (5)
        width:              ScreenTools.defaultFontPixelSize * (10)
        height:             ScreenTools.defaultFontPixelSize * (10)
        heading:            root.heading
        z:                  70
    }

    // Sky/Ground background (visible when no Map or Video is enabled)

    QGCArtificialHorizon {
        id:                 artificialHoriz
        anchors.fill:       parent
        rollAngle:          roll
        pitchAngle:         pitch
        visible:            !videoBackground.visible && !mapBackground.visible
    }

    // Floating (Top Right) Attitude Widget

    QGCAttitudeWidget {
        id:                 attitudeWidget
        y:                  ScreenTools.defaultFontPixelSize * (0.42)
        size:               ScreenTools.defaultFontPixelSize * (13.3)
        rollAngle:          roll
        pitchAngle:         pitch
        showPitch:          showPitchIndicator
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

    // HUD (center) Attitude Indicator

    QGCAttitudeHUD {
        id:                 attitudeHUD
        rollAngle:          roll
        pitchAngle:         pitch
        showPitch:          showPitchIndicator
        width:              ScreenTools.defaultFontPixelSize * (30)
        height:             ScreenTools.defaultFontPixelSize * (30)
        z:                  20
    }

    QGCAltitudeWidget {
        id:                 altitudeWidget
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (5)
        height:             parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        altitude:           root.altitudeWGS84
        z:                  30
    }

    QGCSpeedWidget {
        id:                 speedWidget
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (5)
        height:             parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        speed:              root.groundSpeed
        z:                  40
    }

    QGCCurrentSpeed {
        id: currentSpeed
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        airspeed:           root.airSpeed
        groundspeed:        root.groundSpeed
        showAirSpeed:       true
        showGroundSpeed:    true
        visible:            (currentSpeed.showGroundSpeed || currentSpeed.showAirSpeed)
        z:                  50
    }

    QGCCurrentAltitude {
        id: currentAltitude
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        altitude:           root.altitudeWGS84
        vertZ:              root.climbRate
        showAltitude:       true
        showClimbRate:      true
        visible:            (currentAltitude.showAltitude || currentAltitude.showClimbRate)
        z:                  60
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
