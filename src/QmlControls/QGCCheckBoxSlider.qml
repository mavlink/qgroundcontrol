/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * Volador Ground Control Station (VGCS) - Industrial Dark Theme Switch
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools

AbstractButton {
    id: control
    checkable: true
    padding: 0

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    contentItem: Item {
        implicitWidth: (label.visible ? label.contentWidth + ScreenTools.defaultFontPixelWidth * 2 : 0) + indicator.width 
        implicitHeight: Math.max(label.contentHeight, indicator.height)

        QGCLabel { 
            id: label
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: visible ? control.text : ""
            visible: control.text !== ""
        }
    
        Rectangle {
            id: indicator
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: Math.round(ScreenTools.defaultFontPixelHeight * 1.1)
            width: Math.round(height * 2.1)
            radius: height / 2

            // OFF State: near black (#111111), Border: dark grey (#444444)
            // ON State: Volador neon orange (#FF6A00)
            color: control.checked ? "#FF6A00" : "#111111"
            border.width: 1
            border.color: control.checked ? "#FF8822" : "#444444"

            Behavior on color {
                ColorAnimation { duration: 180 }
            }
            Behavior on border.color {
                ColorAnimation { duration: 180 }
            }

            // Subtle Orange Glow Effect in ON state
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.color: "#FF6A00"
                border.width: 2
                opacity: control.checked ? 0.45 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 180 }
                }
            }

            // Sliding Knob
            // OFF State: grey (#777777)
            // ON State: white (#FFFFFF)
            Rectangle {
                id: knob
                anchors.verticalCenter: parent.verticalCenter
                x: control.checked ? (indicator.width - width - 2) : 2
                height: parent.height - 4
                width: height
                radius: height / 2
                color: control.checked ? "#FFFFFF" : "#777777"

                Behavior on x {
                    NumberAnimation { duration: 180; easing.type: Easing.InOutQuad }
                }
                Behavior on color {
                    ColorAnimation { duration: 180 }
                }
            }
        }
    }
}
