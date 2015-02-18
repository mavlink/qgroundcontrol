import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0

Rectangle {
    width: 600
    height: 400

    property var qgcPal: QGCPalette { id: palette; colorGroup: QGCPalette.Active }

    id: topLevel
    objectName: "topLevel"

    color: qgcPal.window

    Flow {
        anchors.fill: parent
        spacing: 10

        Repeater {
            model: autopilot.components

            Button {
                width: 250
                height: 200

                property var summaryQmlSource: modelData.summaryQmlSource
                text: modelData.name
                property bool setupComplete: modelData.setupComplete

                style: ButtonStyle {
                    id: buttonStyle
                    background: Rectangle {
                        id: innerRect
                        readonly property real titleHeight: 30

                        color: qgcPal.windowShadeDark

                        Text {
                            id: titleBar

                            width: parent.width
                            height: parent.titleHeight

                            verticalAlignment: TextEdit.AlignVCenter
                            horizontalAlignment: TextEdit.AlignHCenter

                            text: control.text.toUpperCase()
                            color: qgcPal.buttonText
                            font.pixelSize: 12

                            Rectangle {
                                id: setupIndicator

                                property bool setupComplete: true
                                readonly property real indicatorRadius: 6

                                x: parent.width - (indicatorRadius * 2) - 5
                                y: (parent.height - (indicatorRadius * 2)) / 2
                                width: indicatorRadius * 2
                                height: indicatorRadius * 2

                                radius: indicatorRadius
                                color: control.setupComplete ? "#00d932" : "red"
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: parent.height - parent.titleHeight

                            y: parent.titleHeight

                            color: qgcPal.windowShade

                            Loader {
                                anchors.fill: parent
                                source: summaryQmlSource
                            }
                        }
                    }

                label: Item {}
                }
            }
        }
    }
}
