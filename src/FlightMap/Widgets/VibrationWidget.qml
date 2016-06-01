/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.4
import QtQuick.Controls 1.4

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl               1.0

QGCFlickable {
    id:                 _root
    height:             Math.min(maxHeight, innerItem.height)
    contentHeight:      innerItem.height
    flickableDirection: Flickable.VerticalFlick
    clip:               true

    property color  textColor
    property color  backgroundColor
    property var    maxHeight

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.disconnectedVehicle
    property real   _margins:       ScreenTools.defaultFontPixelWidth / 2
    property real   _barWidth:      Math.round(ScreenTools.defaultFontPixelWidth * 3)

    readonly property real _barMinimum:     0.0
    readonly property real _barMaximum:     90.0
    readonly property real _barBadValue:    60.0

    QGCPalette { id:qgcPal; colorGroupEnabled: true }

    Item {
        id:     innerItem
        width:  parent.width
        height: barRow.y + barRow.height

        QGCLabel {
            id:     title
            color:  textColor
            text:   qsTr("Vibe")
            anchors.horizontalCenter: barRow.horizontalCenter
        }

        Row {
            id:                 barRow
            anchors.margins:    _margins
            anchors.top:        title.bottom
            anchors.left:       parent.left
            spacing:            _margins

            Column {
                ProgressBar {
                    id:             xBar
                    height:         50
                    orientation:    Qt.Vertical
                    minimumValue:   _barMinimum
                    maximumValue:   _barMaximum
                    value:          _activeVehicle ? _activeVehicle.vibration.xAxis.value : 0
                }

                QGCLabel {
                    id:     xBarLabel
                    color:  textColor
                    text:   "X"
                    anchors.horizontalCenter: xBar.horizontalCenter
                }
            }

            Column {
                ProgressBar {
                    id:             yBar
                    height:         50
                    orientation:    Qt.Vertical
                    minimumValue:   _barMinimum
                    maximumValue:   _barMaximum
                    value:          _activeVehicle ? _activeVehicle.vibration.yAxis.value : 0
                }

                QGCLabel {
                    anchors.horizontalCenter: yBar.horizontalCenter
                    color:  textColor
                    text:   "Y"
                }
            }

            Column {
                ProgressBar {
                    id:             zBar
                    height:         50
                    orientation:    Qt.Vertical
                    minimumValue:   _barMinimum
                    maximumValue:   _barMaximum
                    value:          _activeVehicle ? _activeVehicle.vibration.zAxis.value : 0
                }

                QGCLabel {
                    anchors.horizontalCenter: zBar.horizontalCenter
                    color:  textColor
                    text:   "Z"
                }
            }
        } // Row

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

        QGCLabel {
            id:                 clipLabel
            anchors.margins:    _margins
            anchors.left:       barRow.right
            anchors.right:      parent.right
            color:              textColor
            text:               qsTr("Clip count")
            horizontalAlignment: Text.AlignHCenter
        }

        Column {
            id:             clipColumn
            anchors.top:    barRow.top
            anchors.horizontalCenter: clipLabel.horizontalCenter

            QGCLabel {
                text: qsTr("Accel 1: ") + (_activeVehicle ? _activeVehicle.vibration.clipCount1.valueString : "")
                color: textColor
            }

            QGCLabel {
                text: qsTr("Accel 2: ") + (_activeVehicle ? _activeVehicle.vibration.clipCount2.valueString : "")
                color: textColor
            }

            QGCLabel {
                text: qsTr("Accel 2: ") + (_activeVehicle ? _activeVehicle.vibration.clipCount3.valueString : "")
                color: textColor
            }
        }

        // Not available overlay
        Rectangle {
            anchors.fill:   parent
            color:          backgroundColor
            opacity:        0.95
            visible:        _activeVehicle ? isNaN(_activeVehicle.vibration.xAxis.value) : false

            QGCLabel {
                anchors.fill:   parent
                text:           qsTr("Not Available")
                color:          textColor
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
            }
        }
    } // Item
} // QGCFLickable
