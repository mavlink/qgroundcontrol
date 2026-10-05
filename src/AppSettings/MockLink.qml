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
                id:             enableADSB
                text:           qsTr("Enable ADS-B vehicles")
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
                        entries.push({ name: qsTr("PX4 Vehicle"), linkName: "PX4 MultiRotor MockLink", firmware: MAVLinkEnums.MAV_AUTOPILOT_PX4, vehicle: MAVLinkEnums.MAV_TYPE_QUADROTOR })
                    }
                    if (QGroundControl.apmFirmwareSupported) {
                        entries.push({ name: qsTr("APM ArduCopter Vehicle"), linkName: "ArduCopter MockLink", firmware: MAVLinkEnums.MAV_AUTOPILOT_ARDUPILOTMEGA, vehicle: MAVLinkEnums.MAV_TYPE_QUADROTOR })
                        entries.push({ name: qsTr("APM ArduPlane Vehicle"), linkName: "ArduPlane MockLink", firmware: MAVLinkEnums.MAV_AUTOPILOT_ARDUPILOTMEGA, vehicle: MAVLinkEnums.MAV_TYPE_FIXED_WING })
                        entries.push({ name: qsTr("APM ArduSub Vehicle"), linkName: "ArduSub MockLink", firmware: MAVLinkEnums.MAV_AUTOPILOT_ARDUPILOTMEGA, vehicle: MAVLinkEnums.MAV_TYPE_SUBMARINE })
                        entries.push({ name: qsTr("APM ArduRover Vehicle"), linkName: "ArduRover MockLink", firmware: MAVLinkEnums.MAV_AUTOPILOT_ARDUPILOTMEGA, vehicle: MAVLinkEnums.MAV_TYPE_GROUND_ROVER })
                    }
                    entries.push({ name: qsTr("Generic Vehicle"), linkName: "Generic MockLink", firmware: MAVLinkEnums.MAV_AUTOPILOT_GENERIC, vehicle: MAVLinkEnums.MAV_TYPE_QUADROTOR })
                    return entries
                }
                readonly property var _vehicleNames: _vehicleEntries.map(entry => entry.name)
                // Entries are ordered supported firmwares first, Generic last resort
                readonly property var selectedEntry: _vehicleEntries[Math.max(currentIndex, 0)]
                readonly property bool apmSelected: selectedEntry.firmware === MAVLinkEnums.MAV_AUTOPILOT_ARDUPILOTMEGA
            }
            LabelledComboBox {
                id:                 homeLocationCombo
                label:              qsTr("Home Location")
                Layout.fillWidth:   true
                model: [
                    qsTr("PX4 SITL Default"),
                    qsTr("ArduPilot SITL Default"),
                    qsTr("Terrain Test")
                ]
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
                    const entry = vehicleTypeCombo.selectedEntry
                    QGroundControl.startMockLink({
                        name:                   entry.linkName,
                        firmware:               entry.firmware,
                        vehicle:                entry.vehicle,
                        sendStatus:             sendStatusText.checked,
                        enableCamera:           enableCamera.checked,
                        enableGimbal:           enableGimbal.checked,
                        enableProximity:        enableProximity.checked,
                        enableADSB:             enableADSB.checked,
                        apmStartFreshParams:    apmStartFreshParams.checked,
                        videoStreamType:        videoStreamTypeCombo.currentIndex,
                        homeLocation:           homeLocationCombo.currentIndex
                    })
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
