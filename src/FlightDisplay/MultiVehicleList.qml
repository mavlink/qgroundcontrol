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
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.FlightMap     1.0

QGCListView {
    id:             missionItemEditorListView
    spacing:        ScreenTools.defaultFontPixelHeight / 2
    orientation:    ListView.Vertical
    model:          QGroundControl.multiVehicleManager.vehicles
    cacheBuffer:    _cacheBuffer < 0 ? 0 : _cacheBuffer
    clip:           true

    property real _margin:          ScreenTools.defaultFontPixelWidth / 2
    property real _cacheBuffer:     height * 2
    property real _widgetHeight:    ScreenTools.defaultFontPixelHeight * 3

    delegate: Rectangle {
        width:      parent.width
        height:     innerColumn.y + innerColumn.height + _margin
        color:      qgcPal.buttonHighlight
        opacity:    0.8
        radius:     _margin

        property var    _vehicle:   object
        property color  _textColor: "black"

        QGCPalette { id: qgcPal }

        Row {
            id:                 widgetLayout
            anchors.margins:    _margin
            anchors.top:        parent.top
            anchors.right:      parent.right
            spacing:            ScreenTools.defaultFontPixelWidth / 2
            layoutDirection:    Qt.RightToLeft

            QGCCompassWidget {
                size:       _widgetHeight
                active:     true
                heading:    _vehicle.heading.rawValue
            }

            QGCAttitudeWidget {
                size:       _widgetHeight
                active:     true
                rollAngle:  _vehicle.roll.rawValue
                pitchAngle: _vehicle.pitch.rawValue
            }
        }

        RowLayout {
            anchors.top:    widgetLayout.top
            anchors.bottom: widgetLayout.bottom
            anchors.left:   parent.left
            anchors.right:  widgetLayout.left
            spacing:        ScreenTools.defaultFontPixelWidth / 2

            QGCLabel {
                Layout.alignment:   Qt.AlignTop
                text:               _vehicle.id
                color:              _textColor
            }

            QGCLabel {
                text:               _vehicle.flightMode
                font.pointSize:     ScreenTools.largeFontPointSize
                color:              _textColor
            }
        }

        Column {
            id:                 innerColumn
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        widgetLayout.bottom
            spacing:            _margin

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         5
                color:          "green"
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:       "Arm"
                    visible:    !_vehicle.armed
                    onClicked:  _vehicle.armed = true
                }

                QGCButton {
                    text:       "Start"
                    visible:    _vehicle.armed && _vehicle.flightMode != _vehicle.missionFlightMode
                    onClicked:  _vehicle.flightMode = _vehicle.missionFlightMode
                }

                QGCButton {
                    text:       "Stop"
                    visible:    _vehicle.armed && _vehicle.pauseVehicleSupported
                    onClicked:  _vehicle.pauseVehicle()
                }

                QGCButton {
                    text:       "RTL"
                    visible:    _vehicle.armed && _vehicle.flightMode != _vehicle.rtlFlightMode
                    onClicked:  _vehicle.flightMode = _vehicle.rtlFlightMode
                }

                QGCButton {
                    text:       "Take control"
                    visible:    _vehicle.armed && _vehicle.flightMode != _vehicle.takeControlFlightMode
                    onClicked:  _vehicle.flightMode = _vehicle.takeControlFlightMode
                }

            }
        }
    }
} // QGCListView
