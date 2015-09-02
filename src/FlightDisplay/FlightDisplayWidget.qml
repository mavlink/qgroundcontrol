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

import QtQuick                  2.4
import QtQuick.Controls         1.3
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Flight Display Widget
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
    property real longitude:        activeVehicle ? ((activeVehicle.longitude === 0) ? defaultLongitude : activeVehicle.longitude) : defaultLongitude
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
        videoBackground.visible = getBool(flightDisplay.loadSetting("showVideoBackground", "0"));

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
            id: videoMenu
            text: "Video Background"
            checkable: true
            checked: videoBackground.visible
            onToggled: videoBackground.visible = checked
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

    // HUD (lower middle) Compass

    QGCCompassHUD {
        id:                 compassHUD
        y:                  root.height * 0.7
        x:                  root.width  * 0.5 - ScreenTools.defaultFontPixelSize * (5)
        width:              ScreenTools.defaultFontPixelSize * (10)
        height:             ScreenTools.defaultFontPixelSize * (10)
        heading:            root.heading
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  70
    }

    // Sky/Ground background (visible when no Map or Video is enabled)

    QGCArtificialHorizon {
        id:                 artificialHoriz
        anchors.fill:       parent
        rollAngle:          roll
        pitchAngle:         pitch
        visible:            !videoBackground.visible
    }

    // HUD (center) Attitude Indicator

    QGCAttitudeHUD {
        id:                 attitudeHUD
        rollAngle:          roll
        pitchAngle:         pitch
        showPitch:          showPitchIndicator
        width:              ScreenTools.defaultFontPixelSize * (30)
        height:             ScreenTools.defaultFontPixelSize * (30)
        active:             multiVehicleManager.activeVehicleAvailable
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
        active:             multiVehicleManager.activeVehicleAvailable
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
        active:             multiVehicleManager.activeVehicleAvailable
        visible:            (currentAltitude.showAltitude || currentAltitude.showClimbRate)
        z:                  60
    }

    QGCButton {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.right:      parent.right
        anchors.bottom:     parent.bottom
        text:               "Options"
        menu:               contextMenu
    }
}
