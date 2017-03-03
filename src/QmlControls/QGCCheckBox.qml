import QtQuick                  2.7
import QtQuick.Controls         2.1
import QtQuick.Controls.Styles  1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

CheckBox {
    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    style: CheckBoxStyle {
        label: Item {
            implicitWidth:  text.implicitWidth + 2
            implicitHeight: text.implicitHeight
            baselineOffset: text.baselineOffset

            Rectangle {
                anchors.margins:    -1
                anchors.leftMargin: -3
                anchors.rightMargin: -3
                anchors.fill:       text
                visible:            control.activeFocus
                height:             6
                radius:             3
                color:              "#224f9fef"
                border.color:       "#47b"
                opacity:            0.6
            }

            Text {
                id:             text
                text:           control.text
                antialiasing:   true
                font.pointSize: ScreenTools.defaultFontPointSize
                font.family:    ScreenTools.normalFontFamily
                color:          control.__qgcPal.text
                anchors.verticalCenter: parent.verticalCenter
            }
        } // label

        indicator:  Item {
            implicitWidth:  ScreenTools.implicitCheckBoxWidth
            implicitHeight: implicitWidth

            Rectangle {
                anchors.fill: parent
                anchors.bottomMargin: -1
                color: "#44ffffff"
                radius: baserect.radius
            }

            Rectangle {
                id: baserect
                gradient: Gradient {
                    GradientStop {color: "#eee" ; position: 0}
                    GradientStop {color: control.pressed ? "#eee" : "#fff" ; position: 0.1}
                    GradientStop {color: "#fff" ; position: 1}
                }
                radius: ScreenTools.defaultFontPixelHeight * 0.16
                anchors.fill: parent
                border.color: control.activeFocus ? "#47b" : "#999"
            }

            Image {
                source: "/qmlimages/check.png"
                opacity: control.checkedState === Qt.Checked ? control.enabled ? 1 : 0.5 : 0
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 1
                Behavior on opacity {NumberAnimation {duration: 80}}
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: Math.round(baserect.radius)
                antialiasing: true
                gradient: Gradient {
                    GradientStop {color: control.pressed ? "#555" : "#999" ; position: 0}
                    GradientStop {color: "#555" ; position: 1}
                }
                radius: baserect.radius - 1
                anchors.centerIn: parent
                anchors.alignWhenCentered: true
                border.color: "#222"
                Behavior on opacity {NumberAnimation {duration: 80}}
                opacity: control.checkedState === Qt.PartiallyChecked ? control.enabled ? 1 : 0.5 : 0
            }
        } // indicator
    } // style
}
