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

    function getBool(value) {
        return value === '0' ? false : true;
    }

    function setBool(value) {
        return value ? "1" : "0";
    }

    Component.onCompleted:
    {
        mapBackground.visible               = getBool(flightDisplay.loadSetting("showMapBackground",        "0"));
        mapBackground.alwaysNorth           = getBool(flightDisplay.loadSetting("mapAlwaysPointsNorth",     "0"));
        attitudeWidget.visible              = getBool(flightDisplay.loadSetting("showAttitudeWidget",       "1"));
        attitudeWidget.displayBackground    = getBool(flightDisplay.loadSetting("showAttitudeBackground",   "1"));
        pitchWidget.visible                 = getBool(flightDisplay.loadSetting("showPitchWidget",          "1"));
        altitudeWidget.visible              = getBool(flightDisplay.loadSetting("showAltitudeWidget",       "1"));
        speedWidget.visible                 = getBool(flightDisplay.loadSetting("showSpeedWidget",          "1"));
        compassIndicator.visible            = getBool(flightDisplay.loadSetting("showCompassIndicator",     "1"));
        currentSpeed.showAirSpeed           = getBool(flightDisplay.loadSetting("showCurrentAirSpeed",      "1"));
        currentSpeed.showGroundSpeed        = getBool(flightDisplay.loadSetting("showCurrentGroundSpeed",   "1"));
        currentAltitude.showClimbRate       = getBool(flightDisplay.loadSetting("showCurrentClimbRate",     "1"));
        currentAltitude.showAltitude        = getBool(flightDisplay.loadSetting("showCurrentAltitude",      "1"));
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
        id: contextMenu

        MenuItem {
            text: "Main Attitude Indicators"
            checkable: true
            checked: attitudeWidget.visible
            onTriggered:
            {
                attitudeWidget.visible = !attitudeWidget.visible;
                flightDisplay.saveSetting("showAttitudeWidget", setBool(attitudeWidget.visible));
            }
        }

        MenuItem {
            text: "Display Attitude Background"
            checkable: true
            checked: attitudeWidget.displayBackground
            onTriggered:
            {
                attitudeWidget.displayBackground = !attitudeWidget.displayBackground;
                flightDisplay.saveSetting("showAttitudeBackground", setBool(attitudeWidget.displayBackground));
            }
        }

        MenuItem {
            text: "Pitch Indicator"
            checkable: true
            checked: pitchWidget.visible
            onTriggered:
            {
                pitchWidget.visible = !pitchWidget.visible;
                flightDisplay.saveSetting("showPitchWidget", setBool(pitchWidget.visible));
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

        MenuItem {
            text: "Compass"
            checkable: true
            checked: compassIndicator.visible
            onTriggered:
            {
                compassIndicator.visible = !compassIndicator.visible;
                flightDisplay.saveSetting("showCompassIndicator", setBool(compassIndicator.visible));
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
                flightDisplay.saveSetting("showMapBackground", setBool(mapBackground.visible));
            }
        }

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

        Menu {
            id: mapTypeMenu
            title: "Map Type..."
            ExclusiveGroup { id: currentMapType }
            function setCurrentMap(map) {
                for (var i = 0; i < mapBackground.mapItem.supportedMapTypes.length; i++) {
                    if (map === mapBackground.mapItem.supportedMapTypes[i].name) {
                        mapBackground.mapItem.activeMapType = mapBackground.mapItem.supportedMapTypes[i]
                        flightDisplay.saveSetting("currentMapType", map);
                        return;
                    }
                }
            }
            function addMap(map, checked) {
                var mItem = mapTypeMenu.addItem(map);
                mItem.checkable = true
                mItem.checked   = checked
                mItem.exclusiveGroup = currentMapType
                var menuSlot = function() {setCurrentMap(map);};
                mItem.triggered.connect(menuSlot);
            }
            function update() {
                clear()
                var map = ''
                if (mapBackground.mapItem.supportedMapTypes.length > 0)
                    map = mapBackground.mapItem.activeMapType.name;
                map = flightDisplay.loadSetting("currentMapType", map);
                for (var i = 0; i < mapBackground.mapItem.supportedMapTypes.length; i++) {
                    var name = mapBackground.mapItem.supportedMapTypes[i].name;
                    addMap(name, map === name);
                }
                if(map != '')
                    setCurrentMap(map);
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Restore Defaults"
            onTriggered:
            {
                attitudeWidget.visible = true;
                flightDisplay.saveSetting("showAttitudeWidget", setBool(attitudeWidget.visible));
                attitudeWidget.displayBackground = true;
                flightDisplay.saveSetting("showAttitudeBackground", setBool(attitudeWidget.displayBackground));
                pitchWidget.visible = true;
                flightDisplay.saveSetting("showPitchWidget", setBool(pitchWidget.visible));
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
                compassIndicator.visible = true;
                flightDisplay.saveSetting("showCompassIndicator", setBool(compassIndicator.visible));
                mapBackground.visible = false;
                flightDisplay.saveSetting("showMapBackground", setBool(mapBackground.visible));
                mapBackground.alwaysNorth = false;
                flightDisplay.saveSetting("mapAlwaysPointsNorth", setBool(mapBackground.alwaysNorth));
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
        anchors.centerIn:   parent
        rollAngle:          roll
        pitchAngle:         pitch
        useWhite:           !mapBackground.visible
        backgroundOpacity:  mapBackground.visible ? 0.25 : 1.0
        z:                  10
    }

    QGCPitchWidget {
        id: pitchWidget
        anchors.verticalCenter: parent.verticalCenter
        pitchAngle: pitch
        rollAngle:  roll
        color:      mapBackground.visible ? Qt.rgba(0,0,0,0.5) : Qt.rgba(0,0,0,0)
        opacity:    mapBackground.visible ? 1 : 0.75
        z:          mapBackground.visible ? 20 : 25
    }

    Image {
        anchors.centerIn: parent
        source:   "/qml/crossHair.svg"
        mipmap:   true
        width:    260
        fillMode: Image.PreserveAspectFit
        z:        mapBackground.visible ? 25 : 20
    }

    QGCAltitudeWidget {
        id: altitudeWidget
        anchors.right: parent.right
        width:     60
        altitude:  flightDisplay.altitudeWGS84
        z:         30
    }

    QGCSpeedWidget {
        id: speedWidget
        anchors.left: parent.left
        width:  60
        speed:  flightDisplay.groundSpeed
        z:      40
    }

    QGCCurrentSpeed {
        id: currentSpeed
        anchors.left:       parent.left
        width:              75
        airspeed:           flightDisplay.airSpeed
        groundspeed:        flightDisplay.groundSpeed
        showAirSpeed:       true
        showGroundSpeed:    true
        visible:            (currentSpeed.showGroundSpeed || currentSpeed.showAirSpeed)
        z:                  50
    }

    QGCCurrentAltitude {
        id: currentAltitude
        anchors.right:      parent.right
        width:              75
        altitude:           flightDisplay.altitudeWGS84
        vertZ:              flightDisplay.climbRate
        showAltitude:       true
        showClimbRate:      true
        visible:            (currentAltitude.showAltitude || currentAltitude.showClimbRate)
        z:                  60
    }

    QGCCompass {
        id: compassIndicator
        y: root.height * 0.7
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
