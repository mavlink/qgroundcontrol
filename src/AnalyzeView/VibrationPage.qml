/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

AnalyzePage {
    id:                 geoTagPage
    pageComponent:      pageComponent
    pageDescription:    qsTr("Analyze vibration associated with your vehicle.")
    allowPopout:        true

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property bool   _available:     !isNaN(_activeVehicle.vibration.xAxis.rawValue)
    property real   _margins:       ScreenTools.defaultFontPixelWidth / 2
    property real   _barWidth:      ScreenTools.defaultFontPixelWidth * 3
    property real   _barHeight:     ScreenTools.defaultFontPixelHeight * 10
    property real   _xValue:        _activeVehicle.vibration.xAxis.rawValue
    property real   _yValue:        _activeVehicle.vibration.yAxis.rawValue
    property real   _zValue:        _activeVehicle.vibration.zAxis.rawValue

    readonly property real _barMinimum:     0.0
    readonly property real _barMaximum:     90.0
    readonly property real _barBadValue:    60.0

    QGCPalette { id:qgcPal; colorGroupEnabled: true }

    Component {
        id: pageComponent

        Item {
            width:  childrenRect.width
            height: childrenRect.height

            RowLayout {
                id:         barRow
                spacing:    ScreenTools.defaultFontPixelWidth * 4

                ColumnLayout {
                    Rectangle {
                        id:                 xBar
                        height:             _barHeight
                        width:              _barWidth
                        Layout.alignment:   Qt.AlignHCenter
                        border.width:       1
                        border.color:       qgcPal.text

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width:          parent.width
                            height:         parent.height * (Math.min(_barMaximum, _xValue) / (_barMaximum - _barMinimum))
                            color:          qgcPal.text
                        }
                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("X")
                    }
                }

                Column {
                    Rectangle {
                        height:             _barHeight
                        width:              _barWidth
                        Layout.alignment:   Qt.AlignHCenter
                        border.width:       1
                        border.color:       qgcPal.text

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width:          parent.width
                            height:         parent.height * (Math.min(_barMaximum, _yValue) / (_barMaximum - _barMinimum))
                            color:          qgcPal.text
                        }
                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("Y")
                    }
                }

                Column {
                    Rectangle {
                        height:             _barHeight
                        width:              _barWidth
                        Layout.alignment:   Qt.AlignHCenter
                        border.width:       1
                        border.color:       qgcPal.text

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width:          parent.width
                            height:         parent.height * (Math.min(_barMaximum, _zValue) / (_barMaximum - _barMinimum))
                            color:          qgcPal.text
                        }
                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("Z")
                    }
                }
            }

            // Max vibe indication line at 60
            Rectangle {
                anchors.topMargin:      xBar.height * (1.0 - ((_barBadValue - _barMinimum) / (_barMaximum - _barMinimum)))
                anchors.top:            barRow.top
                anchors.left:           barRow.left
                anchors.right:          barRow.right
                width:                  barRow.width
                height:                 1
                color:                  "red"
            }

            Column {
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.left:       barRow.right

                QGCLabel {
                    text: qsTr("Clip count")
                }

                QGCLabel {
                    text: qsTr("Accel 1: ") + (_activeVehicle.vibration.clipCount1.rawValueString)
                }

                QGCLabel {
                    text: qsTr("Accel 2: ") + (_activeVehicle.vibration.clipCount2.rawValueString)
                }

                QGCLabel {
                    text: qsTr("Accel 3: ") + (_activeVehicle.vibration.clipCount3.rawValueString)
                }
            }

            Rectangle {
                anchors.fill:   parent
                color:          qgcPal.window
                opacity:        0.75
                visible:        !_available

                QGCLabel {
                    anchors.fill:           parent
                    horizontalAlignment:    Text.AlignHCenter
                    verticalAlignment:      Text.AlignVCenter
                    text:                   qsTr("Not Available")
                }
            }
        }
    }
}
