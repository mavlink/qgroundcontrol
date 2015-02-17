import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtGraphicalEffects 1.0

import QGroundControl.Palette 1.0

Button {
    checkable: true
    height: 60

    text: "Button"
    property bool setupComplete: true
    property bool setupIndicator: true

    style: ButtonStyle {
        id: buttonStyle

        property var __qgcpal: QGCPalette {
            colorGroup: control.enabled ? QGCPalette.Active : QGCPalette.Disabled
        }

        background: Rectangle {
            id: innerRect
            readonly property real titleHeight: 20

            border.color: control.checked ? "#eee333" : "#676767"
            radius: 10

            color: control.checked ? "#eee333" : "#343434"

            Text {
                id: titleBar

                width: parent.width
                height: parent.titleHeight

                verticalAlignment: TextEdit.AlignVCenter
                horizontalAlignment: TextEdit.AlignHCenter

                text: control.text
                font.pixelSize: 12
                color: control.checked ? "black" : "white"

                Rectangle {
                    id: setupIndicator

                    readonly property real indicatorRadius: 4

                    x: parent.width - (indicatorRadius * 2) - 5
                    y: (parent.height - (indicatorRadius * 2)) / 2
                    width: indicatorRadius * 2
                    height: indicatorRadius * 2

                    radius: indicatorRadius
                    color: control.setupIndicator ? (control.setupComplete ? "green" : "red") : innerRect.color
                }
            }

            Rectangle {
                width: parent.width
                height: parent.height - parent.titleHeight

                y: parent.titleHeight

                color: __qgcpal.window
                border.color: control.checked ? "#eee333" : "#676767"

                Image {
                    id: buttonImage
                    source: "setupButtonImage.png"
                    sourceSize: Qt.size(parent.width - 20, parent.height - 20)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    smooth: true
                    visible: false
                }

                ColorOverlay {
                    anchors.fill: buttonImage
                    source: buttonImage
                    color: control.checked ? "#eee333" : "#58585a"
                }
            }
        }

    label: Item {}
    }
}
