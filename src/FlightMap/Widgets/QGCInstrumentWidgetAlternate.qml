/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.4

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0

/// Instrument panel shown when virtual thumbsticks are visible
Rectangle {
    id:             root
    height:         _outerRadius * 2
    radius:         _outerRadius
    color:          qgcPal.window
    border.width:   1
    border.color:   lightBorders ? qgcPal.mapWidgetBorderLight : qgcPal.mapWidgetBorderDark

    property alias  heading:        compass.heading
    property alias  rollAngle:      attitude.rollAngle
    property alias  pitchAngle:     attitude.pitchAngle
    property real   size:           _defaultSize
    property bool   active:         false
    property bool   lightBorders:   true
    property var    qgcView
    property real   maxHeight

    property Fact   _emptyFact:         Fact { }
    property Fact   groundSpeedFact:    _emptyFact
    property Fact   airSpeedFact:       _emptyFact
    property Fact   altitudeFact:       _emptyFact

    property real   _innerRadius: (width - (_topBottomMargin * 3)) / 4
    property real   _outerRadius: _innerRadius + _topBottomMargin

    property real   _defaultSize:   ScreenTools.defaultFontPixelHeight * (9)

    property real   _sizeRatio:     ScreenTools.isTinyScreen ? (size / _defaultSize) * 0.5 : size / _defaultSize
    property real   _bigFontSize:   ScreenTools.defaultFontPointSize * 2.5  * _sizeRatio
    property real   _normalFontSize:ScreenTools.defaultFontPointSize * 1.5  * _sizeRatio
    property real   _labelFontSize: ScreenTools.defaultFontPointSize * 0.75 * _sizeRatio
    property real   _spacing:       ScreenTools.defaultFontPixelHeight * 0.33
    property real   _topBottomMargin: (size * 0.05) / 2
    property real   _availableValueHeight: maxHeight - (root.height + _valuesItem.anchors.topMargin)

    QGCPalette { id: qgcPal }

    QGCAttitudeWidget {
        id:                 attitude
        anchors.leftMargin: _topBottomMargin
        anchors.left:       parent.left
        size:               _innerRadius * 2
        active:             root.active
        anchors.verticalCenter: parent.verticalCenter
    }

    QGCCompassWidget {
        id:                 compass
        anchors.leftMargin: _spacing
        anchors.left:       attitude.right
        size:               _innerRadius * 2
        active:             root.active
        anchors.verticalCenter: parent.verticalCenter
    }

    Image {
        id:                         gearThingy
        anchors.bottomMargin:       _topBottomMargin
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        source:                     qgcPal.globalTheme == QGCPalette.Light ? "/res/gear-black.svg" : "/res/gear-white.svg"
        mipmap:                     true
        opacity:                    0.5
        width:                      root.height * 0.15
        sourceSize.width:           width
        fillMode:                   Image.PreserveAspectFit
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      _valuesWidget.showPicker()
    }

    Item {
        id:                 _valuesItem
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 4
        anchors.top:        parent.bottom
        width:              parent.width
        height:             _valuesWidget.height

        Rectangle {
            anchors.fill:   _valuesWidget
            color:          qgcPal.window
        }

        InstrumentSwipeView {
            id:                 _valuesWidget
            anchors.margins:    1
            anchors.left:       parent.left
            anchors.right:      parent.right
            qgcView:            root.qgcView
            textColor:          qgcPal.text
            backgroundColor:    qgcPal.window
            maxHeight:          _availableValueHeight
        }
    }


}
