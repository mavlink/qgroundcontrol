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
import QtLocation               5.3
import QtPositioning            5.2

import QGroundControl               1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0

/// Flight Display View
Item {
    id: root

    // Top margin for all widgets. Used to prevent overlap with the toolbar
    property real   topMargin: 0

    // Used by parent to hide widgets when it displays something above in the z order.
    // Prevents z order drawing problems.
    property bool hideWidgets: false

    readonly property alias zOrderTopMost:   flightMap.zOrderTopMost
    readonly property alias zOrderWidgets:   flightMap.zOrderWidgets
    readonly property alias zOrderMapItems:  flightMap.zOrderMapItems

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    property var _activeVehicle: multiVehicleManager.activeVehicle

    readonly property real _defaultLatitude:        37.803784
    readonly property real _defaultLongitude:       -122.462276
    readonly property real _defaultRoll:            0
    readonly property real _defaultPitch:           0
    readonly property real _defaultHeading:         0
    readonly property real _defaultAltitudeWGS84:   0
    readonly property real _defaultGroundSpeed:     0
    readonly property real _defaultAirSpeed:        0
    readonly property real _defaultClimbRate:       0

    readonly property string _mapName:              "FlightDisplayView"
    readonly property string _showMapBackgroundKey: "/showMapBackground"

    property real _roll:            _activeVehicle ? (isNaN(_activeVehicle.roll) ? _defaultRoll : _activeVehicle.roll) : _defaultRoll
    property real _pitch:           _activeVehicle ? (isNaN(_activeVehicle.pitch) ? _defaultPitch : _activeVehicle.pitch) : _defaultPitch
    property real _heading:         _activeVehicle ? (isNaN(_activeVehicle.heading) ? _defaultHeading : _activeVehicle.heading) : _defaultHeading

    property real _latitude:        _activeVehicle ? ((_activeVehicle.latitude  === 0) ? _defaultLatitude : _activeVehicle.latitude) : _defaultLatitude
    property real _longitude:       _activeVehicle ? ((_activeVehicle.longitude === 0) ? _defaultLongitude : _activeVehicle.longitude) : _defaultLongitude

    property real _altitudeWGS84:   _activeVehicle ? _activeVehicle.altitudeWGS84 : _defaultAltitudeWGS84
    property real _groundSpeed:     _activeVehicle ? _activeVehicle.groundSpeed : _defaultGroundSpeed
    property real _airSpeed:        _activeVehicle ? _activeVehicle.airSpeed : _defaultAirSpeed
    property real _climbRate:       _activeVehicle ? _activeVehicle.climbRate : _defaultClimbRate

    property bool _showMap: getBool(QGroundControl.flightMapSettings.loadMapSetting(flightMap.mapName, _showMapBackgroundKey, "1"))

    FlightDisplayViewController { id: _controller; }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    // Validate _showMap setting
    Component.onCompleted: _setShowMap(_showMap)

    function getBool(value) {
        return value === '0' ? false : true;
    }

    function setBool(value) {
        return value ? "1" : "0";
    }

    function _setShowMap(showMap) {
        _showMap = _controller.hasVideo ? showMap : true
        QGroundControl.flightMapSettings.saveMapSetting(flightMap.mapName, _showMapBackgroundKey, setBool(_showMap))
    }

    FlightMap {
        id:             flightMap
        anchors.fill:   parent
        mapName:        _mapName
        visible:        _showMap

        property real rootLatitude:     root._latitude
        property real rootLongitude:    root._longitude

        Component.onCompleted: updateMapPosition(true /* force */)

        onRootLatitudeChanged: updateMapPosition(false /* force */)
        onRootLongitudeChanged: updateMapPosition(false /* force */)

        function updateMapPosition(force) {
            if (_followVehicle || force) {
                latitude = root._latitude
                longitude = root._longitude
            }
        }

        property bool _followVehicle: true

        // Home position
        MissionItemIndicator {
            label:          "H"
            coordinate:     (_activeVehicle && _activeVehicle.homePositionAvailable) ? _activeVehicle.homePosition : QtPositioning.coordinate(0, 0)
            visible:        _activeVehicle ? _activeVehicle.homePositionAvailable : false
            z:              flightMap.zOrderMapItems
        }

        // Add trajectory points to the map
        MapItemView {
            model: multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.trajectoryPoints : 0
            
            delegate:
                MapPolyline {
                    line.width: 3
                    line.color: "orange"
                    z:          flightMap.zOrderMapItems - 1


                    path: [
                        { latitude: object.coordinate1.latitude, longitude: object.coordinate1.longitude },
                        { latitude: object.coordinate2.latitude, longitude: object.coordinate2.longitude },
                    ]
                }
        }

        // Add the vehicles to the map
        MapItemView {
            model: multiVehicleManager.vehicles
            
            delegate:
                VehicleMapItem {
                        vehicle:        object
                        coordinate:     object.coordinate
                        isSatellite:    flightMap.isSatelliteMap
                        z:              flightMap.zOrderMapItems
                }
        }

        // Add the mission items to the map
        MapItemView {
            model: multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.missionItems : 0
            
            delegate:
                MissionItemIndicator {
                    label:          object.sequenceNumber
                    isCurrentItem:  object.isCurrentItem
                    coordinate:     object.coordinate
                    z:              flightMap.zOrderMapItems
                }
        }

        // Vehicle GPS lock display
        Column {
            id:     gpsLockColumn
            y:      (parent.height - height) / 2
            width:  parent.width

            Repeater {
                model: multiVehicleManager.vehicles
                
                delegate:
                    QGCLabel {
                        width:                  gpsLockColumn.width
                        horizontalAlignment:    Text.AlignHCenter
                        visible:                object.satelliteLock < 2
                        text:                   "No GPS Lock for Vehicle #" + object.id
                        z:                      flightMap.zOrderMapItems - 2
                    }
            }
        }

        QGCCompassWidget {
            anchors.leftMargin: ScreenTools.defaultFontPixelHeight
            anchors.topMargin:  topMargin
            anchors.left:       parent.left
            anchors.top:        parent.top
            size:               ScreenTools.defaultFontPixelSize * (13.3)
            heading:            _heading
            active:             multiVehicleManager.activeVehicleAvailable
            z:                  flightMap.zOrderWidgets
        }

        QGCAttitudeWidget {
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.left:       parent.left
            anchors.bottom:     parent.bottom
            size:               ScreenTools.defaultFontPixelSize * (13.3)
            rollAngle:          _roll
            pitchAngle:         _pitch
            active:             multiVehicleManager.activeVehicleAvailable
            z:                  flightMap.zOrderWidgets
        }

        DropButton {
            id:                     centerMapDropButton
            anchors.rightMargin:    ScreenTools.defaultFontPixelHeight
            anchors.right:          mapTypeButton.left
            anchors.top:            mapTypeButton.top
            dropDirection:          dropDown
            buttonImage:            "/qmlimages/MapCenter.svg"
            viewportMargins:        ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:         _dropButtonsExclusiveGroup
            z:                      flightMap.zOrderWidgets

            dropDownComponent: Component {
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCCheckBox {
                        id:                 followVehicleCheckBox
                        text:               "Follow Vehicle"
                        checked:            flightMap._followVehicle
                        anchors.baseline:   centerMapButton.baseline

                        onClicked: {
                            centerMapDropButton.hideDropDown()
                            flightMap._followVehicle = !flightMap._followVehicle
                        }
                    }

                    QGCButton {
                        id:         centerMapButton
                        text:       "Center map on Vehicle"
                        enabled:    _activeVehicle && !followVehicleCheckBox.checked

                        property var activeVehicle: multiVehicleManager.activeVehicle

                        onClicked: {
                            centerMapDropButton.hideDropDown()
                            flightMap.latitude = activeVehicle.latitude
                            flightMap.longitude = activeVehicle.longitude
                        }
                    }
                }
            }
        }

        DropButton {
            id:                     mapTypeButton
            anchors.topMargin:      topMargin
            anchors.rightMargin:    ScreenTools.defaultFontPixelHeight
            anchors.top:            parent.top
            anchors.right:          parent.right
            dropDirection:          dropDown
            buttonImage:            "/qmlimages/MapType.svg"
            viewportMargins:        ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:         _dropButtonsExclusiveGroup
            z:                      flightMap.zOrderWidgets

            dropDownComponent: Component {
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    Repeater {
                        model: QGroundControl.flightMapSettings.mapTypes

                        QGCButton {
                            checkable:  true
                            checked:    flightMap.mapType == text
                            text:       modelData

                            onClicked: {
                                flightMap.mapType = text
                                mapTypeButton.hideDropDown()
                            }
                        }
                    }
                }
            }
        }

    } // Flight Map

    QGCVideoBackground {
        anchors.fill:   parent
        display:        _controller.videoSurface
        receiver:       _controller.videoReceiver
        visible:        !_showMap

        QGCCompassHUD {
            id:                 compassHUD
            y:                  root.height * 0.7
            x:                  root.width  * 0.5 - ScreenTools.defaultFontPixelSize * (5)
            width:              ScreenTools.defaultFontPixelSize * (10)
            height:             ScreenTools.defaultFontPixelSize * (10)
            heading:            _heading
            active:             multiVehicleManager.activeVehicleAvailable
            z:                  flightMap.zOrderWidgets
        }

        QGCAttitudeHUD {
            id:                 attitudeHUD
            rollAngle:          _roll
            pitchAngle:         _pitch
            width:              ScreenTools.defaultFontPixelSize * (30)
            height:             ScreenTools.defaultFontPixelSize * (30)
            active:             multiVehicleManager.activeVehicleAvailable
            z:                  flightMap.zOrderWidgets
        }
    }

    QGCAltitudeWidget {
        anchors.right:  parent.right
        height:         parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        width:          ScreenTools.defaultFontPixelSize * (5)
        altitude:       _altitudeWGS84
        z:              flightMap.zOrderWidgets
        visible:        !hideWidgets
    }

    QGCSpeedWidget {
        anchors.left:   parent.left
        width:          ScreenTools.defaultFontPixelSize * (5)
        height:         parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        speed:          _groundSpeed
        z:              flightMap.zOrderWidgets
        visible:        !hideWidgets
    }

    QGCCurrentSpeed {
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        airspeed:           _airSpeed
        groundspeed:        _groundSpeed
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  flightMap.zOrderWidgets
        visible:             !hideWidgets
    }

    QGCCurrentAltitude {
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        altitude:           _altitudeWGS84
        vertZ:              _climbRate
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  flightMap.zOrderWidgets
        visible:              !hideWidgets
    }

    // Mission item list
    ListView {
        id:                 missionItemSummaryList
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        anchors.right:      optionsButton.left
        anchors.bottom:     parent.bottom
        height:             ScreenTools.defaultFontPixelHeight * 7
        spacing:            ScreenTools.defaultFontPixelWidth / 2
        opacity:            0.75
        orientation:        ListView.Horizontal
        model:              multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.missionItems : 0
        z:                  flightMap.zOrderWidgets
        visible:            !hideWidgets

        property real _maxItemHeight: 0

        delegate:
            MissionItemSummary {
                opacity:        0.75
                missionItem:    object
            }
    } // ListView - Mission item list


    QGCButton {
        id:         optionsButton
        x:          flightMap.mapWidgets.x
        y:          flightMap.mapWidgets.y - height - (ScreenTools.defaultFontPixelHeight / 2)
        z:          flightMap.zOrderWidgets
        width:      flightMap.mapWidgets.width
        text:       "Options"
        menu:       optionsMenu
        visible:    _controller.hasVideo && !hideWidgets

        ExclusiveGroup {
            id: backgroundTypeGroup
        }

        Menu {
            id: optionsMenu

            MenuItem {
                id:             mapBackgroundMenuItem
                exclusiveGroup: backgroundTypeGroup
                checkable:      true
                checked:        _showMap
                text:           "Show map as background"

                onTriggered:    _setShowMap(true)
            }

            MenuItem {
                id:             videoBackgroundMenuItem
                exclusiveGroup: backgroundTypeGroup
                checkable:      true
                checked:        !_showMap
                text:           "Show video as background"

                onTriggered:    _setShowMap(false)
            }
        }
    }
}
