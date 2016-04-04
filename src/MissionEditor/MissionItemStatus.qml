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

import QtQuick          2.5
import QtQuick.Controls 1.3

import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl               1.0

Rectangle {
    property var    currentMissionItem          ///< Mission item to display status for
    property var    missionItems                ///< List of all available mission items
    property real   expandedWidth               ///< Width of control when expanded

    width:      _expanded ? expandedWidth : _collapsedWidth
    height:     valueGrid.height + _margins
    radius:     ScreenTools.defaultFontPixelWidth
    color:      qgcPal.window
    opacity:    0.80
    clip:       true

    readonly property real margins: ScreenTools.defaultFontPixelWidth

    property real   _collapsedWidth:    valueGrid.width + (margins * 2)
    property bool   _expanded:          true
    property real   _distance:          _statusValid ? _currentMissionItem.distance : 0
    property real   _altDifference:     _statusValid ? _currentMissionItem.altDifference : 0
    property real   _gradient:          _statusValid ? Math.atan(_currentMissionItem.altDifference / _currentMissionItem.distance) : 0
    property real   _gradientPercent:   isNaN(_gradient) ? 0 : _gradient * 100
    property real   _azimuth:           _statusValid ? _currentMissionItem.azimuth : -1
    property bool   _statusValid:       currentMissionItem != undefined
    property string _distanceText:      _statusValid ? QGroundControl.metersToAppSettingsDistanceUnits(_distance).toFixed(2) + " " + QGroundControl.appSettingsDistanceUnitsString : ""
    property string _altText:           _statusValid ? QGroundControl.metersToAppSettingsDistanceUnits(_altDifference).toFixed(2) + " " + QGroundControl.appSettingsDistanceUnitsString : ""
    property string _gradientText:      _statusValid ? _gradientPercent.toFixed(0) + "%" : ""
    property string _azimuthText:       _statusValid ? Math.round(_azimuth) : ""

    readonly property real _margins:    ScreenTools.defaultFontPixelWidth

    MouseArea {
        anchors.fill:   parent
        onClicked:      _expanded = !_expanded
    }

    Grid {
        id:                 valueGrid
        anchors.margins:    _margins
        anchors.left:       parent.left
        anchors.top:        parent.top
        columns:            2
        columnSpacing:      _margins

        QGCLabel { text: qsTr("Distance:") }
        QGCLabel { text: _distanceText }

        QGCLabel { text: qsTr("Alt diff:") }
        QGCLabel { text: _altText }

        QGCLabel { text: qsTr("Gradient:") }
        QGCLabel { text: _gradientText }

        QGCLabel { text: qsTr("Azimuth:") }
        QGCLabel { text: _azimuthText }
    }

    QGCFlickable {
        anchors.leftMargin:     _margins
        anchors.rightMargin:    _margins
        anchors.left:           valueGrid.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        contentWidth:           graphRow.width
        clip:                   true

        Row {
            id:             graphRow
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        ScreenTools.smallFontPixelWidth

            Repeater {
                model: missionItems

                Item {
                    height:     graphRow.height
                    width:      ScreenTools.smallFontPixelWidth * 2
                    visible:    object.specifiesCoordinate && !object.isStandaloneCoordinate


                    property real availableHeight: height - ScreenTools.smallFontPixelHeight - indicator.height

                    property bool graphAbsolute:    true

                    MissionItemIndexLabel {
                        id:                         indicator
                        anchors.horizontalCenter:   parent.horizontalCenter
                        y:                          availableHeight - (availableHeight * object.altPercent)
                        small:                      true
                        isCurrentItem:              object.isCurrentItem
                        label:                      object.abbreviation
                        visible:                    object.relativeAltitude ? true : (object.homePosition || graphAbsolute)
                    }

                    /*
                      Taking these off for now since there really isn't room for the numbers
                    QGCLabel {
                        anchors.bottom:             parent.bottom
                        anchors.horizontalCenter:   parent.horizontalCenter
                        font.pixelSize:             ScreenTools.smallFontPixelSize
                        text:                       (object.relativeAltitude ? "" : "=") + object.coordinate.altitude.toFixed(0)
                    }
                    */
                }
            }
        }
    }
}
