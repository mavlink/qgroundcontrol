import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0

Rectangle {
    width: 600
    height: 400

    QGCPalette { id: palette; colorGroup: QGCPalette.Active }

    id: topLevel
    objectName: "topLevel"

    color: palette.window
    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        smooth: true
        source: autopilot.setupBackgroundImage;
    }
    Column {
        anchors.margins: 20
        anchors.fill: parent
        spacing: 5

        Flow {
            width: parent.width;
            height: parent.height
            spacing: 5

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

                            border.color: "#888"
                            radius: 10

                            color: "white"
                            opacity: 0.8

                            Text {
                                id: titleBar

                                width: parent.width
                                height: parent.titleHeight

                                verticalAlignment: TextEdit.AlignVCenter
                                horizontalAlignment: TextEdit.AlignHCenter

                                text: control.text
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
                                    color: control.setupComplete ? "green" : "red"
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: parent.height - parent.titleHeight

                                y: parent.titleHeight

                                border.color: "#888"

                                gradient: Gradient {
                                    GradientStop { position: 0; color: "#ffffff" }
                                    GradientStop { position: 1; color: "#000000" }
                                }

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
}
