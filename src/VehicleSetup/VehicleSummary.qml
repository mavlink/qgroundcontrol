import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {
    width: 600
    height: 400

    property var qgcPal: QGCPalette { id: palette; colorGroupEnabled: true }

    id: topLevel
    objectName: "topLevel"

    color: qgcPal.window

    Column {
        anchors.fill: parent

        QGCLabel {
            text: "VEHICLE SUMMARY"
            font.pointSize: 20
        }

        Item {
            // Just used as a spacer
            height: 20
            width: 10
        }

        Flow {
            width: parent.width
            spacing: 10

            Repeater {
                model: autopilot.components

                // Outer summary item rectangle
                Rectangle {
                    readonly property real titleHeight: 30

                    width:  250
                    height: 200
                    color:  qgcPal.windowShade

                    // Title bar
                    Rectangle {

                        width: parent.width
                        height: titleHeight
                        color: qgcPal.windowShadeDark

                        // Title text
                        Text {
                            anchors.fill:   parent

                            color:          qgcPal.buttonText
                            font.pixelSize: 12
                            text:           modelData.name.toUpperCase()

                            verticalAlignment:      TextEdit.AlignVCenter
                            horizontalAlignment:    TextEdit.AlignHCenter
                        }
                    }

                    // Setup indicator
                    Rectangle {
                        readonly property real indicatorRadius: 6
                        readonly property real indicatorRightInset: 5

                        x:      parent.width - (indicatorRadius * 2) - indicatorRightInset
                        y:      (parent.titleHeight - (indicatorRadius * 2)) / 2
                        width:  indicatorRadius * 2
                        height: indicatorRadius * 2
                        radius: indicatorRadius
                        color:  modelData.setupComplete ? "#00d932" : "red"
                    }

                    // Summary Qml
                    Rectangle {
                        y:      parent.titleHeight
                        width:  parent.width
                        height: parent.height - parent.titleHeight
                        color:  qgcPal.windowShade

                        Loader {
                            anchors.fill: parent
                            source: modelData.summaryQmlSource
                        }
                    }
                }
            }
        }
    }
}
