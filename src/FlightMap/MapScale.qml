/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.SettingsManager

/// Map scale control
RowLayout {
    id: scale

    property var    mapControl                      ///< Map control for which this scale control is being used
    property bool   extraButtonsVisible:    false   ///< Show terrain, stats buttons
    property alias  terrainButtonChecked:   terrainButton.checked
    property alias  statsButtonChecked:     statsButton.checked
    property bool   zoomButtonsVisible:     true
    property bool   buttonsOnLeft:          true    ///< Buttons to left/right of scale bar

    signal terrainButtonClicked
    signal statsButtonClicked

    property var    _scaleLengthsMeters:    [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
    property var    _scaleLengthsFeet:      [10, 25, 50, 100, 250, 500, 1000, 2000, 3000, 4000, 5280, 5280*2, 5280*5, 5280*10, 5280*25, 5280*50, 5280*100, 5280*250, 5280*500, 5280*1000]
    property bool   _zoomButtonsVisible:    zoomButtonsVisible && !ScreenTools.isMobile
    property var    _color:                 mapControl.isSatelliteMap ? "white" : "black"
    property real   _buttonOpacity:         0.75
    property real   _scaleEndWidth:         2
    property real   _scaleEndHeight:        ScreenTools.defaultFontPixelHeight

    Component.onCompleted: {
        if (scale.visible) {
            calculateScale();
        }
    }

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
        centerLine.Layout.preferredWidth = (scaleLinePixelLength * scaleLineRatio) - (2 * leftEnd.width)
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
        centerLine.Layout.preferredWidth = (scaleLinePixelLength * scaleLineRatio) - (2 * _scaleEndWidth)
        scaleText.text = text
    }

    function calculateScale() {
        if(mapControl) {
            var scaleLinePixelLength = 100
            var leftCoord  = mapControl.toCoordinate(Qt.point(0, scale.y), false /* clipToViewPort */)
            var rightCoord = mapControl.toCoordinate(Qt.point(scaleLinePixelLength, scale.y), false /* clipToViewPort */)
            var scaleLineMeters = Math.round(leftCoord.distanceTo(rightCoord))
            if (QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits.value === UnitsSettings.HorizontalDistanceUnitsFeet) {
                calculateFeetRatio(scaleLineMeters, scaleLinePixelLength)
            } else {
                calculateMetersRatio(scaleLineMeters, scaleLinePixelLength)
            }
        }
    }

    Connections {
        target: mapControl

        function onWidthChanged     () { scaleTimer.restart() }
        function onHeightChanged    () { scaleTimer.restart() }
        function onZoomLevelChanged () { scaleTimer.restart() }
    }

    Timer {
        id:                 scaleTimer
        interval:           100
        running:            false
        repeat:             false
        onTriggered:        calculateScale()
    }

    QGCButton {
        id:                 terrainButton
        width:              height
        Layout.fillHeight:  true
        text:               qsTr("T")
        opacity:            _buttonOpacity
        visible:            extraButtonsVisible
        onClicked:          terrainButtonClicked()
    }

    QGCButton {
        id:                 statsButton
        width:              height
        Layout.fillHeight:  true
        text:               qsTr("S")
        opacity:            _buttonOpacity
        visible:            extraButtonsVisible
        onClicked:          statsButtonClicked()
    }

    QGCButton {
        id:                 zoomUpButton
        width:              height
        Layout.fillHeight:  true
        text:               qsTr("+")
        opacity:            _buttonOpacity
        visible:            _zoomButtonsVisible
        onClicked:          mapControl.zoomLevel += 0.5
    }

    QGCButton {
        id:                 zoomDownButton
        width:              height
        Layout.fillHeight:  true
        text:               qsTr("-")
        opacity:            _buttonOpacity
        visible:            _zoomButtonsVisible
        onClicked:          mapControl.zoomLevel -= 0.5
    }

    ColumnLayout {
        spacing: 0

        QGCMapLabel {
            id:                     scaleText
            Layout.fillWidth:       true
            map:                    mapControl
            font.family:            ScreenTools.demiboldFontFamily
            text:                   "0 m"
            horizontalAlignment:    Text.AlignRight
        }

        RowLayout {
            spacing: 0

            Rectangle {
                width:  _scaleEndWidth
                height: _scaleEndHeight
                color:  _color
            }

            Rectangle {
                id:                 centerLine
                height:             _scaleEndWidth
                color:              _color
            }

            Rectangle {
                width:  _scaleEndWidth
                height: _scaleEndHeight
                color:  _color
            }
        }
    }
}
