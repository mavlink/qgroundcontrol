/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.4
import QtQuick.Controls 1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

/// Map scale control
Item {
    id:     scale
    width:  rightEnd.x + rightEnd.width
    height: rightEnd.y + rightEnd.height

    property var    mapControl                                                          ///< Map control for which this scale control is being used

    property variant _scaleLengthsMeters: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
    property variant _scaleLengthsFeet: [10, 25, 50, 100, 250, 500, 1000, 2000, 3000, 4000, 5280, 5280*2, 5280*5, 5280*10, 5280*25, 5280*50, 5280*100, 5280*250, 5280*500, 5280*1000]

    property var _color: mapControl.isSatelliteMap ? "white" : "black"

    function formatDistanceMeters(meters) {
        var dist = Math.round(meters)
        if (dist > 1000 ){
            if (dist > 100000){
                dist = Math.round(dist / 1000)
            } else {
                dist = Math.round(dist / 100)
                dist = dist / 10
            }
            dist = dist + qsTr(" km")
        } else {
            dist = dist + qsTr(" m")
        }
        return dist
    }

    function formatDistanceFeet(feet) {
        var dist = Math.round(feet)
        if (dist >= 5280) {
            dist = Math.round(dist / 5280)
            dist = dist
            if (dist == 1) {
                dist += qsTr(" mile")
            } else {
                dist += qsTr(" miles")
            }
        } else {
            dist = dist + qsTr(" ft")
        }
        return dist
    }

    function calculateMetersRatio(scaleLineMeters, scaleLinePixelLength) {
        var scaleLineRatio = 0

        if (scaleLineMeters === 0) {
            // not visible
        } else {
            for (var i = 0; i < _scaleLengthsMeters.length - 1; i++) {
                if (scaleLineMeters < (_scaleLengthsMeters[i] + _scaleLengthsMeters[i+1]) / 2 ) {
                    scaleLineRatio = _scaleLengthsMeters[i] / scaleLineMeters
                    scaleLineMeters = _scaleLengthsMeters[i]
                    break;
                }
            }
            if (scaleLineRatio === 0) {
                scaleLineRatio = scaleLineMeters / _scaleLengthsMeters[i]
                scaleLineMeters = _scaleLengthsMeters[i]
            }
        }

        var text = formatDistanceMeters(scaleLineMeters)
        centerLine.width = (scaleLinePixelLength * scaleLineRatio) - (2 * leftEnd.width)
        scaleText.text = text
    }

    function calculateFeetRatio(scaleLineMeters, scaleLinePixelLength) {
        var scaleLineRatio = 0
        var scaleLineFeet = scaleLineMeters * 3.2808399

        if (scaleLineFeet === 0) {
            // not visible
        } else {
            for (var i = 0; i < _scaleLengthsFeet.length - 1; i++) {
                if (scaleLineFeet < (_scaleLengthsFeet[i] + _scaleLengthsFeet[i+1]) / 2 ) {
                    scaleLineRatio = _scaleLengthsFeet[i] / scaleLineFeet
                    scaleLineFeet = _scaleLengthsFeet[i]
                    break;
                }
            }
            if (scaleLineRatio === 0) {
                scaleLineRatio = scaleLineFeet / _scaleLengthsFeet[i]
                scaleLineFeet = _scaleLengthsFeet[i]
            }
        }

        var text = formatDistanceFeet(scaleLineFeet)
        centerLine.width = (scaleLinePixelLength * scaleLineRatio) - (2 * leftEnd.width)
        scaleText.text = text
    }

    function calculateScale() {
        var scaleLinePixelLength = 100
        var leftCoord = mapControl.toCoordinate(Qt.point(0, scale.y))
        var rightCoord = mapControl.toCoordinate(Qt.point(scaleLinePixelLength, scale.y))
        var scaleLineMeters = Math.round(leftCoord.distanceTo(rightCoord))

        if (QGroundControl.distanceUnits.value == QGroundControl.DistanceUnitsFeet) {
            calculateFeetRatio(scaleLineMeters, scaleLinePixelLength)
        } else {
            calculateMetersRatio(scaleLineMeters, scaleLinePixelLength)
        }
    }

    Connections {
        target: mapControl

        onWidthChanged: scaleTimer.restart()
        onHeightChanged: scaleTimer.restart()
        onZoomLevelChanged: scaleTimer.restart()
    }

    Timer {
        id:         scaleTimer
        interval:   100
        running:    false
        repeat:     false

        onTriggered: calculateScale()
    }

    QGCMapLabel {
        id:                     scaleText
        map:                    mapControl
        font.family:            ScreenTools.demiboldFontFamily
        anchors.left:           parent.left
        anchors.right:          parent.right
        horizontalAlignment:    Text.AlignRight
        text:                   "0 m"
    }

    Rectangle {
        id:             leftEnd
        anchors.top:    scaleText.bottom
        anchors.left:   parent.left
        width:          2
        height:         ScreenTools.defaultFontPixelHeight
        color:          _color
    }

    Rectangle {
        id:                     centerLine
        anchors.bottomMargin:   2
        anchors.bottom:         leftEnd.bottom
        anchors.left:           leftEnd.right
        height:                 2
        color:                  _color
    }

    Rectangle {
        id:             rightEnd
        anchors.top:    leftEnd.top
        anchors.left:   centerLine.right
        width:          2
        height:         ScreenTools.defaultFontPixelHeight
        color:          _color
    }

    Component.onCompleted: {
        if (scale.visible) {
            calculateScale();
        }
    }
}
