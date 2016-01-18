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

Rectangle {
    property var    currentMissionItem          ///< Mission item to display status for
    property var    missionItems                ///< List of all available mission items
    property real   expandedWidth               ///< Width of control when expanded
    property bool   homePositionValid:  false   /// true: home position in missionItems[0] is valid

    width:      _expanded ? expandedWidth : _collapsedWidth
    height:     expandLabel.y + expandLabel.height + _margins
    radius:     ScreenTools.defaultFontPixelWidth
    color:      qgcPal.window
    opacity:    0.80
    clip:       true

    readonly property real margins: ScreenTools.defaultFontPixelWidth

    property real   _collapsedWidth:    distanceLabel.width + (margins * 2)
    property bool   _expanded:          true
    property real   _distance:          _currentMissionItem ? _currentMissionItem.distance : -1
    property real   _altDifference:     _currentMissionItem ? _currentMissionItem.altDifference : -1
    property real   _azimuth:           _currentMissionItem ? _currentMissionItem.azimuth : -1
    property real   _isHomePosition:    _currentMissionItem ? _currentMissionItem.homePosition : false
    property bool   _statusValid:       _distance != -1 && ((_isHomePosition && homePositionValid) || !_isHomePosition)
    property string _distanceText:      _statusValid ? Math.round(_distance) + " meters" : ""
    property string _altText:           _statusValid ? Math.round(_altDifference) + " meters" : ""
    property string _azimuthText:       _statusValid ? Math.round(_azimuth) : ""

    readonly property real _margins:    ScreenTools.defaultFontPixelWidth

    MouseArea {
        anchors.fill:   parent
        onClicked:      _expanded = !_expanded
    }

    QGCLabel {
        id:                 distanceLabel
        anchors.margins:    _margins
        anchors.left:       parent.left
        anchors.top:        parent.top
        text:               "Distance: " + _distanceText
    }

    QGCLabel {
        id:                 altLabel
        anchors.left:       distanceLabel.left
        anchors.top:        distanceLabel.bottom
        text:               "Alt diff: " + _altText
    }

    QGCLabel {
        id:                 azimuthLabel
        anchors.left:       altLabel.left
        anchors.top:        altLabel.bottom
        text:               "Azimuth: " + _azimuthText
    }

    QGCLabel {
        id:                 expandLabel
        anchors.left:       azimuthLabel.left
        anchors.top:        azimuthLabel.bottom
        text:               _expanded ? "<<" : ">>"
    }

    QGCFlickable {
        anchors.leftMargin:     _margins
        anchors.rightMargin:    _margins
        anchors.left:           distanceLabel.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom

        Row {
            id:             graphRow
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        ScreenTools.smallFontPixelWidth

            Repeater {
                model: missionItems

                Item {
                    height: graphRow.height
                    width:  ScreenTools.smallFontPixelWidth * 2

                    property real availableHeight: height - ScreenTools.smallFontPixelHeight - indicator.height

                    // If home position is not valid we are graphing relative based on a home alt of 0. Because of this
                    // we cannot graph absolute altitudes since we have no basis for comparison against the relative values.
                    property bool graphAbsolute:    homePositionValid

                    MissionItemIndexLabel {
                        id:                         indicator
                        anchors.horizontalCenter:   parent.horizontalCenter
                        y:                          availableHeight - (availableHeight * object.altPercent)
                        small:                      true
                        isCurrentItem:              object.isCurrentItem
                        label:                      object.homePosition ? "H" : object.sequenceNumber
                        visible:                    object.relativeAltitude ? true : (object.homePosition || graphAbsolute)
                    }

                    QGCLabel {
                        anchors.bottom:             parent.bottom
                        anchors.horizontalCenter:   parent.horizontalCenter
                        font.pixelSize:             ScreenTools.smallFontPixelSize
                        text:                       (object.relativeAltitude ? "" : "=") + object.coordinate.altitude
                    }
                }

            }
        }
    }
}
