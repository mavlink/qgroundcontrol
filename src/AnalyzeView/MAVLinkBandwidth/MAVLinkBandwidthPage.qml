pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

AnalyzePage {
    id: root

    pageComponent: pageComponent
    pageDescription: qsTr("Measures end-to-end MAVLink payload goodput between QGroundControl and an ArduPilot Lua endpoint.")
    allowPopout: true

    MAVLinkBandwidthController {
        id: controller
    }

    QGCPalette {
        id: qgcPal

        colorGroupEnabled: true
    }

    function formatRate(rateKbps) {
        return qsTr("%1 kbit/s").arg(rateKbps.toFixed(1))
    }

    function formatBytes(bytes) {
        return Number(bytes).toLocaleString(Qt.locale(), "f", 0)
    }

    Component {
        id: pageComponent

        QGCFlickable {
            width: root.availableWidth
            height: root.availableHeight
            contentWidth: width
            contentHeight: contentColumn.implicitHeight

            ColumnLayout {
                id: contentColumn

                width: parent.width
                spacing: ScreenTools.defaultFontPixelHeight * 0.75

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.35

                    QGCLabel { text: qsTr("Primary link") }
                    QGCLabel {
                        Layout.fillWidth: true
                        text: controller.linkName.length > 0 ? controller.linkName : qsTr("Unavailable")
                    }

                    QGCLabel { text: qsTr("Lua endpoint") }
                    QGCLabel {
                        Layout.fillWidth: true
                        text: controller.endpointAvailable ? qsTr("Ready") : qsTr("Not detected")
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.minimumHeight: statusLabel.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.8)
                    color: qgcPal.windowShade
                    radius: ScreenTools.defaultFontPixelWidth * 0.5

                    QGCLabel {
                        id: statusLabel

                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelHeight * 0.4
                        text: controller.statusText
                        wrapMode: Text.WordWrap
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCButton {
                        text: qsTr("Probe Endpoint")
                        enabled: controller.vehicleAvailable && !controller.running
                        onClicked: controller.probeEndpoint()
                    }

                    Item { Layout.fillWidth: true }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5

                    QGCLabel { text: qsTr("Direction") }
                    QGCComboBox {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 26
                        model: [qsTr("QGC to vehicle"), qsTr("Vehicle to QGC")]
                        currentIndex: controller.direction === MAVLinkBandwidthController.QgcToVehicle ? 0 : 1
                        enabled: !controller.running
                        onActivated: (index) => {
                            controller.direction = index === 0 ? MAVLinkBandwidthController.QgcToVehicle
                                                               : MAVLinkBandwidthController.VehicleToQgc
                        }
                    }

                    QGCLabel { text: qsTr("Target payload rate") }
                    QGCTextField {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                        text: controller.targetRateKbps.toString()
                        enabled: !controller.running
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 1; top: 2000 }
                        onEditingFinished: {
                            controller.targetRateKbps = Number(text)
                            text = controller.targetRateKbps.toString()
                        }
                    }

                    QGCLabel { text: qsTr("Rate units") }
                    QGCLabel { text: qsTr("kbit/s of test payload") }

                    QGCLabel { text: qsTr("Duration") }
                    QGCTextField {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                        text: controller.durationSeconds.toString()
                        enabled: !controller.running
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 1; top: 60 }
                        onEditingFinished: {
                            controller.durationSeconds = Number(text)
                            text = controller.durationSeconds.toString()
                        }
                    }

                    QGCLabel { text: qsTr("Duration units") }
                    QGCLabel { text: qsTr("seconds") }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCButton {
                        text: qsTr("Start Test")
                        enabled: controller.canStart
                        onClicked: controller.startTest()
                    }

                    QGCButton {
                        text: qsTr("Stop")
                        enabled: controller.running
                        onClicked: controller.stopTest()
                    }

                    Item { Layout.fillWidth: true }
                }

                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: controller.progress
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.35

                    QGCLabel { text: qsTr("Payload transmit rate") }
                    QGCLabel { text: root.formatRate(controller.transmitRateKbps) }

                    QGCLabel { text: qsTr("Payload receive rate") }
                    QGCLabel { text: root.formatRate(controller.receiveRateKbps) }

                    QGCLabel { text: qsTr("Total link transmit rate") }
                    QGCLabel { text: root.formatRate(controller.wireTransmitRateKbps) }

                    QGCLabel { text: qsTr("Total link receive rate") }
                    QGCLabel { text: root.formatRate(controller.wireReceiveRateKbps) }

                    QGCLabel { text: qsTr("Payload bytes sent") }
                    QGCLabel { text: root.formatBytes(controller.transmittedPayloadBytes) }

                    QGCLabel { text: qsTr("Payload bytes received") }
                    QGCLabel { text: root.formatBytes(controller.receivedPayloadBytes) }

                    QGCLabel { text: qsTr("Packets sent") }
                    QGCLabel { text: root.formatBytes(controller.transmittedPackets) }

                    QGCLabel { text: qsTr("Packets received") }
                    QGCLabel { text: root.formatBytes(controller.receivedPackets) }

                    QGCLabel { text: qsTr("Packets lost") }
                    QGCLabel { text: root.formatBytes(controller.lostPackets) }

                    QGCLabel { text: qsTr("Out-of-order packets") }
                    QGCLabel { text: root.formatBytes(controller.outOfOrderPackets) }

                    QGCLabel { text: qsTr("Send backpressure/failures") }
                    QGCLabel { text: root.formatBytes(controller.sendFailures) }
                }
            }
        }
    }
}
