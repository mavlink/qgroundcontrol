import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtGraphicalEffects 1.0
import QGroundControl.FactSystem 1.0

Button {
    checkable: true
    height: 30

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
            readonly property real titleHeight: 30

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

                    readonly property real indicatorRadius: 6

                    x: parent.width - (indicatorRadius * 2) - 5
                    y: (parent.height - (indicatorRadius * 2)) / 2
                    width: indicatorRadius * 2
                    height: indicatorRadius * 2

                    radius: indicatorRadius
                    color: control.setupIndicator ? (control.setupComplete ? "green" : "red") : innerRect.color
                }
            }
        }

    label: Item {}
    }
}
