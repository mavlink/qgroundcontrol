import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay


AnalyzePage {
    id: root
    pageComponent: pageComponent
    pageDescription: qsTr("Al-Bayraq Object Tracking Dashboard - Monitor and control object tracking operations")

    property bool trackingActive: false

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            height: availableHeight
            spacing: ScreenTools.defaultFontPixelHeight * 0.5

            property var qgcPal: QGCPalette { colorGroupEnabled: true }

            Component.onCompleted: {
                // Auto-start video when dashboard loads
                if (!QGroundControl.videoManager.streaming) {
                    QGroundControl.videoManager.startVideo()
                }
            }

            // Header with Al-Bayraq branding
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 4
                color: "#153247"
                radius: 6
                border.color: "#a50605"
                border.width: 2

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 1.5
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCColoredImage {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 2
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 2
                        source: "/custom/img/bay_logo_light.png"
                        fillMode: Image.PreserveAspectFit
                        color: "white"
                    }

                    QGCLabel {
                        text: "Al-Bayraq Object Tracking Dashboard"
                        font.pointSize: ScreenTools.largeFontPointSize
                        font.bold: true
                        color: "white"
                        Layout.fillWidth: true
                    }

                    // Connection status indicator
                    Rectangle {
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                        color: QGroundControl.multiVehicleManager.activeVehicle ? "#28a745" : "#dc3545"
                        radius: 4

                        QGCLabel {
                            anchors.centerIn: parent
                            text: QGroundControl.multiVehicleManager.activeVehicle ? "ONLINE" : "OFFLINE"
                            color: "white"
                            font.bold: true
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                    }
                }
            }

            // Camera Feed Area
            QGCGroupBox {
                Layout.fillWidth: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 22
                title: "Camera Feed & Tracking"

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 0.3
                    color: "#0a0a0a"
                    border.color: QGroundControl.multiVehicleManager.activeVehicle ? "#28a745" : "#6c757d"
                    border.width: 2
                    radius: 6

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                        spacing: ScreenTools.defaultFontPixelHeight * 0.3

                        // Camera feed area with real GStreamer video
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#1a1a1a"
                            radius: 4
                            border.color: "#333333"
                            border.width: 1

                            // GStreamer Video Background
                            QGCVideoBackground {
                                id: videoBackground
                                anchors.fill: parent
                                visible: QGroundControl.videoManager.decoding

                                function getWidth() {
                                    return QGroundControl.videoManager.videoSize.width > 0 ? QGroundControl.videoManager.videoSize.width : parent.width
                                }

                                function getHeight() {
                                    return QGroundControl.videoManager.videoSize.height > 0 ? QGroundControl.videoManager.videoSize.height : parent.height
                                }
                            }

                            // Video overlay with tracking elements
                            Item {
                                anchors.fill: parent
                                visible: QGroundControl.videoManager.decoding

                                // Crosshair overlay
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 2
                                    height: parent.height * 0.6
                                    color: "#28a745"
                                    opacity: 0.8
                                }
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: parent.width * 0.6
                                    height: 2
                                    color: "#28a745"
                                    opacity: 0.8
                                }

                                // Tracking box (shown when tracking is active)
                                Rectangle {
                                    id: trackingBox
                                    x: parent.width * 0.25
                                    y: parent.height * 0.3
                                    width: parent.width * 0.35
                                    height: parent.height * 0.25
                                    color: "transparent"
                                    border.color: "#a50605"
                                    border.width: 3
                                    radius: 2
                                    visible: trackingActive

                                    QGCLabel {
                                        anchors.top: parent.bottom
                                        anchors.left: parent.left
                                        anchors.topMargin: 4
                                        text: "TARGET LOCKED"
                                        color: "#a50605"
                                        font.bold: true
                                        font.pointSize: ScreenTools.smallFontPointSize
                                    }
                                }
                            }

                            // No video signal state
                            ColumnLayout {
                                anchors.centerIn: parent
                                visible: !QGroundControl.videoManager.decoding
                                spacing: ScreenTools.defaultFontPixelHeight * 0.5

                                QGCColoredImage {
                                    Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 4
                                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 4
                                    Layout.alignment: Qt.AlignHCenter
                                    source: "/qmlimages/camera.svg"
                                    fillMode: Image.PreserveAspectFit
                                    color: "#6c757d"
                                }

                                QGCLabel {
                                    text: "NO VIDEO SIGNAL"
                                    font.pointSize: ScreenTools.mediumFontPointSize
                                    font.bold: true
                                    color: "#dc3545"
                                    Layout.alignment: Qt.AlignHCenter
                                }

                                QGCButton {
                                    text: "Start Video"
                                    Layout.alignment: Qt.AlignHCenter
                                    onClicked: {
                                        QGroundControl.videoManager.startVideo()
                                    }
                                }
                            }
                        }

                        // Camera info bar
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                            color: "#2a2a2a"
                            radius: 3

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: ScreenTools.defaultFontPixelWidth * 0.3

                                QGCLabel {
                                    text: QGroundControl.videoManager.decoding ?
                                           `${QGroundControl.videoManager.videoSize.width}x${QGroundControl.videoManager.videoSize.height} • ${QGroundControl.videoManager.streaming ? "STREAMING" : "READY"} • Tracking: ${trackingActive ? "ON" : "OFF"}` :
                                           "No Video"
                                    color: QGroundControl.videoManager.decoding ? "#28a745" : "#6c757d"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                QGCLabel {
                                    text: QGroundControl.videoManager.recording ? "● REC" : ""
                                    color: "#dc3545"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.bold: true
                                    visible: QGroundControl.videoManager.recording
                                }
                            }
                        }
                    }
                }
            }

            // Controls Section
            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                // Main Controls
                QGCGroupBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 8
                    title: "Tracking Controls"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                        spacing: ScreenTools.defaultFontPixelHeight * 0.5

                        // Main control buttons
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: ScreenTools.defaultFontPixelWidth * 0.5

                            QGCButton {
                                text: "Start Tracking"
                                enabled: QGroundControl.videoManager.decoding && !trackingActive
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 18
                                backgroundColor: "#28a745"
                                textColor: "white"
                                onClicked: {
                                    trackingActive = true
                                    // Here you would start your actual tracking algorithm
                                }
                            }

                            QGCButton {
                                text: "Stop Tracking"
                                enabled: trackingActive
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 18
                                backgroundColor: "#ffc107"
                                textColor: "black"
                                onClicked: {
                                    trackingActive = false
                                    // Here you would stop your actual tracking algorithm
                                }
                            }

                            Item { Layout.fillWidth: true }

                            QGCButton {
                                text: "EMERGENCY STOP"
                                enabled: QGroundControl.videoManager.decoding
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 20
                                fontWeight: Font.Bold
                                backgroundColor: "#dc3545"
                                textColor: "white"
                                onClicked: {
                                    trackingActive = false
                                    QGroundControl.videoManager.stopVideo()
                                }
                            }
                        }

                        // Quick settings
                        RowLayout {
                            Layout.fillWidth: true

                            QGCCheckBox {
                                text: "Auto Follow"
                                checked: true
                                enabled: QGroundControl.videoManager.decoding
                                textColor: "white"
                            }

                            QGCCheckBox {
                                text: "Record Video"
                                checked: QGroundControl.videoManager.recording
                                enabled: QGroundControl.videoManager.decoding
                                textColor: "white"
                                onClicked: {
                                    if (checked) {
                                        QGroundControl.videoManager.startRecording()
                                    } else {
                                        QGroundControl.videoManager.stopRecording()
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            QGCLabel {
                                text: "Follow Distance:"
                                enabled: QGroundControl.videoManager.decoding
                                color: "white"
                            }

                            QGCSlider {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                                from: 50
                                to: 500
                                value: 150
                                enabled: QGroundControl.videoManager.decoding
                            }
                        }
                    }
                }

                // Status Panel
                QGCGroupBox {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 22
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 8
                    title: "System Status"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                        spacing: ScreenTools.defaultFontPixelHeight * 0.2

                        // Status indicators
                        RowLayout {
                            QGCLabel { text: "Video:" }
                            Rectangle {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                                color: QGroundControl.videoManager.decoding ? "#28a745" : "#dc3545"
                                radius: 3
                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: QGroundControl.videoManager.decoding ? "ONLINE" : "OFFLINE"
                                    color: "white"
                                    font.bold: true
                                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                                }
                            }
                        }

                        RowLayout {
                            QGCLabel { text: "Tracking:" }
                            Rectangle {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                                color: trackingActive ? "#ffc107" : "#6c757d"
                                radius: 3
                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: trackingActive ? "ACTIVE" : "IDLE"
                                    color: trackingActive ? "black" : "white"
                                    font.bold: true
                                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                                }
                            }
                        }

                        RowLayout {
                            QGCLabel { text: "Streaming:" }
                            Rectangle {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                                color: QGroundControl.videoManager.streaming ? "#28a745" : "#6c757d"
                                radius: 3
                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: QGroundControl.videoManager.streaming ? "ON" : "OFF"
                                    color: "white"
                                    font.bold: true
                                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                                }
                            }
                        }

                        RowLayout {
                            QGCLabel { text: "Recording:" }
                            Rectangle {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                                color: QGroundControl.videoManager.recording ? "#dc3545" : "#6c757d"
                                radius: 3
                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: QGroundControl.videoManager.recording ? "REC" : "OFF"
                                    color: "white"
                                    font.bold: true
                                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
