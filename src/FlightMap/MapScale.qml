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

    property variant _scaleLengths: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]

    property bool   _isSatelliteMap:    mapControl.activeMapType.name.indexOf("Satellite") > -1 || mapControl.activeMapType.name.indexOf("Hybrid") > -1
    property var    _color:             _isSatelliteMap ? "white" : "black"

    function formatDistance(meters)
    {
        var dist = Math.round(meters)
        if (dist > 1000 ){
            if (dist > 100000){
                dist = Math.round(dist / 1000)
            }
            else{
                dist = Math.round(dist / 100)
                dist = dist / 10
            }
            dist = dist + " km"
        }
        else{
            dist = dist + " m"
        }
        return dist
    }

    function calculateScale() {
        var scaleLineWidth = 100
        var coord1, coord2, dist, text, f
        f = 0
        coord1 = mapControl.toCoordinate(Qt.point(0, scale.y))
        coord2 = mapControl.toCoordinate(Qt.point(scaleLineWidth, scale.y))
        dist = Math.round(coord1.distanceTo(coord2))
        if (dist === 0) {
            // not visible
        } else {
            for (var i = 0; i < _scaleLengths.length - 1; i++) {
                if (dist < (_scaleLengths[i] + _scaleLengths[i+1]) / 2 ) {
                    f = _scaleLengths[i] / dist
                    dist = _scaleLengths[i]
                    break;
                }
            }
            if (f === 0) {
                f = dist / _scaleLengths[i]
                dist = _scaleLengths[i]
            }
        }
        text = formatDistance(dist)
        centerLine.width = (scaleLineWidth * f) - (2 * leftEnd.width)
        scaleText.text = text
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

    QGCLabel {
        id:                     scaleText
        color:                  _color
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
