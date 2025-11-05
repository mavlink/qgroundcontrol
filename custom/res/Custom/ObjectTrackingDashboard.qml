import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls


AnalyzePage {
    id: root
    pageComponent: pageComponent
    pageDescription: qsTr("Al-Bayraq Object Tracking Dashboard - Monitor and control object tracking operations")

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            height: availableHeight
            spacing: ScreenTools.defaultFontPixelHeight * 0.5

            property var qgcPal: QGCPalette { colorGroupEnabled: true }

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

                        // Camera feed area
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#1a1a1a"
                            radius: 4
                            border.color: "#333333"
                            border.width: 1

                            // Active feed simulation
                            Item {
                                anchors.fill: parent
                                visible: QGroundControl.multiVehicleManager.activeVehicle

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

                                // Mock tracking box
                                Rectangle {
                                    x: parent.width * 0.25
                                    y: parent.height * 0.3
                                    width: parent.width * 0.35
                                    height: parent.height * 0.25
                                    color: "transparent"
                                    border.color: "#a50605"
                                    border.width: 3
                                    radius: 2

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

                            // No signal state
                            ColumnLayout {
                                anchors.centerIn: parent
                                visible: !QGroundControl.multiVehicleManager.activeVehicle
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
                                    text: "NO CAMERA SIGNAL"
                                    font.pointSize: ScreenTools.mediumFontPointSize
                                    font.bold: true
                                    color: "#dc3545"
                                    Layout.alignment: Qt.AlignHCenter
                                }

                                QGCLabel {
                                    text: "Connect vehicle to view camera feed"
                                    font.pointSize: ScreenTools.defaultFontPointSize
                                    color: "#6c757d"
                                    Layout.alignment: Qt.AlignHCenter
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
                                    text: QGroundControl.multiVehicleManager.activeVehicle ? "1920x1080 • 30fps • Tracking: ON" : "Offline"
                                    color: QGroundControl.multiVehicleManager.activeVehicle ? "#28a745" : "#6c757d"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                QGCLabel {
                                    text: QGroundControl.multiVehicleManager.activeVehicle ? "● REC" : ""
                                    color: "#dc3545"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.bold: true
                                    visible: QGroundControl.multiVehicleManager.activeVehicle
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
                                enabled: QGroundControl.multiVehicleManager.activeVehicle
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 18
                                backgroundColor: "#28a745"
                                textColor: "white"
                            }

                            QGCButton {
                                text: "Stop Tracking"
                                enabled: QGroundControl.multiVehicleManager.activeVehicle
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 18
                                backgroundColor: "#ffc107"
                                textColor: "black"
                            }

                            Item { Layout.fillWidth: true }

                            QGCButton {
                                text: "EMERGENCY STOP"
                                enabled: QGroundControl.multiVehicleManager.activeVehicle
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 20
                                fontWeight: Font.Bold
                                backgroundColor: "#dc3545"
                                textColor: "white"
                            }
                        }

                        // Quick settings
                        RowLayout {
                            Layout.fillWidth: true

                            QGCCheckBox {
                                text: "Auto Follow"
                                checked: true
                                enabled: QGroundControl.multiVehicleManager.activeVehicle
                                textColor: "white"
                            }

                            QGCCheckBox {
                                text: "Record Video"
                                checked: QGroundControl.multiVehicleManager.activeVehicle
                                enabled: QGroundControl.multiVehicleManager.activeVehicle
                                textColor: "white"
                            }

                            Item { Layout.fillWidth: true }

                            QGCLabel {
                                text: "Follow Distance:"
                                enabled: QGroundControl.multiVehicleManager.activeVehicle
                                color: "white"
                            }

                            QGCSlider {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                                from: 50
                                to: 500
                                value: 150
                                enabled: QGroundControl.multiVehicleManager.activeVehicle
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
                            QGCLabel { text: "Connection:" }
                            Rectangle {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                                color: QGroundControl.multiVehicleManager.activeVehicle ? "#28a745" : "#dc3545"
                                radius: 3
                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: QGroundControl.multiVehicleManager.activeVehicle ? "ONLINE" : "OFFLINE"
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
                                color: QGroundControl.multiVehicleManager.activeVehicle ? "#ffc107" : "#6c757d"
                                radius: 3
                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: QGroundControl.multiVehicleManager.activeVehicle ? "READY" : "IDLE"
                                    color: QGroundControl.multiVehicleManager.activeVehicle ? "black" : "white"
                                    font.bold: true
                                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                                }
                            }
                        }

                        RowLayout {
                            QGCLabel { text: "Signal:" }
                            Rectangle {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 4
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                                color: QGroundControl.multiVehicleManager.activeVehicle ? "#28a745" : "#6c757d"
                                radius: 3
                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: QGroundControl.multiVehicleManager.activeVehicle ? "85%" : "0%"
                                    color: "white"
                                    font.bold: true
                                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                                }
                            }
                        }

                        RowLayout {
                            QGCLabel { text: "Battery:" }
                            Rectangle {
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 4
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                                color: QGroundControl.multiVehicleManager.activeVehicle ? "#28a745" : "#6c757d"
                                radius: 3
                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: QGroundControl.multiVehicleManager.activeVehicle ? "87%" : "---"
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
