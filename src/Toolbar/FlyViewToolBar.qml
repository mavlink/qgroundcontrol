import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.Toolbar

Item {
    id: control

    required property var guidedValueSlider

    width: parent.width
    height: Math.max(ScreenTools.toolbarHeight, 62)

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false

    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator()
    }

    Rectangle {
        anchors.fill: parent
        color: "#08141D"

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.00; color: "#073A63" }
            GradientStop { position: 0.14; color: "#075A99" }
            GradientStop { position: 0.30; color: "#076FC0" }
            GradientStop { position: 0.47; color: "#07568F" }
            GradientStop { position: 0.58; color: "#0A2636" }
            GradientStop { position: 0.65; color: "#171A18" }
            GradientStop { position: 0.72; color: "#4B2B0A" }
            GradientStop { position: 0.81; color: "#884A05" }
            GradientStop { position: 0.91; color: "#C66B05" }
            GradientStop { position: 1.00; color: "#EC8B09" }
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 2
        color: "#168BEB"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 9

        Item {
            id: atlasLogoButton
            objectName: "toolbar_atlasLogo"
            Layout.preferredWidth: 112
            Layout.fillHeight: true

            Rectangle {
                anchors.fill: parent
                anchors.margins: 5
                radius: 7
                color: "#254A6280"
                border.width: 1
                border.color: "#3E6B84"
            }

            Image {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.topMargin: 7
                anchors.bottomMargin: 7
                source: "qrc:/atlas/atlas_logo_white.svg"
                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: mainWindow.showToolSelectDialog()
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: 9

            QGCLabel {
                text: "ATLAS Video Control"
                color: "#F4F8FA"
                font.bold: true
                font.pointSize: ScreenTools.defaultFontPointSize * 1.28
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 24
                color: "#547184"
            }

            QGCLabel {
                text: "v0.12.2"
                color: "#9CC8E6"
                font.pointSize: ScreenTools.defaultFontPointSize * 1.00
            }

            Rectangle {
                visible: _activeVehicle
                Layout.preferredWidth: flightModeLabel.implicitWidth + 14
                Layout.preferredHeight: 28
                radius: 5
                color: "#182E3C"
                border.width: 1
                border.color: "#35566A"

                QGCLabel {
                    id: flightModeLabel
                    anchors.centerIn: parent
                    text: _activeVehicle ? _activeVehicle.flightMode : ""
                    color: "#D9EEF9"
                    font.bold: true
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }
        }

        Item { Layout.fillWidth: true }

        QGCButton {
            visible: _activeVehicle && _communicationLost
            text: qsTr("Disconnect")
            onClicked: _activeVehicle.closeVehicle()
        }

        Item {
            id: payloadRouteControl
            Layout.preferredWidth: 132
            Layout.preferredHeight: 52
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                anchors.fill: parent
                radius: 5
                color: routeMouse.containsMouse ? "#324052" : "#24081720"
                border.width: 1
                border.color: "#35566A"

                Rectangle {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: 12
                    height: 12
                    radius: 6
                    color: atlasPayload.connected ? "#39D353" : "#FFB020"
                }

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 31
                    anchors.right: parent.right
                    anchors.rightMargin: 18
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    QGCLabel {
                        width: parent.width
                        text: "PAYLOAD"
                        color: "#C7D8E2"
                        font.bold: true
                        font.pointSize: ScreenTools.smallFontPointSize * 0.86
                    }

                    QGCLabel {
                        width: parent.width
                        text: atlasPayload.modeDisplayName
                        color: "#F4F8FA"
                        font.bold: true
                        font.pointSize: ScreenTools.smallFontPointSize * 1.05
                        elide: Text.ElideRight
                    }
                }

                QGCLabel {
                    anchors.right: parent.right
                    anchors.rightMargin: 7
                    anchors.verticalCenter: parent.verticalCenter
                    text: "▾"
                    color: "#B8CCD8"
                    font.bold: true
                }

                MouseArea {
                    id: routeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: payloadRoutePopup.open()
                }
            }

            Popup {
                id: payloadRoutePopup
                parent: Overlay.overlay
                width: 500
                height: 420
                padding: 12
                modal: false
                focus: true
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                x: Math.max(10, Math.min(Overlay.overlay.width - width - 10,
                                         payloadRouteControl.mapToItem(Overlay.overlay, 0, 0).x + payloadRouteControl.width - width))
                y: Math.min(Overlay.overlay.height - height - 10,
                            payloadRouteControl.mapToItem(Overlay.overlay, 0, 0).y + payloadRouteControl.height + 6)

                background: Rectangle {
                    radius: 8
                    color: "#F20A1822"
                    border.width: 1
                    border.color: "#3C5D70"
                }

                contentItem: Column {
                    spacing: 10

                    Row {
                        width: parent.width
                        height: 34
                        spacing: 8

                        QGCLabel {
                            width: parent.width - 86
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Payload Route"
                            color: "#F4F8FA"
                            font.bold: true
                            font.pointSize: ScreenTools.defaultFontPointSize * 1.12
                        }

                        Rectangle {
                            width: 78
                            height: 30
                            radius: 5
                            color: atlasPayload.connected ? "#183A2B" : "#3B2B14"
                            border.width: 1
                            border.color: atlasPayload.connected ? "#2D8C4B" : "#9B6A18"
                            QGCLabel {
                                anchors.centerIn: parent
                                text: atlasPayload.connected ? "ONLINE" : "NO LINK"
                                color: atlasPayload.connected ? "#72E38D" : "#FFC55C"
                                font.bold: true
                                font.pointSize: ScreenTools.smallFontPointSize * 0.82
                            }
                        }
                    }

                    QGCLabel {
                        width: parent.width
                        text: "Manual only — ATLAS never changes route automatically."
                        color: "#9FB6C4"
                        font.pointSize: ScreenTools.smallFontPointSize * 0.9
                        wrapMode: Text.WordWrap
                    }

                    Row {
                        width: parent.width
                        height: 38
                        spacing: 8

                        Rectangle {
                            width: (parent.width - 8) / 2
                            height: parent.height
                            radius: 5
                            color: atlasPayload.directMode ? "#1C5E87" : "#142A37"
                            border.width: 1
                            border.color: atlasPayload.directMode ? "#49AEEA" : "#365466"
                            QGCLabel { anchors.centerIn: parent; text: "DIRECT A8"; color: "#F4F8FA"; font.bold: true }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: atlasPayload.setMode("DIRECT_A8")
                            }
                        }

                        Rectangle {
                            width: (parent.width - 8) / 2
                            height: parent.height
                            radius: 5
                            color: atlasPayload.jetsonMode ? "#6A3B08" : "#142A37"
                            border.width: 1
                            border.color: atlasPayload.jetsonMode ? "#E58A19" : "#365466"
                            QGCLabel { anchors.centerIn: parent; text: "JETSON"; color: "#F4F8FA"; font.bold: true }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: atlasPayload.setMode("JETSON")
                            }
                        }
                    }

                    Rectangle { width: parent.width; height: 1; color: "#304A5A" }

                    QGCLabel { text: "DIRECT A8 PROFILE"; color: "#75BDE9"; font.bold: true }

                    Row {
                        width: parent.width
                        spacing: 8
                        TextField {
                            id: directHostField
                            width: 190
                            height: 34
                            text: atlasPayload.directA8Host
                            placeholderText: "A8 IP"
                            color: "#F4F8FA"
                            background: Rectangle { radius: 4; color: "#132733"; border.color: "#3A5B6E" }
                        }
                        TextField {
                            id: directPortField
                            width: 92
                            height: 34
                            text: atlasPayload.directA8Port.toString()
                            placeholderText: "Port"
                            inputMethodHints: Qt.ImhDigitsOnly
                            color: "#F4F8FA"
                            background: Rectangle { radius: 4; color: "#132733"; border.color: "#3A5B6E" }
                        }
                        Rectangle {
                            width: parent.width - 190 - 92 - 16
                            height: 34
                            radius: 4
                            color: "#1A3B4E"
                            border.width: 1
                            border.color: "#3A6E89"
                            QGCLabel { anchors.centerIn: parent; text: "APPLY A8"; color: "#E8F5FC"; font.bold: true }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: atlasPayload.configureDirectA8(directHostField.text,
                                                                          parseInt(directPortField.text),
                                                                          directRtspField.text)
                            }
                        }
                    }

                    TextField {
                        id: directRtspField
                        width: parent.width
                        height: 34
                        text: atlasPayload.directRtspUrl
                        placeholderText: "Direct A8 RTSP URL"
                        color: "#F4F8FA"
                        background: Rectangle { radius: 4; color: "#132733"; border.color: "#3A5B6E" }
                    }

                    QGCLabel { text: "JETSON PROFILE"; color: "#F0A552"; font.bold: true }

                    Row {
                        width: parent.width
                        spacing: 8
                        TextField {
                            id: jetsonHostField
                            width: 190
                            height: 34
                            text: atlasPayload.jetsonHost
                            placeholderText: "Jetson IP"
                            color: "#F4F8FA"
                            background: Rectangle { radius: 4; color: "#132733"; border.color: "#3A5B6E" }
                        }
                        TextField {
                            id: jetsonPortField
                            width: 92
                            height: 34
                            text: atlasPayload.jetsonPort.toString()
                            placeholderText: "Port"
                            inputMethodHints: Qt.ImhDigitsOnly
                            color: "#F4F8FA"
                            background: Rectangle { radius: 4; color: "#132733"; border.color: "#3A5B6E" }
                        }
                        Rectangle {
                            width: parent.width - 190 - 92 - 16
                            height: 34
                            radius: 4
                            color: "#4B300F"
                            border.width: 1
                            border.color: "#8A5A21"
                            QGCLabel { anchors.centerIn: parent; text: "APPLY JETSON"; color: "#FFF0DB"; font.bold: true }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: atlasPayload.configureJetson(jetsonHostField.text,
                                                                        parseInt(jetsonPortField.text),
                                                                        jetsonRtspField.text)
                            }
                        }
                    }

                    TextField {
                        id: jetsonRtspField
                        width: parent.width
                        height: 34
                        text: atlasPayload.jetsonRtspUrl
                        placeholderText: "Jetson processed RTSP URL"
                        color: "#F4F8FA"
                        background: Rectangle { radius: 4; color: "#132733"; border.color: "#3A5B6E" }
                    }
                }
            }
        }

        FlyViewToolBarIndicators {
            id: flyViewIndicators
            Layout.preferredWidth: implicitWidth
            Layout.fillHeight: true
        }

        Rectangle {
            Layout.preferredWidth: 42
            Layout.preferredHeight: 42
            Layout.alignment: Qt.AlignVCenter
            radius: 7
            color: gearMouse.containsMouse ? "#28475A" : "#152A37"
            border.width: 1
            border.color: "#34566C"

            Image {
                anchors.centerIn: parent
                width: 24
                height: 24
                source: "qrc:/atlas/icons/settings.svg"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            MouseArea {
                id: gearMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: mainWindow.showToolSelectDialog()
            }
        }

        MainStatusIndicator {
            id: mainStatusIndicator
            visible: false
            Layout.preferredWidth: 0
            Layout.preferredHeight: 0
        }
    }

    ParameterDownloadProgress {
        anchors.fill: parent
    }
}
