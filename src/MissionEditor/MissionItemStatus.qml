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

Rectangle {
    property var    currentMissionItem          ///< Mission item to display status for
    property var    missionItems                ///< List of all available mission items
    property real   expandedWidth               ///< Width of control when expanded

    width:      _expanded ? expandedWidth : _collapsedWidth
    height:     valueGrid.height + (_margins * 2)
    radius:     ScreenTools.defaultFontPixelWidth * 0.5
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

    Row {
        anchors.fill:       parent
        anchors.margins:    _margins
        spacing:            _margins

        Grid {
            id:                 valueGrid
            columns:            2
            columnSpacing:      _margins
            anchors.verticalCenter: parent.verticalCenter

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
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            width:                  parent.width - valueGrid.width - (_margins * 2)
            contentWidth:           graphRow.width
            visible:                _expanded
            clip:                   true

            Row {
                id:                 graphRow
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                //anchors.margins:    ScreenTools.defaultFontPixelWidth * ScreenTools.smallFontPointRatio
                spacing:            ScreenTools.defaultFontPixelWidth * ScreenTools.smallFontPointRatio

                Repeater {
                    model: missionItems

                    Item {
                        height:     graphRow.height
                        width:      indicator.width
                        visible:    object.specifiesCoordinate && !object.isStandaloneCoordinate

                        property real availableHeight:  height - indicator.height
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
                            font.pointSize:             ScreenTools.smallFontPointSize
                            text:                       (object.relativeAltitude ? "" : "=") + object.coordinate.altitude.toFixed(0)
                        }
                        */
                    }
                }
            }
        }
    }
}
