import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    color:          qgcPal.window
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.fill:   parent
        contentWidth:   column.width  + (_margins * 2)
        contentHeight:  column.height + (_margins * 2)
        clip:           true

        ColumnLayout {
            id:                 column
            anchors.margins:    _margins
            anchors.left:       parent.left
            anchors.top:        parent.top
            spacing:            ScreenTools.defaultFontPixelHeight / 4

            QGCCheckBox {
                id:             sendStatusText
                text:           qsTr("Send status text + voice")
            }
            QGCCheckBox {
                id:             enableCamera
                text:           qsTr("Enable camera")
            }
            QGCCheckBox {
                id:             enableGimbal
                text:           qsTr("Enable gimbal")
            }
            QGCCheckBox {
                id:             enableProximity
                text:           qsTr("Enable proximity sensors")
            }
            QGCCheckBox {
                id:             apmStartFreshParams
                text:           qsTr("Start with fresh firmware parameters (setup required)")
                visible:        vehicleTypeCombo.apmSelected

                onVisibleChanged: {
                    if (!visible) {
                        checked = false
                    }
                }
            }
            LabelledComboBox {
                id:                 vehicleTypeCombo
                label:              qsTr("Vehicle Type")
                Layout.fillWidth:   true
                model:              _vehicleNames

                readonly property var _vehicleEntries: {
                    let entries = []
                    if (QGroundControl.px4ProFirmwareSupported) {
                        entries.push({ key: "px4", name: qsTr("PX4 Vehicle") })
                    }
                    if (QGroundControl.apmFirmwareSupported) {
                        entries.push({ key: "apmCopter", name: qsTr("APM ArduCopter Vehicle") })
                        entries.push({ key: "apmPlane", name: qsTr("APM ArduPlane Vehicle") })
                        entries.push({ key: "apmSub", name: qsTr("APM ArduSub Vehicle") })
                        entries.push({ key: "apmRover", name: qsTr("APM ArduRover Vehicle") })
                    }
                    entries.push({ key: "generic", name: qsTr("Generic Vehicle") })
                    return entries
                }
                readonly property var _vehicleNames: _vehicleEntries.map(entry => entry.name)
                // Entries are ordered supported firmwares first, Generic last resort
                readonly property string selectedKey: currentIndex >= 0 ? _vehicleEntries[currentIndex].key : _vehicleEntries[0].key
                readonly property bool apmSelected: selectedKey.startsWith("apm")
            }
            LabelledComboBox {
                id:                 videoStreamTypeCombo
                label:              qsTr("Served Video Stream")
                Layout.fillWidth:   true
                visible:            enableCamera.checked
                model: [
                    qsTr("Disabled"),
                    qsTr("RTP/UDP H.264"),
                    qsTr("RTP/UDP H.265"),
                    qsTr("RTSP (H.264)"),
                    qsTr("MPEG-TS (UDP)"),
                    qsTr("MPEG-TS (TCP)")
                ]
            }
            QGCButton {
                text:               qsTr("Start MockLink")
                Layout.fillWidth:   true
                onClicked: {
                    switch (vehicleTypeCombo.selectedKey) {
                    case "px4":
                        QGroundControl.startPX4MockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, videoStreamTypeCombo.currentIndex)
                        break
                    case "apmCopter":
                        QGroundControl.startAPMArduCopterMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, apmStartFreshParams.checked, videoStreamTypeCombo.currentIndex)
                        break
                    case "apmPlane":
                        QGroundControl.startAPMArduPlaneMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, apmStartFreshParams.checked, videoStreamTypeCombo.currentIndex)
                        break
                    case "apmSub":
                        QGroundControl.startAPMArduSubMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, apmStartFreshParams.checked, videoStreamTypeCombo.currentIndex)
                        break
                    case "apmRover":
                        QGroundControl.startAPMArduRoverMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, apmStartFreshParams.checked, videoStreamTypeCombo.currentIndex)
                        break
                    default:
                        QGroundControl.startGenericMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, videoStreamTypeCombo.currentIndex)
                        break
                    }
                }
            }
            QGCButton {
                text:               qsTr("Stop One MockLink")
                Layout.fillWidth:   true
                onClicked:          QGroundControl.stopOneMockLink()
            }
        }
    }
}
