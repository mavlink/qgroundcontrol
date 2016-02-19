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
 *   @brief QGC Fly View Widgets
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0

Rectangle {
    id:     instrumentPanel
    height: compass.y + compass.height + _topBottomMargin
    width:  size
    radius: size / 2
    color:  isSatellite ? Qt.rgba(1,1,1,0.75) : Qt.rgba(0,0,0,0.75)

    property alias  heading:        compass.heading
    property alias  rollAngle:      attitude.rollAngle
    property alias  pitchAngle:     attitude.pitchAngle
    property real   size:           _defaultSize
    property bool   isSatellite:    false
    property bool   active:         false
    property var    qgcView
    property real   maxHeight

    property Fact   _emptyFact:         Fact { }
    property Fact   groundSpeedFact:    _emptyFact
    property Fact   airSpeedFact:       _emptyFact

    property real   _defaultSize:   ScreenTools.defaultFontPixelSize * (9)

    property real   _sizeRatio:     ScreenTools.isTinyScreen ? (size / _defaultSize) * 0.5 : size / _defaultSize
    property real   _bigFontSize:   ScreenTools.defaultFontPixelSize * 2.5  * _sizeRatio
    property real   _normalFontSize:ScreenTools.defaultFontPixelSize * 1.5  * _sizeRatio
    property real   _labelFontSize: ScreenTools.defaultFontPixelSize * 0.75 * _sizeRatio
    property real   _spacing:       ScreenTools.defaultFontPixelSize * 0.33
    property real   _topBottomMargin: (size * 0.05) / 2
    property real   _availableValueHeight: maxHeight - (attitude.height + _spacer1.height + _spacer2.height + compass.height + (_spacing * 4))

    MouseArea {
        anchors.fill: parent
        onClicked: _valuesWidget.showPicker()
    }

    QGCAttitudeWidget {
        id:             attitude
        y:              _topBottomMargin
        size:           parent.width * 0.95
        active:         instrumentPanel.active
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Rectangle {
        id:                 _spacer1
        anchors.topMargin:  _spacing
        anchors.top:        attitude.bottom
        height:             1
        width:              parent.width * 0.9
        color:              isSatellite ? Qt.rgba(0,0,0,0.25) : Qt.rgba(1,1,1,0.25)
        anchors.horizontalCenter: parent.horizontalCenter
    }

    ValuesWidget {
        id:                 _valuesWidget
        anchors.topMargin:  _spacing
        anchors.top:        _spacer1.bottom
        width:              parent.width
        qgcView:            instrumentPanel.qgcView
        textColor:          isSatellite ? "black" : "white"
        maxHeight:          _availableValueHeight
    }

    Rectangle {
        id:                 _spacer2
        anchors.topMargin:  _spacing
        anchors.top:        _valuesWidget.bottom
        height:             1
        width:              parent.width * 0.9
        color:              isSatellite ? Qt.rgba(0,0,0,0.25) : Qt.rgba(1,1,1,0.25)
        anchors.horizontalCenter: parent.horizontalCenter
    }

    QGCCompassWidget {
        id:                 compass
        anchors.topMargin:  _spacing
        anchors.top:        _spacer2.bottom
        size:               parent.width * 0.95
        active:             instrumentPanel.active
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
