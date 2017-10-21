/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0

Rectangle {
    id:             instrumentPanel
    height:         instrumentColumn.height + (_topBottomMargin * 2)
    width:          getPreferredInstrumentWidth()
    radius:         _showLargeCompass ? width / 2 :  ScreenTools.defaultFontPixelWidth / 2
    color:          _backgroundColor
    border.width:   _showLargeCompass ? 1 : 0
    border.color:   _isSatellite ? qgcPal.mapWidgetBorderLight : qgcPal.mapWidgetBorderDark

    property var    _qgcView:               qgcView
    property real   _maxHeight:             maxHeight
    property real   _defaultSize:           ScreenTools.defaultFontPixelHeight * (9)
    property color  _backgroundColor:       qgcPal.window
    property real   _spacing:               ScreenTools.defaultFontPixelHeight * 0.33
    property real   _topBottomMargin:       (width * 0.05) / 2
    property real   _availableValueHeight:  _maxHeight - (outerCompass.height + _spacer1.height + _spacer2.height + (_spacing * 4)) - (_showLargeCompass ? compass.height : 0)
    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _showLargeCompass:      QGroundControl.settingsManager.appSettings.showLargeCompass.value

    readonly property real _outerRingRatio: 0.95
    readonly property real _innerRingRatio: 0.80

    QGCPalette { id: qgcPal }

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
            height: outerCompass.height

            CompassRing {
                id:                 outerCompass
                size:               parent.width * _outerRingRatio
                vehicle:            _activeVehicle
                anchors.horizontalCenter: parent.horizontalCenter
                visible:            !_showLargeCompass

            }

            QGCAttitudeWidget {
                id:                 attitudeWidget
                size:               parent.width * (_showLargeCompass ? _outerRingRatio : _innerRingRatio)
                vehicle:            _activeVehicle
                anchors.centerIn:   outerCompass
                showHeading:        !_showLargeCompass
            }

            Image {
                id:                 gearThingy
                anchors.bottom:     outerCompass.bottom
                anchors.right:      outerCompass.right
                source:             qgcPal.globalTheme == QGCPalette.Light ? "/res/gear-black.svg" : "/res/gear-white.svg"
                mipmap:             true
                opacity:            0.5
                width:              outerCompass.width * 0.15
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

            Image {
                id:                 healthWarning
                anchors.bottom:     outerCompass.bottom
                anchors.left:       outerCompass.left
                source:             "/qmlimages/Yield.svg"
                mipmap:             true
                visible:            _activeVehicle ? !_warningsViewed && _activeVehicle.unhealthySensors.length > 0 && _valuesWidget.currentPage() != 2 : false
                opacity:            0.8
                width:              outerCompass.width * 0.15
                sourceSize.width:   width
                fillMode:           Image.PreserveAspectFit

                property bool _warningsViewed: false

                MouseArea {
                    anchors.fill:   parent
                    hoverEnabled:   true
                    onEntered:      healthWarning.opacity = 1
                    onExited:       healthWarning.opacity = 0.8
                    onClicked:      {
                        _valuesWidget.showPage(2)
                        healthWarning._warningsViewed = true
                    }
                }

                Connections {
                    target: _activeVehicle
                    onUnhealthySensorsChanged: healthWarning._warningsViewed = false
                }
            }
        }

        Rectangle {
            id:                         _spacer1
            anchors.horizontalCenter:   parent.horizontalCenter
            height:                     1
            width:                      parent.width * 0.9
            color:                      qgcPal.text
        }

        Item {
            width:  parent.width
            height: _valuesWidget.height

            Rectangle {
                anchors.fill:   _valuesWidget
                color:          _backgroundColor
                radius:         _spacing
                visible:        !_showLargeCompass
            }

            InstrumentSwipeView {
                id:                 _valuesWidget
                anchors.margins:    1
                anchors.left:       parent.left
                anchors.right:      parent.right
                qgcView:            instrumentPanel._qgcView
                textColor:          qgcPal.text
                backgroundColor:    _backgroundColor
                maxHeight:          _availableValueHeight
            }
        }

        Rectangle {
            id:                         _spacer2
            anchors.horizontalCenter:   parent.horizontalCenter
            height:                     1
            width:                      parent.width * 0.9
            color:                      qgcPal.text
            visible:                    _showLargeCompass
        }

        QGCCompassWidget {
            id:                         compass
            anchors.horizontalCenter:   parent.horizontalCenter
            size:                       parent.width * 0.95
            vehicle:                    _activeVehicle
            visible:                    _showLargeCompass
        }
    }
}
