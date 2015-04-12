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
 *   @brief QGC Altitude Indicator
 *   @author Gus Grubba <mavlink@grubba.com>
 */

// TODO: This is temporary until I find a better way to display a large range of numbers

import QtQuick 2.4

Rectangle {
    id: root
    property real altitude:         50
    property real _reticleSpacing:  16
    property real _reticleHeight:   2
    property real _reticleSlot:     _reticleSpacing + _reticleHeight
    property var  _speedArray:      []
    property int  _currentCenter:   0
    property int  _currentStart:    -100

    function updateArray() {
        var tmpArray = [];
        _currentCenter = Math.floor(altitude / 5) * 5;
        _currentStart = _currentCenter + 100;
        for(var i = 0; i < 40; i++) {
            tmpArray[i] = _currentStart - (i * 5);
        }
        _speedArray = tmpArray;
    }

    Component.onCompleted:
    {
        updateArray() ;
    }

    onAltitudeChanged: {
        if(Math.abs(_currentCenter - altitude) > 50) {
            updateArray() ;
        }
    }

    anchors.verticalCenter: parent.verticalCenter

    height: parent.height * 0.75 > 280 ? 280 : parent.height * 0.75
    clip:   true
    smooth: true
    radius: 5
    border.color: Qt.rgba(1,1,1,0.25)
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(0,0,0,0.65) }
        GradientStop { position: 0.5; color: Qt.rgba(0,0,0,0.25) }
        GradientStop { position: 1.0; color: Qt.rgba(0,0,0,0.65) }
    }
    Column{
        id: col
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        spacing: _reticleSpacing
        Repeater {
            model: _speedArray
            anchors.left: parent.left
            Rectangle {
                property int _alt: modelData
                width:  (_alt % 10 === 0) ? 10 : 15
                height: _reticleHeight
                color:  Qt.rgba(1,1,1,0.35)
                Text {
                    visible: (_alt % 10 === 0)
                    x: 20
                    anchors.verticalCenter:   parent.verticalCenter
                    antialiasing: true
                    font.weight: Font.DemiBold
                    text:  _alt
                    color: _alt < 0 ? "#f8983a" : "white"
                    style: Text.Outline
                    styleColor: Qt.rgba(0,0,0,0.25)
                }
            }
        }
        transform: Translate {
            y: ((altitude - _currentCenter) * _reticleSlot / 5) - (_reticleSlot / 2)
        }
    }
}
