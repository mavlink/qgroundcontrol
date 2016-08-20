/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.3

import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Rectangle {
    readonly property var       _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle

    property Fact   _offlineEditingVehicleType:     QGroundControl.offlineEditingVehicleType
    property Fact   _offlineEditingCruiseSpeed:     QGroundControl.offlineEditingCruiseSpeed
    property Fact   _offlineEditingHoverSpeed:      QGroundControl.offlineEditingHoverSpeed

    property var    currentMissionItem          ///< Mission item to display status for
    property var    missionItems                ///< List of all available mission items
    property real   expandedWidth               ///< Width of control when expanded
    property real   missionDistance
    property real   missionMaxTelemetry
    property real   cruiseDistance
    property real   hoverDistance

    width:      _expanded ? expandedWidth : _collapsedWidth
    height:     Math.max(valueGrid.height, valueMissionGrid.height) + (_margins * 2)
    radius:     ScreenTools.defaultFontPixelWidth * 0.5
    color:      qgcPal.window
    opacity:    0.80
    clip:       true

    readonly property real margins: ScreenTools.defaultFontPixelWidth

    property real   _collapsedWidth:    valueGrid.width + (margins * 2)
    property bool   _expanded:          true

    property real   _distance:          _statusValid ? _currentMissionItem.distance : 0
    property real   _altDifference:     _statusValid ? _currentMissionItem.altDifference : 0
    property real   _gradient:          _statusValid && _currentMissionItem.distance > 0 ? Math.atan(_currentMissionItem.altDifference / _currentMissionItem.distance) : 0
    property real   _gradientPercent:   isNaN(_gradient) ? 0 : _gradient * 100
    property real   _azimuth:           _statusValid ? _currentMissionItem.azimuth : -1
    property real   _missionDistance:   _missionValid ? missionDistance : 0
    property real   _missionMaxTelemetry: _missionValid ? missionMaxTelemetry : 0
    property real   _missionTime:       _missionValid && _missionSpeed > 0 ?  (_isVTOL ? _hoverTime + _cruiseTime : _missionDistance / _missionSpeed) : 0
    property real   _hoverDistance:    _missionValid ? hoverDistance : 0
    property real   _cruiseDistance:    _missionValid ? cruiseDistance : 0
    property real   _hoverTime:         _missionValid && _offlineEditingHoverSpeed.value > 0 ? _hoverDistance / _offlineEditingHoverSpeed.value : 0
    property real   _cruiseTime:        _missionValid && _offlineEditingCruiseSpeed.value > 0 ? _cruiseDistance / _offlineEditingCruiseSpeed.value : 0

    property bool   _statusValid:       currentMissionItem != undefined
    property bool   _vehicleValid:      _activeVehicle != undefined
    property bool   _missionValid:      missionItems != undefined
    property bool   _currentSurvey:     _statusValid ? _currentMissionItem.commandName == "Survey" : false
    property bool   _isVTOL:            _vehicleValid ? _activeVehicle.vtol : _offlineEditingVehicleType.enumStringValue == "VTOL" //hardcoded
    property real   _missionSpeed:      _offlineEditingVehicleType.enumStringValue == "Fixedwing" ? _offlineEditingCruiseSpeed.value : _offlineEditingHoverSpeed.value

    property string _distanceText:      _statusValid ? QGroundControl.metersToAppSettingsDistanceUnits(_distance).toFixed(2) + " " + QGroundControl.appSettingsDistanceUnitsString : " "
    property string _altText:           _statusValid ? QGroundControl.metersToAppSettingsDistanceUnits(_altDifference).toFixed(2) + " " + QGroundControl.appSettingsDistanceUnitsString : " "
    property string _gradientText:      _statusValid ? _gradientPercent.toFixed(0) + "%" : " "
    property string _azimuthText:       _statusValid ? Math.round(_azimuth) : " "
    property string _missionDistanceText: _missionValid ? QGroundControl.metersToAppSettingsDistanceUnits(_missionDistance).toFixed(2) + " " + QGroundControl.appSettingsDistanceUnitsString : " "
    property string _missionTimeText:     _missionValid ? _missionTime.toFixed(0) + "s" : " "
    property string _missionMaxTelemetryText:  _missionValid ? QGroundControl.metersToAppSettingsDistanceUnits(_missionMaxTelemetry).toFixed(2) + " " + QGroundControl.appSettingsDistanceUnitsString : " "
    property string _hoverDistanceText: _missionValid ? QGroundControl.metersToAppSettingsDistanceUnits(_hoverDistance).toFixed(2) + " " + QGroundControl.appSettingsDistanceUnitsString : " "
    property string _cruiseDistanceText: _missionValid ? QGroundControl.metersToAppSettingsDistanceUnits(_cruiseDistance).toFixed(2) + " " + QGroundControl.appSettingsDistanceUnitsString : " "
    property string _hoverTimeText:     _missionValid ? _hoverTime.toFixed(0) + "s" : " "
    property string _cruiseTimeText:    _missionValid ? _cruiseTime.toFixed(0) + "s" : " "

    readonly property real _margins:    ScreenTools.defaultFontPixelWidth

    MouseArea {
        anchors.fill:   parent
        onClicked:      _expanded = !_expanded
    }

    Row {
        anchors.fill:       parent
        anchors.margins:    _margins
        spacing:            _margins

        Grid {
            id:                 valueGrid
            columns:            2
            columnSpacing:      _margins
            anchors.verticalCenter: parent.verticalCenter

            QGCLabel { text: qsTr("Selected waypoint") }
            QGCLabel { text: qsTr(" ") }

            QGCLabel { text: qsTr("Distance:") }
            QGCLabel { text: _distanceText }

            QGCLabel { text: qsTr("Alt diff:") }
            QGCLabel { text: _altText }

            QGCLabel { text: qsTr("Gradient:") }
            QGCLabel { text: _gradientText }

            QGCLabel { text: qsTr("Azimuth:") }
            QGCLabel { text: _azimuthText }
        }

        ListView {
            id:                     statusListView
            model:                  missionItems
            highlightMoveDuration:  250
            anchors.leftMargin:     _margins
            anchors.rightMargin:    _margins
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            orientation:            ListView.Horizontal
            spacing:                0
            visible:                _expanded
            width:                  parent.width - valueGrid.width - valueMissionGrid.width - (_margins * 2)
            clip:                   true
            currentIndex:           _currentMissionIndex

            delegate: Item {
                height:     statusListView.height
                width:      display ? (indicator.width + spacing)  : 0
                visible:    display

                property real availableHeight:  height - indicator.height
                property bool graphAbsolute:    true
                readonly property bool display: object.specifiesCoordinate && !object.isStandaloneCoordinate
                readonly property real spacing: ScreenTools.defaultFontPixelWidth * ScreenTools.smallFontPointRatio

                MissionItemIndexLabel {
                    id:                         indicator
                    anchors.horizontalCenter:   parent.horizontalCenter
                    y:                          availableHeight - (availableHeight * object.altPercent)
                    small:                      true
                    isCurrentItem:              object.isCurrentItem
                    label:                      object.abbreviation
                    visible:                    object.relativeAltitude ? true : (object.homePosition || graphAbsolute)
                }
            }
        }

        Grid {
            id:                 valueMissionGrid
            columns:            2
            columnSpacing:      _margins
            anchors.verticalCenter: parent.verticalCenter

            QGCLabel { text: qsTr("Total mission") }
            QGCLabel { text: qsTr(" ") }

            QGCLabel { text: qsTr("Distance:") }
            QGCLabel { text: _missionDistanceText }

            QGCLabel { text: qsTr("Time:") }
            QGCLabel { text: _missionTimeText }

            QGCLabel { text: qsTr("Max telem dist:") }
            QGCLabel { text: _missionMaxTelemetryText }

            QGCLabel {
                text: qsTr("Hover distance:")
                visible: _isVTOL
            }
            QGCLabel {
                text: _hoverDistanceText
                visible: _isVTOL
            }

            QGCLabel {
                text: qsTr("Cruise distance:")
                visible: _isVTOL
            }
            QGCLabel {
                text: _cruiseDistanceText
                visible: _isVTOL
            }

            QGCLabel {
                text: qsTr("Hover time:")
                visible: _isVTOL
            }
            QGCLabel {
                text: _hoverTimeText
                visible: _isVTOL
            }

            QGCLabel {
                text: qsTr("Cruise time:")
                visible: _isVTOL
            }
            QGCLabel {
                text: _cruiseTimeText
                visible: _isVTOL
            }
        }
    }
}
