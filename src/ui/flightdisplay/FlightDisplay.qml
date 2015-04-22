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
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2

import QGroundControl.FlightControls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    id: root

    property ScreenTools __screenTools: ScreenTools { }
    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

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
    }

    // TODO: This is to replace the context menu but it is not working. Not only the buttons don't show,
    // the default placement is random and mostly off screen on mobile devices.
    Dialog {
        id: optionsDialog
        modality: Qt.WindowModal
        title: "Flight Display Options"
        standardButtons: StandardButton.Close | StandardButton.RestoreDefaults
        onReset: {
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
        contentItem: Rectangle {
            color: __qgcPal.window
            implicitWidth:  __screenTools.pixelSizeFactor * (360)
            implicitHeight: __screenTools.pixelSizeFactor * (300)
            Column {
                id: dialogColumn
                anchors.centerIn: parent
                spacing:  __screenTools.adjustPixelSize(10)
                width: parent.width
                Grid {
                    columns: 2
                    spacing:    __screenTools.pixelSizeFactor * (8)
                    rowSpacing: __screenTools.pixelSizeFactor * (10)
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCCheckBox {
                        text: "Map Background"
                        checked: mapBackground.visible
                        onClicked:
                        {
                            mapBackground.visible = !mapBackground.visible;
                            flightDisplay.saveSetting("showMapBackground", setBool(mapBackground.visible));
                        }
                    }
                    QGCCheckBox {
                        text: "Pitch Indicator"
                        checked: showPitchIndicator
                        onClicked:
                        {
                            showPitchIndicator = !showPitchIndicator;
                            flightDisplay.saveSetting("showPitchIndicator", setBool(showPitchIndicator));
                        }
                    }
                    QGCCheckBox {
                        text: "Attitude Indicator"
                        checked: showAttitudeIndicator
                        onClicked:
                        {
                            showAttitudeIndicator = !showAttitudeIndicator;
                            flightDisplay.saveSetting("showAttitudeIndicator", setBool(showAttitudeIndicator));
                        }
                    }
                    QGCCheckBox {
                        text: "Compass"
                        checked: showCompass
                        onClicked:
                        {
                            showCompass = !showCompass;
                            flightDisplay.saveSetting("showCompass", setBool(showCompass));
                        }
                    }
                    QGCCheckBox {
                        text: "Altitude Indicator"
                        checked: altitudeWidget.visible
                        onClicked:
                        {
                            altitudeWidget.visible = !altitudeWidget.visible;
                            flightDisplay.saveSetting("showAltitudeWidget", setBool(altitudeWidget.visible));
                        }
                    }
                    QGCCheckBox {
                        text: "Current Altitude"
                        checked: currentAltitude.showAltitude
                        onClicked:
                        {
                            currentAltitude.showAltitude = !currentAltitude.showAltitude;
                            flightDisplay.saveSetting("showCurrentAltitude", setBool(currentAltitude.showAltitude));
                        }
                    }
                    QGCCheckBox {
                        text: "Current Climb Rate"
                        checked: currentAltitude.showClimbRate
                        onClicked:
                        {
                            currentAltitude.showClimbRate = !currentAltitude.showClimbRate;
                            flightDisplay.saveSetting("showCurrentClimbRate", setBool(currentAltitude.showClimbRate));
                        }
                    }
                    QGCCheckBox {
                        text: "Speed Indicator"
                        checked: speedWidget.visible
                        onClicked:
                        {
                            speedWidget.visible = !speedWidget.visible;
                            flightDisplay.saveSetting("showSpeedWidget", setBool(speedWidget.visible));
                        }
                    }
                    QGCCheckBox {
                        text: "Current Air Speed"
                        checked: currentSpeed.showAirSpeed
                        onClicked:
                        {
                            currentSpeed.showAirSpeed = !currentSpeed.showAirSpeed;
                            flightDisplay.saveSetting("showCurrentAirSpeed", setBool(currentSpeed.showAirSpeed));
                        }
                    }
                    QGCCheckBox {
                        text: "Current Ground Speed"
                        checked: currentSpeed.showGroundSpeed
                        onClicked:
                        {
                            currentSpeed.showGroundSpeed = !currentSpeed.showGroundSpeed;
                            flightDisplay.saveSetting("showCurrentGroundSpeed", setBool(currentSpeed.showGroundSpeed));
                        }
                    }
                }
                //-- Hack tool to find optimal scale factor
                Column {
                    id: fudgeColumn
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing:    __screenTools.adjustPixelSize(4)
                    width:      parent.width
                    QGCLabel {
                        text: "Adjust Pixel Size Factor"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:    __screenTools.adjustPixelSize(4)
                        anchors.horizontalCenter: parent.horizontalCenter
                        Button {
                            text: 'Inc'
                            onClicked: {
                                __screenTools.increasePixelSize()
                            }
                        }
                        Label {
                            text: __screenTools.pixelSizeFactor.toFixed(2)
                            color: __qgcPal.text
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Button {
                            text: 'Dec'
                            onClicked: {
                                __screenTools.decreasePixelSize()
                            }
                        }
                    }
                    QGCLabel {
                        text: "Adjust Font Size Factor"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:    __screenTools.adjustPixelSize(4)
                        anchors.horizontalCenter: parent.horizontalCenter
                        Button {
                            text: 'Inc'
                            onClicked: {
                                __screenTools.increaseFontSize()
                            }
                        }
                        Label {
                            text: __screenTools.fontPointFactor.toFixed(2)
                            color: __qgcPal.text
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Button {
                            text: 'Dec'
                            onClicked: {
                                __screenTools.decreaseFontSize()
                            }
                        }
                    }
                }
            }
        }
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
            text: "Options Dialog"
            onTriggered:
            {
                optionsDialog.open()
            }
        }
        */

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
            ExclusiveGroup { id: currMapType }
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
                mItem.exclusiveGroup = currMapType
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

    QGCCompassInstrument {
        id:                 compassInstrument
        y:                  __screenTools.pixelSizeFactor * (5)
        x:                  __screenTools.pixelSizeFactor * (85)
        size:               __screenTools.pixelSizeFactor * (160)
        heading:            isNaN(flightDisplay.heading) ? 0 : flightDisplay.heading
        visible:            mapBackground.visible && showCompass
        z:                  mapBackground.z + 1
        onResetRequested: {
            y               = __screenTools.pixelSizeFactor * (5)
            x               = __screenTools.pixelSizeFactor * (85)
            size            = __screenTools.pixelSizeFactor * (160)
            tForm.xScale    = 1
            tForm.yScale    = 1
        }
    }

    QGCAttitudeInstrument {
        id:                 attitudeInstrument
        y:                  __screenTools.pixelSizeFactor * (5)
        size:               __screenTools.pixelSizeFactor * (160)
        rollAngle:          roll
        pitchAngle:         pitch
        showPitch:          showPitchIndicator
        visible:            mapBackground.visible && showAttitudeIndicator
        anchors.right:      root.right
        anchors.rightMargin: __screenTools.pixelSizeFactor * (85)
        z:                  mapBackground.z + 1
        onResetRequested: {
            y                   = __screenTools.pixelSizeFactor * (5)
            anchors.right       = root.right
            anchors.rightMargin = __screenTools.pixelSizeFactor * (85)
            size                = __screenTools.pixelSizeFactor * (160)
            tForm.xScale        = 1
            tForm.yScale        = 1
        }
    }

    QGCArtificialHorizon {
        id:                 artificialHoriz
        anchors.fill:       parent
        rollAngle:          roll
        pitchAngle:         pitch
        visible:            !mapBackground.visible
        z:                  10
    }

    QGCAttitudeWidget {
        id:                 attitudeWidget
        rollAngle:          roll
        pitchAngle:         pitch
        visible:            !mapBackground.visible && showAttitudeIndicator
        width:              __screenTools.pixelSizeFactor * (260)
        height:             __screenTools.pixelSizeFactor * (260)
        z:                  20
    }

    QGCPitchWidget {
        id:                 pitchWidget
        visible:            showPitchIndicator && !mapBackground.visible
        anchors.verticalCenter: parent.verticalCenter
        pitchAngle:         pitch
        rollAngle:          roll
        color:              Qt.rgba(0,0,0,0)
        size:               __screenTools.pixelSizeFactor * (120)
        z:                  30
    }

    QGCAltitudeWidget {
        id:                 altitudeWidget
        anchors.right:      parent.right
        width:              __screenTools.pixelSizeFactor * (60)
        height:             parent.height * 0.65 > __screenTools.pixelSizeFactor * (280) ? __screenTools.pixelSizeFactor * (280) : parent.height * 0.65
        altitude:           flightDisplay.altitudeWGS84
        z:                  30
    }

    QGCSpeedWidget {
        id:                 speedWidget
        anchors.left:       parent.left
        width:              __screenTools.pixelSizeFactor * (60)
        height:             parent.height * 0.65 > __screenTools.pixelSizeFactor * (280) ? __screenTools.pixelSizeFactor * (280) : parent.height * 0.65
        speed:              flightDisplay.groundSpeed
        z:                  40
    }

    QGCCurrentSpeed {
        id: currentSpeed
        anchors.left:       parent.left
        width:              __screenTools.pixelSizeFactor * (75)
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
        width:              __screenTools.pixelSizeFactor * (75)
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
        x:                  root.width  * 0.5 - __screenTools.pixelSizeFactor * (60)
        width:              __screenTools.pixelSizeFactor * (120)
        height:             __screenTools.pixelSizeFactor * (120)
        heading:            isNaN(flightDisplay.heading) ? 0 : flightDisplay.heading
        visible:            !mapBackground.visible && showCompass
        z:                  70
    }

    // Button at upper left corner
    Item {
        id:             optionsButton
        x:              __screenTools.pixelSizeFactor * (5)
        y:              __screenTools.pixelSizeFactor * (5)
        width:          __screenTools.pixelSizeFactor * (30)
        height:         __screenTools.pixelSizeFactor * (30)
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
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: {
                if (mouse.button == Qt.LeftButton)
                {
                    contextMenu.popup();
                }
                // Experimental
                if (mouse.button == Qt.RightButton)
                {
                    optionsDialog.open();
                }
            }
        }
    }

}
