/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Rectangle {
    width:      expandedWidth
    height:     ScreenTools.defaultFontPixelHeight * 7
    radius:     ScreenTools.defaultFontPixelWidth * 0.5
    color:      qgcPal.window
    opacity:    0.80
    clip:       true

    property var    currentMissionItem          ///< Mission item to display status for
    property var    missionItems                ///< List of all available mission items
    property real   expandedWidth               ///< Width of control when expanded
    property real   missionDistance             ///< Total mission distance
    property real   missionTime                 ///< Total mission time
    property real   missionMaxTelemetry

    property bool   _statusValid:               currentMissionItem != undefined
    property bool   _missionValid:              missionItems != undefined

    property real   _distance:                  _statusValid ? _currentMissionItem.distance : NaN
    property real   _altDifference:             _statusValid ? _currentMissionItem.altDifference : NaN
    property real   _gradient:                  _statusValid && _currentMissionItem.distance > 0 ? Math.atan(_currentMissionItem.altDifference / _currentMissionItem.distance) : NaN
    property real   _gradientPercent:           isNaN(_gradient) ? NaN : _gradient * 100
    property real   _azimuth:                   _statusValid ? _currentMissionItem.azimuth : NaN
    property real   _missionDistance:           _missionValid ? missionDistance : NaN
    property real   _missionMaxTelemetry:       _missionValid ? missionMaxTelemetry : NaN
    property real   _missionTime:               _missionValid ? missionTime : NaN

    property string _distanceText:              isNaN(_distance) ? "-.-" : QGroundControl.metersToAppSettingsDistanceUnits(_distance).toFixed(1) + " " + QGroundControl.appSettingsDistanceUnitsString
    property string _altDifferenceText:         isNaN(_altDifference) ? "-.-" : QGroundControl.metersToAppSettingsDistanceUnits(_altDifference).toFixed(1) + " " + QGroundControl.appSettingsDistanceUnitsString
    property string _gradientText:              isNaN(_gradient) ? "-.-" : _gradientPercent.toFixed(0) + "%"
    property string _azimuthText:               isNaN(_azimuth) ? "-.-" : Math.round(_azimuth)
    property string _missionDistanceText:       isNaN(_missionDistance) ? "-.-" : QGroundControl.metersToAppSettingsDistanceUnits(_missionDistance).toFixed(1) + " " + QGroundControl.appSettingsDistanceUnitsString
    property string _missionTimeText:           isNaN(_missionTime) ? "-.-" : Number(_missionTime / 60).toFixed(1) + " min"
    property string _missionMaxTelemetryText:   isNaN(_missionMaxTelemetry) ? "-.-" : QGroundControl.metersToAppSettingsDistanceUnits(_missionMaxTelemetry).toFixed(1) + " " + QGroundControl.appSettingsDistanceUnitsString

    readonly property real _margins:    ScreenTools.defaultFontPixelWidth

    QGCListView {
        id:                     statusListView
        anchors.fill:           parent
        anchors.margins:        _margins
        model:                  missionItems
        highlightMoveDuration:  250
        orientation:            ListView.Horizontal
        spacing:                0
        width:                  parent.width -  (_margins * 2)
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
                checked:                    object.isCurrentItem
                label:                      object.abbreviation
                visible:                    object.relativeAltitude ? true : (object.homePosition || graphAbsolute)
            }
        }
    }
}

