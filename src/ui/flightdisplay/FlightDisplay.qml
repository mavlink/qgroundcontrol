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
 *   @brief QGC Main Tool Bar
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.HUDControls 1.0

Rectangle {
    id: root
    color: Qt.rgba(0,0,0,0);

    property real roll:    isNaN(flightDisplay.roll)    ? 0 : flightDisplay.roll
    property real pitch:   isNaN(flightDisplay.pitch)   ? 0 : flightDisplay.pitch

    Component.onCompleted:
    {
        mapBackground.visible               = flightDisplay.loadSetting("enableMapBackground", false);
        mapBackground.alwaysNorth           = flightDisplay.loadSetting("mapAlwaysNorth", false);
        attitudeWidget.visible              = flightDisplay.loadSetting("enableattitudeWidget", true);
        attitudeWidget.displayBackground    = flightDisplay.loadSetting("displayRollPitchBackground", true);
        pitchWidget.visible                 = flightDisplay.loadSetting("enablepitchWidget", true);
        altitudeWidget.visible              = flightDisplay.loadSetting("enablealtitudeWidget", true);
        speedWidget.visible                 = flightDisplay.loadSetting("enablespeedWidget", true);
        compassIndicator.visible            = flightDisplay.loadSetting("enableCompassIndicator", true);
        currentSpeed.showAirSpeed           = flightDisplay.loadSetting("showAirSpeed", true);
        currentSpeed.showGroundSpeed        = flightDisplay.loadSetting("showGroundSpeed", true);
        currentAltitude.showClimbRate       = flightDisplay.loadSetting("showCurrentClimbRate", true);
        currentAltitude.showAltitude        = flightDisplay.loadSetting("showCurrentAltitude", true);
        mapTypeMenu.update();
    }

    Rectangle {
        id: windowBackground
        anchors.fill: parent
        anchors.centerIn: parent
        visible: !attitudeWidget.visible && !mapBackground.visible
        color:   Qt.hsla(0.25, 0.5, 0.45)
        z:       0
    }

    Menu {
        id: mapTypeMenu
        function setCurrentMap(map) {
            for (var i = 0; i < mapBackground.mapItem.supportedMapTypes.length; i++) {
                if (map === mapBackground.mapItem.supportedMapTypes[i].name) {
                    mapBackground.mapItem.activeMapType = mapBackground.mapItem.supportedMapTypes[i]
                    return;
                }
            }
        }
        function addMap(map) {
            var mItem = mapTypeMenu.addItem(map);
            var menuSlot = function() {setCurrentMap(map);};
            mItem.triggered.connect(menuSlot);
        }
        function update() {
            clear()
            for (var i = 0; i < mapBackground.mapItem.supportedMapTypes.length; i++) {
                addMap(mapBackground.mapItem.supportedMapTypes[i].name);
            }
            if (mapBackground.mapItem.supportedMapTypes.length > 0)
                setCurrentMap(mapBackground.mapItem.activeMapType.name);
        }
    }

    Menu {
        id: contextMenu

        MenuItem {
            text: "Main Attitude Indicators"
            checkable: true
            checked: attitudeWidget.visible
            onTriggered:
            {
                attitudeWidget.visible = !attitudeWidget.visible;
                flightDisplay.saveSetting("enableattitudeWidget", attitudeWidget.visible);
            }
        }

        MenuItem {
            text: "Display Attitude Background"
            checkable: true
            checked: attitudeWidget.displayBackground
            onTriggered:
            {
                attitudeWidget.displayBackground = !attitudeWidget.displayBackground;
                flightDisplay.saveSetting("displayRollPitchBackground", attitudeWidget.displayBackground);
            }
        }

        MenuItem {
            text: "Pitch Indicator"
            checkable: true
            checked: pitchWidget.visible
            onTriggered:
            {
                pitchWidget.visible = !pitchWidget.visible;
                flightDisplay.saveSetting("enablepitchWidget", pitchWidget.visible);
            }
        }

        MenuItem {
            text: "Altitude Indicator"
            checkable: true
            checked: altitudeWidget.visible
            onTriggered:
            {
                altitudeWidget.visible = !altitudeWidget.visible;
                flightDisplay.saveSetting("enablealtitudeWidget", altitudeWidget.visible);
            }
        }

        MenuItem {
            text: "Current Altitude"
            checkable: true
            checked: currentAltitude.showAltitude
            onTriggered:
            {
                currentAltitude.showAltitude = !currentAltitude.showAltitude;
                flightDisplay.saveSetting("showCurrentAltitude", currentAltitude.showAltitude);
            }
        }

        MenuItem {
            text: "Current Climb Rate"
            checkable: true
            checked: currentAltitude.showClimbRate
            onTriggered:
            {
                currentAltitude.showClimbRate = !currentAltitude.showClimbRate;
                flightDisplay.saveSetting("showCurrentClimbRate", currentAltitude.showClimbRate);
            }
        }

        MenuItem {
            text: "Speed Indicator"
            checkable: true
            checked: speedWidget.visible
            onTriggered:
            {
                speedWidget.visible = !speedWidget.visible;
                flightDisplay.saveSetting("enablespeedWidget", speedWidget.visible);
            }
        }

        MenuItem {
            text: "Current Air Speed"
            checkable: true
            checked: currentSpeed.showAirSpeed
            onTriggered:
            {
                currentSpeed.showAirSpeed = !currentSpeed.showAirSpeed;
                flightDisplay.saveSetting("showAirSpeed", currentSpeed.showAirSpeed);
            }
        }

        MenuItem {
            text: "Current Ground Speed"
            checkable: true
            checked: currentSpeed.showGroundSpeed
            onTriggered:
            {
                currentSpeed.showGroundSpeed = !currentSpeed.showGroundSpeed;
                flightDisplay.saveSetting("showGroundSpeed", currentSpeed.showGroundSpeed);
            }
        }

        MenuItem {
            text: "Compass"
            checkable: true
            checked: compassIndicator.visible
            onTriggered:
            {
                compassIndicator.visible = !compassIndicator.visible;
                flightDisplay.saveSetting("enableCompassIndicator", compassIndicator.visible);
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Map Background"
            checkable: true
            checked: mapBackground.visible
            onTriggered:
            {
                mapBackground.visible = !mapBackground.visible;
                flightDisplay.saveSetting("enableMapBackground", mapBackground.visible);
            }
        }

        MenuItem {
            text: "Map Always Points North"
            checkable: true
            checked: mapBackground.alwaysNorth
            onTriggered:
            {
                mapBackground.alwaysNorth = !mapBackground.alwaysNorth;
                flightDisplay.saveSetting("mapAlwaysNorth", mapBackground.alwaysNorth);
            }
        }

        MenuItem {
            text: "Map Type..."
            checkable: false
            onTriggered:
            {
                mapTypeMenu.popup();
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Restore Defaults"
            onTriggered:
            {
                attitudeWidget.visible = true;
                flightDisplay.saveSetting("enableattitudeWidget", attitudeWidget.visible);
                attitudeWidget.displayBackground = true;
                flightDisplay.saveSetting("displayRollPitchBackground", attitudeWidget.displayBackground);
                pitchWidget.visible = true;
                flightDisplay.saveSetting("enablepitchWidget", pitchWidget.visible);
                altitudeWidget.visible = true;
                flightDisplay.saveSetting("enablealtitudeWidget", altitudeWidget.visible);
                currentAltitude.showAltitude = true;
                flightDisplay.saveSetting("showCurrentAltitude", currentAltitude.showAltitude);
                currentAltitude.showClimbRate = true;
                flightDisplay.saveSetting("showCurrentClimbRate", currentAltitude.showClimbRate);
                speedWidget.visible = true;
                flightDisplay.saveSetting("enablespeedWidget", speedWidget.visible);
                currentSpeed.showAirSpeed = true;
                flightDisplay.saveSetting("showAirSpeed", currentSpeed.showAirSpeed);
                currentSpeed.showGroundSpeed = true;
                flightDisplay.saveSetting("showGroundSpeed", currentSpeed.showGroundSpeed);
                compassIndicator.visible = true;
                flightDisplay.saveSetting("enableCompassIndicator", compassIndicator.visible);
                mapBackground.visible = false;
                flightDisplay.saveSetting("enableMapBackground", mapBackground.visible);
                mapBackground.alwaysNorth = false;
                flightDisplay.saveSetting("mapAlwaysNorth", mapBackground.alwaysNorth);
            }
        }

    }

    QGCMapBackground {
        id: mapBackground
        anchors.centerIn: parent
        visible:   false
        heading:   isNaN(flightDisplay.heading) ? 0 : flightDisplay.heading
        latitude:  flightDisplay.latitude
        longitude: flightDisplay.longitude
        z: 5
    }

    QGCAttitudeWidget {
        id: attitudeWidget
        anchors.centerIn: parent
        rollAngle:  roll
        pitchAngle: pitch
        backgroundOpacity: mapBackground.visible ? 0.25 : 1.0
        z:          10
    }

    QGCPitchWidget {
        id: pitchWidget
        anchors.verticalCenter: parent.verticalCenter
        opacity:    0.5
        pitchAngle: pitch
        rollAngle:  roll
        z:          20
    }

    QGCAltitudeWidget {
        id: altitudeWidget
        anchors.right: parent.right
        width:     40
        altitude:  flightDisplay.altitudeWGS84
        z:         30
    }

    QGCSpeedWidget {
        id: speedWidget
        anchors.left: parent.left
        width:  40
        speed:  flightDisplay.groundSpeed
        z:      40
    }

    QGCCurrentSpeed {
        id: currentSpeed
        anchors.left:       parent.left
        width:              80
        airspeed:           flightDisplay.airSpeed
        groundspeed:        flightDisplay.groundSpeed
        showAirSpeed:       true
        showGroundSpeed:    true
        visible:            currentSpeed.showGroundSpeed || currentSpeed.showAirSpeed
        z:                  50
    }

    QGCCurrentAltitude {
        id: currentAltitude
        anchors.right:  parent.right
        width:          80
        altitude:       flightDisplay.altitudeWGS84
        vertZ:          flightDisplay.climbRate
        showAltitude:   true
        showClimbRate:  true
        visible:        (currentAltitude.showAltitude || currentAltitude.showClimbRate)
        z:              60
    }

    QGCCompass {
        id: compassIndicator
        y: root.height * 0.65
        anchors.horizontalCenter: parent.horizontalCenter
        heading: isNaN(flightDisplay.heading) ? 0 : flightDisplay.heading
        z:       70
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            if (mouse.button == Qt.RightButton)
            {
                contextMenu.popup()
            }
        }
        z: 100
    }

}

