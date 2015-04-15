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

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FlightControls 1.0

Rectangle {
    id: root
    color: Qt.rgba(0,0,0,0);

    property real roll:    isNaN(flightDisplay.roll)    ? 0 : flightDisplay.roll
    property real pitch:   isNaN(flightDisplay.pitch)   ? 0 : flightDisplay.pitch

    property bool showPitchIndicator:       true
    property bool showAttitudeIndicator:    true
    property bool showCompass:              true

    function getBool(value) {
        return value === '0' ? false : true;
    }

    function setBool(value) {
        return value ? "1" : "0";
    }

    function adjustSizes() {
        var dist = 85
        var wide = 160
        var minw = 496
        if(root.width > minw)
        {
            attitudeInstrument.size = wide;
            attitudeInstrument.x    = dist
            compassInstrument.size  = wide;
            compassInstrument.x     = root.width - wide - dist
        } else {
            var factor = (root.width / minw);
            var ndist  = dist * factor;
            var nwide  = wide * factor;
            if (ndist < 0)
                ndist = 0;
            attitudeInstrument.size = nwide;
            compassInstrument.size  = nwide;
            attitudeInstrument.x    = ndist;
            compassInstrument.x     = root.width - nwide - ndist;
        }
    }

    Component.onCompleted:
    {
        mapBackground.visible               = getBool(flightDisplay.loadSetting("showMapBackground",        "0"));
        mapBackground.alwaysNorth           = getBool(flightDisplay.loadSetting("mapAlwaysPointsNorth",     "0"));
        showAttitudeIndicator               = getBool(flightDisplay.loadSetting("showAttitudeIndicator",    "1"));
        showPitchIndicator                  = getBool(flightDisplay.loadSetting("showPitchIndicator",       "1"));
        showCompass                         = getBool(flightDisplay.loadSetting("showCompass",              "1"));
        altitudeWidget.visible              = getBool(flightDisplay.loadSetting("showAltitudeWidget",       "1"));
        speedWidget.visible                 = getBool(flightDisplay.loadSetting("showSpeedWidget",          "1"));
        currentSpeed.showAirSpeed           = getBool(flightDisplay.loadSetting("showCurrentAirSpeed",      "1"));
        currentSpeed.showGroundSpeed        = getBool(flightDisplay.loadSetting("showCurrentGroundSpeed",   "1"));
        currentAltitude.showClimbRate       = getBool(flightDisplay.loadSetting("showCurrentClimbRate",     "1"));
        currentAltitude.showAltitude        = getBool(flightDisplay.loadSetting("showCurrentAltitude",      "1"));
        mapTypeMenu.update();
        adjustSizes();
    }

    Menu {
        id: contextMenu

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

        /*
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
            }
        }

    }

    QGCMapBackground {
        id:                 mapBackground
        anchors.fill:       parent
        heading:            0 // isNaN(flightDisplay.heading) ? 0 : flightDisplay.heading
        latitude:           mapBackground.visible ? ((flightDisplay.latitude  === 0) ?   37.803784 : flightDisplay.latitude)  :   37.803784
        longitude:          mapBackground.visible ? ((flightDisplay.longitude === 0) ? -122.462276 : flightDisplay.longitude) : -122.462276
        interactive:        !flightDisplay.mavPresent
        z:                  10
    }

    QGCAttitudeWidget {
        id:                 attitudeWidget
        anchors.centerIn:   parent
        rollAngle:          roll
        pitchAngle:         pitch
        showAttitude:       showAttitudeIndicator
        visible:            !mapBackground.visible
        z:                  10
    }

    QGCPitchWidget {
        id:                 pitchWidget
        visible:            showPitchIndicator && !mapBackground.visible
        anchors.verticalCenter: parent.verticalCenter
        pitchAngle:         pitch
        rollAngle:          roll
        color:              Qt.rgba(0,0,0,0)
        size:               120
        z:                  30
    }

    QGCAltitudeWidget {
        id:                 altitudeWidget
        anchors.right:      parent.right
        width:              60
        altitude:           flightDisplay.altitudeWGS84
        z:                  30
    }

    QGCSpeedWidget {
        id:                 speedWidget
        anchors.left:       parent.left
        width:              60
        speed:              flightDisplay.groundSpeed
        z:                  40
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
        id:                 compassIndicator
        y:                  root.height * 0.7
        x:                  root.width  * 0.5 - 60
        width:              120
        height:             120
        heading:            isNaN(flightDisplay.heading) ? 0 : flightDisplay.heading
        visible:            !mapBackground.visible && showCompass
        z:                  70
    }

    QGCCompassInstrument {
        id:                 compassInstrument
        y:                  5
        x:                  85
        size:               160
        heading:            isNaN(flightDisplay.heading) ? 0 : flightDisplay.heading
        visible:            mapBackground.visible && showCompass
        z:                  70
    }

    QGCAttitudeInstrument {
        id:                 attitudeInstrument
        y:                  5
        x:                  root.width - 160 - 85
        size:               160
        rollAngle:          roll
        pitchAngle:         pitch
        showPitch:          showPitchIndicator
        visible:            mapBackground.visible && showAttitudeIndicator
        z:                  80
    }

    // Button at upper left corner
    Item {
        id:             optionsButton
        x:              5
        y:              5
        width:          30
        height:         30
        opacity:        0.85
        z:              1000
        Image {
            id:             buttomImg
            anchors.fill:   parent
            source:         "/qml/buttonMore.svg"
            mipmap:         true
            smooth:         true
            antialiasing:   true
            fillMode:       Image.PreserveAspectFit
        }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                if (mouse.button == Qt.LeftButton)
                {
                    contextMenu.popup()
                }
            }
        }
    }

    onWidthChanged: {
        adjustSizes();
    }
}
