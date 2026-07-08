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

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.SettingsManager

/// Map scale control
Item {
    id:     scale
    width:  buttonsOnLeft || !_zoomButtonsVisible ? rightEnd.x + rightEnd.width : zoomDownButton.x + zoomDownButton.width
    height: rightEnd.y + rightEnd.height

    property var    mapControl                      ///< Map control for which this scale control is being used
    property bool   terrainButtonVisible:   false
    property alias  terrainButtonChecked:   terrainButton.checked
    property bool   zoomButtonsVisible:     true
    property bool   buttonsOnLeft:          true    ///< Buttons to left/right of scale bar
    property var    backdropSourceItem:     mapControl

    signal terrainButtonClicked

    property var    _scaleLengthsMeters:    [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
    property var    _scaleLengthsFeet:      [10, 25, 50, 100, 250, 500, 1000, 2000, 3000, 4000, 5280, 5280*2, 5280*5, 5280*10, 5280*25, 5280*50, 5280*100, 5280*250, 5280*500, 5280*1000]
    property bool   _zoomButtonsVisible:    zoomButtonsVisible && !ScreenTools.isMobile
    property var    _color:                 mapControl.isSatelliteMap ? "white" : "black"
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
        target:             mapControl
        function onWidthChanged() {     scaleTimer.restart() }
        function onHeightChanged() {    scaleTimer.restart() }
        function onZoomLevelChanged() { scaleTimer.restart() }
    }

    Timer {
        id:                 scaleTimer
        interval:           100
        running:            false
        repeat:             false
        onTriggered:        calculateScale()
    }

    QGCMapLabel {
        id:                 scaleText
        map:                mapControl
        font.bold:          true
        anchors.left:       parent.left
        anchors.right:      rightEnd.right
        horizontalAlignment:Text.AlignRight
        text:               "0 m"
    }

    Rectangle {
        id:                 leftEnd
        anchors.top:        scaleText.bottom
        anchors.leftMargin: buttonsOnLeft && (_zoomButtonsVisible || terrainButtonVisible) ? ScreenTools.defaultFontPixelWidth / 2 : 0
        anchors.left:       buttonsOnLeft ?
                                (_zoomButtonsVisible ? zoomDownButton.right : (terrainButtonVisible ? terrainButton.right : parent.left)) :
                                parent.left
        width:              2
        height:             ScreenTools.defaultFontPixelHeight
        color:              _color
    }

    Rectangle {
        id:                 centerLine
        anchors.bottomMargin:   2
        anchors.bottom:     leftEnd.bottom
        anchors.left:       leftEnd.right
        height:             2
        color:              _color
    }

    Rectangle {
        id:                 rightEnd
        anchors.top:        leftEnd.top
        anchors.left:       centerLine.right
        width:              2
        height:             ScreenTools.defaultFontPixelHeight
        color:              _color
    }

    component MapControlButton: Rectangle {
        id: mapButton

        property bool plus: true
        property bool checked: false
        property string iconSource: ""
        property var backdropSourceItem: scale.backdropSourceItem
        property color _symbolColor: Qt.rgba(1, 1, 1, enabled ? 0.92 : 0.48)
        property real _symbolLength: Math.max(ScreenTools.defaultFontPixelHeight * 0.82, width * 0.42)
        property real _symbolThickness: Math.max(2.5, width * 0.065)
        signal clicked()

        width:              height
        radius:             Math.round(height * 0.16)
        color:              "transparent"
        border.color:       checked ? Qt.rgba(0.82, 0.90, 0.95, 0.30) :
                                (mapButtonMouse.containsMouse ? Qt.rgba(0.82, 0.90, 0.95, 0.24) : Qt.rgba(0.82, 0.90, 0.95, 0.14))
        border.width:       1
        clip:               true

        GlassBackdrop {
            anchors.fill:       parent
            sourceItem:         mapButton.backdropSourceItem
            backdropBlurEnabled:true
            targetItem:         mapButton
            cornerRadius:       mapButton.radius
            sourceScale:        0.46
            blurAmount:         0.94
            blurMax:            42
            sourceBrightness:   -0.01
            sourceSaturation:   0.62
            tintColor:          Qt.rgba(0.045, 0.048, 0.052, 0.68)
            sheenColor:         "transparent"
        }

        Rectangle {
            anchors.fill: parent
            radius:       mapButton.radius
            color:        mapButtonMouse.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.44) :
                            (checked ? Qt.rgba(1, 1, 1, 0.060) :
                             (mapButtonMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.052) : "transparent"))
        }

        QGCColoredImage {
            anchors.centerIn:   parent
            width:              Math.max(ScreenTools.defaultFontPixelHeight * 0.94, parent.width * 0.48)
            height:             width
            sourceSize.width:   width
            sourceSize.height:  height
            source:             mapButton.iconSource
            visible:            source !== ""
            color:              mapButton._symbolColor
            fillMode:           Image.PreserveAspectFit
        }

        Item {
            anchors.centerIn:   parent
            width:              mapButton._symbolLength
            height:             width
            visible:            mapButton.iconSource === ""

            Rectangle {
                anchors.centerIn:   parent
                width:              parent.width
                height:             mapButton._symbolThickness
                radius:             height / 2
                color:              mapButton._symbolColor
            }

            Rectangle {
                anchors.centerIn:   parent
                width:              mapButton._symbolThickness
                height:             parent.height
                radius:             width / 2
                color:              mapButton._symbolColor
                visible:            mapButton.plus
            }
        }

        QGCMouseArea {
            id:             mapButtonMouse
            anchors.fill:   parent
            hoverEnabled:   !ScreenTools.isMobile
            onClicked:      mapButton.clicked()
        }
    }

    MapControlButton {
        id:                 terrainButton
        anchors.top:        scaleText.top
        anchors.bottom:     rightEnd.bottom
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.10
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.10
        anchors.leftMargin: buttonsOnLeft ? 0 : ScreenTools.defaultFontPixelWidth / 2
        anchors.left:       buttonsOnLeft ? parent.left : rightEnd.right
        iconSource:         "/res/terrain.svg"
        visible:            terrainButtonVisible
        onClicked:          terrainButtonClicked()
    }

    MapControlButton {
        id:                 zoomUpButton
        anchors.top:        scaleText.top
        anchors.bottom:     rightEnd.bottom
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.10
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.10
        anchors.leftMargin: terrainButton.visible ? ScreenTools.defaultFontPixelWidth / 2 : 0
        anchors.left:       terrainButton.visible ? terrainButton.right : terrainButton.left
        plus:               true
        visible:            _zoomButtonsVisible
        onClicked:          if (mapControl) mapControl.zoomLevel += 0.5
    }

    MapControlButton {
        id:                 zoomDownButton
        anchors.top:        scaleText.top
        anchors.bottom:     rightEnd.bottom
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.10
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.10
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
        anchors.left:       zoomUpButton.right
        plus:               false
        visible:            _zoomButtonsVisible
        onClicked:          if (mapControl) mapControl.zoomLevel -= 0.5
    }

    Component.onCompleted: {
        if (scale.visible) {
            calculateScale();
        }
    }
}
