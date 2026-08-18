pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

AnalyzePage {
    id: root

    pageComponent: pageComponent
    pageDescription: qsTr("Measures end-to-end MAVLink payload goodput using an ArduPilot streaming endpoint or stock MAVFTP.")
    allowPopout: true

    readonly property bool _streamingMode: controller.testMode === MAVLinkBandwidthController.Streaming

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

                    QGCLabel {
                        visible: root._streamingMode
                        text: qsTr("Lua endpoint")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root._streamingMode
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
                    visible: root._streamingMode
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCButton {
                        text: qsTr("Probe Endpoint")
                        visible: root._streamingMode
                        enabled: controller.streamingAvailable && !controller.running
                        onClicked: controller.probeEndpoint()
                    }

                    Item { Layout.fillWidth: true }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5

                    QGCLabel { text: qsTr("Test mode") }
                    QGCComboBox {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 26
                        model: [qsTr("Streaming (ArduPilot Lua)"), qsTr("MAVFTP (PX4 or ArduPilot)")]
                        currentIndex: root._streamingMode ? 0 : 1
                        enabled: !controller.running
                        onActivated: (index) => {
                            controller.testMode = index === 0 ? MAVLinkBandwidthController.Streaming
                                                              : MAVLinkBandwidthController.MavFtp
                        }
                    }

                    QGCLabel {
                        visible: root._streamingMode
                        text: qsTr("Direction")
                    }
                    QGCComboBox {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 26
                        visible: root._streamingMode
                        model: [qsTr("QGC to vehicle"), qsTr("Vehicle to QGC")]
                        currentIndex: controller.direction === MAVLinkBandwidthController.QgcToVehicle ? 0 : 1
                        enabled: !controller.running
                        onActivated: (index) => {
                            controller.direction = index === 0 ? MAVLinkBandwidthController.QgcToVehicle
                                                               : MAVLinkBandwidthController.VehicleToQgc
                        }
                    }

                    QGCLabel {
                        visible: root._streamingMode
                        text: qsTr("Target payload rate")
                    }
                    QGCTextField {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                        visible: root._streamingMode
                        text: controller.targetRateKbps.toString()
                        enabled: !controller.running
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 1; top: 2000 }
                        onEditingFinished: {
                            controller.targetRateKbps = Number(text)
                            text = controller.targetRateKbps.toString()
                        }
                    }

                    QGCLabel {
                        visible: root._streamingMode
                        text: qsTr("Rate units")
                    }
                    QGCLabel {
                        visible: root._streamingMode
                        text: qsTr("kbit/s of test payload")
                    }

                    QGCLabel {
                        visible: root._streamingMode
                        text: qsTr("Duration")
                    }
                    QGCTextField {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                        visible: root._streamingMode
                        text: controller.durationSeconds.toString()
                        enabled: !controller.running
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 1; top: 60 }
                        onEditingFinished: {
                            controller.durationSeconds = Number(text)
                            text = controller.durationSeconds.toString()
                        }
                    }

                    QGCLabel {
                        visible: root._streamingMode
                        text: qsTr("Duration units")
                    }
                    QGCLabel {
                        visible: root._streamingMode
                        text: qsTr("seconds")
                    }

                    QGCLabel {
                        visible: !root._streamingMode
                        text: qsTr("Test file size")
                    }
                    QGCTextField {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                        visible: !root._streamingMode
                        text: controller.ftpFileSizeKiB.toString()
                        enabled: !controller.running
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 64; top: 10240 }
                        onEditingFinished: {
                            controller.ftpFileSizeKiB = Number(text)
                            text = controller.ftpFileSizeKiB.toString()
                        }
                    }

                    QGCLabel {
                        visible: !root._streamingMode
                        text: qsTr("Size units")
                    }
                    QGCLabel {
                        visible: !root._streamingMode
                        text: qsTr("KiB")
                    }
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

                    QGCLabel {
                        text: root._streamingMode ? qsTr("Payload transmit rate") : qsTr("Upload payload rate")
                    }
                    QGCLabel { text: root.formatRate(controller.transmitRateKbps) }

                    QGCLabel {
                        text: root._streamingMode ? qsTr("Payload receive rate") : qsTr("Download payload rate")
                    }
                    QGCLabel { text: root.formatRate(controller.receiveRateKbps) }

                    QGCLabel {
                        text: root._streamingMode ? qsTr("Total link transmit rate") : qsTr("Upload total-link rate")
                    }
                    QGCLabel { text: root.formatRate(controller.wireTransmitRateKbps) }

                    QGCLabel {
                        text: root._streamingMode ? qsTr("Total link receive rate") : qsTr("Download total-link rate")
                    }
                    QGCLabel { text: root.formatRate(controller.wireReceiveRateKbps) }

                    QGCLabel {
                        text: root._streamingMode ? qsTr("Payload bytes sent") : qsTr("Upload payload bytes")
                    }
                    QGCLabel { text: root.formatBytes(controller.transmittedPayloadBytes) }

                    QGCLabel {
                        text: root._streamingMode ? qsTr("Payload bytes received") : qsTr("Download payload bytes")
                    }
                    QGCLabel { text: root.formatBytes(controller.receivedPayloadBytes) }
                }

                GridLayout {
                    Layout.fillWidth: true
                    visible: root._streamingMode
                    columns: 2
                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.35

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
