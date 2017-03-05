/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCFlickable {
    id:                 _root
    height:             Math.min(maxHeight, healthColumn.y + healthColumn.height)
    contentHeight:      healthColumn.y + healthColumn.height
    flickableDirection: Flickable.VerticalFlick
    clip:               true

    property var    qgcView
    property color  textColor
    property var    maxHeight

    property var    unhealthySensors:   QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.unhealthySensors : [ ]

    // Any time the unhealthy sensors list changes, switch to the health page
    onUnhealthySensorsChanged: {
        if (unhealthySensors.length != 0) {
            showPage(1)
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      showNextPage()
    }

    Column {
        id:     healthColumn
        width:  parent.width

        QGCLabel {
            width:                  parent.width
            horizontalAlignment:    Text.AlignHCenter
            color:                  textColor
            text:                   qsTr("Vehicle Health")
        }

        QGCLabel {
            width:                  parent.width
            horizontalAlignment:    Text.AlignHCenter
            color:                  textColor
            text:                   qsTr("All systems healthy")
            visible:                healthRepeater.count == 0
        }

        Repeater {
            id:     healthRepeater
            model:  unhealthySensors

            Row {
                Image {
                    source:             "/qmlimages/Yield.svg"
                    height:             ScreenTools.defaultFontPixelHeight
                    sourceSize.height:  height
                    fillMode:           Image.PreserveAspectFit
                }

                QGCLabel {
                    color:  textColor
                    text:   modelData
                }
            }
        }
    }
}
