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
    id:                 vibrationPage
    pageComponent:      pageComponent
    pageDescription:    qsTr("Analyze vibration associated with your vehicle.")
    allowPopout:        true

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property bool   _available:     !isNaN(_activeVehicle.vibration.xAxis.rawValue)
    property real   _margins:       ScreenTools.defaultFontPixelWidth / 2
    property real   _barWidth:      ScreenTools.defaultFontPixelWidth * 7
    property real   _barHeight:     ScreenTools.defaultFontPixelHeight * 10
    property real   _xValue:        _activeVehicle.vibration.xAxis.rawValue
    property real   _yValue:        _activeVehicle.vibration.yAxis.rawValue
    property real   _zValue:        _activeVehicle.vibration.zAxis.rawValue

    readonly property real _barMinimum:     0.0
    readonly property real _barMaximum:     90.0
    readonly property real _barBadValue:    60.0
    readonly property real _barMidValue:    30.0

    QGCPalette { id:qgcPal; colorGroupEnabled: true }

    Component {
        id: pageComponent

        Item {
            width:  childrenRect.width
            height: childrenRect.height

            RowLayout {
                id:         barRow
                spacing:    ScreenTools.defaultFontPixelWidth * 2

                ColumnLayout {
                    Rectangle {
                        id:                 xBar
                        height:             _barHeight
                        width:              _barWidth
                        Layout.alignment:   Qt.AlignHCenter
                        color:              "transparent"
                        border.width:       1
                        border.color:       qgcPal.text

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width:          parent.width
                            height:         parent.height * (Math.min(_barMaximum, _xValue) / (_barMaximum - _barMinimum))
                            color:          qgcPal.text
                        }

                        // Max vibe indication line at 60
                        Rectangle {
                            anchors.topMargin:      parent.height * (1.0 - ((_barBadValue - _barMinimum) / (_barMaximum - _barMinimum)))
                            anchors.top:            parent.top
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            width:                  parent.width
                            height:                 1
                            color:                  "red"
                        }

                        // Mid vibe indication line at 30
                        Rectangle {
                            anchors.topMargin:      parent.height * (1.0 - ((_barMidValue - _barMinimum) / (_barMaximum - _barMinimum)))
                            anchors.top:            parent.top
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            width:                  parent.width
                            height:                 1
                            color:                  "red"
                        }
                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("X (%1)").arg(_xValue.toFixed(0))
                    }
                }

                ColumnLayout {
                    Rectangle {
                        height:             _barHeight
                        width:              _barWidth
                        Layout.alignment:   Qt.AlignHCenter
                        color:              "transparent"
                        border.width:       1
                        border.color:       qgcPal.text

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width:          parent.width
                            height:         parent.height * (Math.min(_barMaximum, _yValue) / (_barMaximum - _barMinimum))
                            color:          qgcPal.text
                        }

                        // Max vibe indication line at 60
                        Rectangle {
                            anchors.topMargin:      parent.height * (1.0 - ((_barBadValue - _barMinimum) / (_barMaximum - _barMinimum)))
                            anchors.top:            parent.top
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            width:                  parent.width
                            height:                 1
                            color:                  "red"
                        }

                        // Mid vibe indication line at 30
                        Rectangle {
                            anchors.topMargin:      parent.height * (1.0 - ((_barMidValue - _barMinimum) / (_barMaximum - _barMinimum)))
                            anchors.top:            parent.top
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            width:                  parent.width
                            height:                 1
                            color:                  "red"
                        }
                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("Y (%1)").arg(_yValue.toFixed(0))
                    }
                }

                ColumnLayout {
                    Rectangle {
                        height:             _barHeight
                        width:              _barWidth
                        Layout.alignment:   Qt.AlignHCenter
                        color:              "transparent"
                        border.width:       1
                        border.color:       qgcPal.text

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width:          parent.width
                            height:         parent.height * (Math.min(_barMaximum, _zValue) / (_barMaximum - _barMinimum))
                            color:          qgcPal.text
                        }

                        // Max vibe indication line at 60
                        Rectangle {
                            anchors.topMargin:      parent.height * (1.0 - ((_barBadValue - _barMinimum) / (_barMaximum - _barMinimum)))
                            anchors.top:            parent.top
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            width:                  parent.width
                            height:                 1
                            color:                  "red"
                        }

                        // Mid vibe indication line at 30
                        Rectangle {
                            anchors.topMargin:      parent.height * (1.0 - ((_barMidValue - _barMinimum) / (_barMaximum - _barMinimum)))
                            anchors.top:            parent.top
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            width:                  parent.width
                            height:                 1
                            color:                  "red"
                        }
                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("Z (%1)").arg(_zValue.toFixed(0))
                    }
                }
            }

            Column {
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.left:       barRow.right

                QGCLabel {
                    text: qsTr("Clip count")
                }

                QGCLabel {
                    text: qsTr("Accel 1: %1").arg(_activeVehicle.vibration.clipCount1.rawValue)
                }

                QGCLabel {
                    text: qsTr("Accel 2: %1").arg(_activeVehicle.vibration.clipCount2.rawValue)
                }

                QGCLabel {
                    text: qsTr("Accel 3: %1").arg(_activeVehicle.vibration.clipCount3.rawValue)
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
