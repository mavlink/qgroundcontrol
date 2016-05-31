/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Fly View Widgets
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0

Item {
    id:     instrumentPanel
    height: instrumentColumn.y + instrumentColumn.height + _topBottomMargin
    width:  size

    property alias  heading:        compass.heading
    property alias  rollAngle:      attitudeWidget.rollAngle
    property alias  pitchAngle:     attitudeWidget.pitchAngle
    property real   size:           _defaultSize
    property bool   lightBorders:   true
    property bool   active:         false
    property var    qgcView
    property real   maxHeight

    property Fact   _emptyFact:         Fact { }
    property Fact   groundSpeedFact:    _emptyFact
    property Fact   airSpeedFact:       _emptyFact

    property real   _defaultSize:   ScreenTools.defaultFontPixelHeight * (9)

    property color  _backgroundColor:   qgcPal.window
    property real   _spacing:           ScreenTools.defaultFontPixelHeight * 0.33
    property real   _topBottomMargin:   (size * 0.05) / 2
    property real   _availableValueHeight: maxHeight - (attitudeWidget.height + _spacer1.height + _spacer2.height + (_spacing * 4)) - (_showCompass ? compass.height : 0)
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle

    readonly property bool _showCompass:    true // !ScreenTools.isShortScreen

    QGCPalette { id: qgcPal }

    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         (_showCompass ? instrumentColumn.height : attitudeWidget.height) + (_topBottomMargin * 2)
        radius:         size / 2
        color:          _backgroundColor
        border.width:   1
        border.color:   lightBorders ? qgcPal.mapWidgetBorderLight : qgcPal.mapWidgetBorderDark
    }

    MouseArea {
        anchors.fill: parent
        onClicked: _valuesWidget.showPicker()
    }

    Column {
        id:                 instrumentColumn
        anchors.topMargin:  _topBottomMargin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _spacing

        Item {
            width:  parent.width
            height: attitudeWidget.height

            QGCAttitudeWidget {
                id:             attitudeWidget
                size:           parent.width * 0.95
                active:         instrumentPanel.active
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Image {
                id:                 gearThingy
                anchors.bottom:     attitudeWidget.bottom
                anchors.right:      attitudeWidget.right
                source:             qgcPal.globalTheme == QGCPalette.Light ? "/res/gear-black.svg" : "/res/gear-white.svg"
                mipmap:             true
                opacity:            0.5
                width:              attitudeWidget.width * 0.15
                sourceSize.width:   width
                fillMode:           Image.PreserveAspectFit
                MouseArea {
                    anchors.fill:   parent
                    hoverEnabled:   true
                    onEntered:      gearThingy.opacity = 0.85
                    onExited:       gearThingy.opacity = 0.5
                    onClicked:      _valuesWidget.showPicker()
                }
            }
        }

        Rectangle {
            id:                 _spacer1
            height:             1
            width:              parent.width * 0.9
            color:              qgcPal.text
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Item {
            width:  parent.width
            height: _valuesWidget.height

            Rectangle {
                anchors.fill:   _valuesWidget
                color:          _backgroundColor
                visible:        !_showCompass
                radius:         _spacing
            }

            InstrumentSwipeView {
                id:                 _valuesWidget
                width:              parent.width
                qgcView:            instrumentPanel.qgcView
                textColor:          qgcPal.text
                backgroundColor:    _backgroundColor
                maxHeight:          _availableValueHeight
            }
        }

        Rectangle {
            id:                 _spacer2
            height:             1
            width:              parent.width * 0.9
            color:              qgcPal.text
            visible:            _showCompass
            anchors.horizontalCenter: parent.horizontalCenter
        }

        QGCCompassWidget {
            id:                 compass
            size:               parent.width * 0.95
            active:             instrumentPanel.active
            visible:            _showCompass
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
