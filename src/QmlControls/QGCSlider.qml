/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Slider {
    property var _qgcPal: QGCPalette { colorGroupEnabled: enabled }

    style: SliderStyle {
        groove: Item {
            property color fillColor: "#49d"
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: Math.round(TextSingleton.implicitHeight * 4.5)
            implicitHeight: Math.max(6, Math.round(TextSingleton.implicitHeight * 0.3))

            Rectangle {
                radius: height/2
                anchors.fill: parent
                border.width: 1
                border.color: "#888"
                gradient: Gradient {
                    GradientStop { color: "#bbb" ; position: 0 }
                    GradientStop { color: "#ccc" ; position: 0.6 }
                    GradientStop { color: "#ccc" ; position: 1 }
                }
            }

            Item {
                clip: true
                width: styleData.handlePosition
                height: parent.height
                Rectangle {
                    anchors.fill: parent
                    border.color: Qt.darker(fillColor, 1.2)
                    radius: height/2
                    gradient: Gradient {
                        GradientStop {color: Qt.lighter(fillColor, 1.3)  ; position: 0}
                        GradientStop {color: fillColor ; position: 1.4}
                    }
                }
            }
        }
    }
}
