import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtGraphicalEffects 1.0

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Button {
    checkable: true
    height: 60

    text: "Button"
    property bool setupComplete: true
    property bool setupIndicator: true
    property string imageResource: "/qmlimages/subMenuButtonImage.png"

    style: ButtonStyle {
        id: buttonStyle

        property var __qgcPal: QGCPalette {
            colorGroupEnabled: control.enabled
        }

        property bool __showHighlight: control.pressed | control.checked

        background: Rectangle {
            id: innerRect
            readonly property real titleHeight: 20

            color: __showHighlight ? __qgcPal.buttonHighlight : __qgcPal.button

            Text {
                id: titleBar

                width: parent.width
                height: parent.titleHeight

                verticalAlignment: TextEdit.AlignVCenter
                horizontalAlignment: TextEdit.AlignHCenter

                text: control.text
                font.pixelSize: ScreenTools.defaultFontPixelSize
                antialiasing: true
                color: __showHighlight ? __qgcPal.buttonHighlightText : __qgcPal.buttonText

                Rectangle {
                    id: setupIndicator

                    readonly property real indicatorRadius: 4

                    x: parent.width - (indicatorRadius * 2) - 3
                    y: (parent.height - (indicatorRadius * 2)) / 2
                    width: indicatorRadius * 2
                    height: indicatorRadius * 2

                    radius: indicatorRadius
                    color: control.setupIndicator ? (control.setupComplete ? "#00d932" : "red") : innerRect.color
                }
            }

            Rectangle {
                width: parent.width
                height: parent.height - parent.titleHeight

                y: parent.titleHeight

                color: __qgcPal.windowShade

                QGCColoredImage {
                    source: control.imageResource
                    fillMode: Image.PreserveAspectFit
                    width: parent.width - 20
                    height: parent.height - 20
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    smooth: true
                    color: __showHighlight ? __qgcPal.buttonHighlight : __qgcPal.button
                }
            }
        }

    label: Item {}
    }
}
