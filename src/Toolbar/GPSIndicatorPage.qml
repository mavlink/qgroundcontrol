pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// Used with a connected vehicle and as the standalone receiver indicator.
ToolIndicatorPage {
    id: root
    showExpand: true

    property string na: qsTr("N/A", "No data to display")
    property string valueNA: qsTr("–.––", "No data to display")
    property var rtkSettings: QGroundControl.settingsManager.rtkSettings
    readonly property var _receiver: QGroundControl.gpsManager.gpsRtk
    readonly property bool _rtkConnected: QGroundControl.gpsRtk.connected.value
    readonly property var _activePresentation: _receiver.capabilitiesForManufacturer(_receiver.activeManufacturer)
    readonly property var _serialPortManager: QGroundControl.serialPortManager
    readonly property bool _averagingConnected: _receiver.activeBaseMode === BaseModeDefinition.BaseReceiverAveraging
    readonly property real _preferredStatusWidth: ScreenTools.defaultFontPixelWidth * 36
    readonly property real _preferredSettingsWidth: ScreenTools.defaultFontPixelWidth * 56
    property real availableWidth: drawer && drawer.parent
                                  ? drawer.parent.width - ScreenTools.defaultFontPixelHeight * 4
                                  : root.Window.window
                                    ? root.Window.window.width - ScreenTools.defaultFontPixelHeight * 4
                                    : _preferredStatusWidth + _preferredSettingsWidth + spacing * 2 + 1
    readonly property bool _compact: availableWidth < _preferredStatusWidth + _preferredSettingsWidth + spacing * 2 + 1
    readonly property real _settingsWidth: Math.max(0, Math.min(_preferredSettingsWidth,
        availableWidth - (_compact ? 0 : _preferredStatusWidth) - spacing * 2 - 1))
    property alias _allowPersistentChanges: connectionConsent.allowed
    property var _settingsPanel: null

    function connectSelectedReceiver() {
        return _settingsPanel ? _settingsPanel.connectSelectedReceiver() : false
    }

    function errorText() {
        if (!activeVehicle) {
            return qsTr("Disconnected")
        }
        switch (activeVehicle.gps.systemErrors.value) {
        case 1: return qsTr("Incoming correction")
        case 2: return qsTr("Configuration")
        case 4: return qsTr("Software")
        case 8: return qsTr("Antenna")
        case 16: return qsTr("Event congestion")
        case 32: return qsTr("CPU overload")
        case 64: return qsTr("Output congestion")
        default: return qsTr("Multiple errors")
        }
    }

    QtObject {
        id: connectionConsent
        property bool allowed: false
    }

    contentComponent: Component {
        ColumnLayout {
            // On narrow screens the expanded view replaces status instead of requiring horizontal scrolling.
            width: root.expanded && root._compact ? 0 : Math.min(root._preferredStatusWidth, root.availableWidth)
            visible: !root.expanded || !root._compact
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                heading: qsTr("Vehicle GPS Status")
                visible: root.activeVehicle

                LabelledLabel {
                    label: qsTr("Satellites")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.count.valueString : root.na
                }
                LabelledLabel {
                    label: qsTr("GPS Lock")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.lock.enumStringValue : root.na
                }
                LabelledLabel {
                    label: qsTr("HDOP")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.hdop.valueString : root.valueNA
                }
                LabelledLabel {
                    label: qsTr("VDOP")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.vdop.valueString : root.valueNA
                }
                LabelledLabel {
                    label: qsTr("Course Over Ground")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.courseOverGround.valueString : root.valueNA
                }
                LabelledLabel {
                    label: qsTr("GPS Error")
                    labelText: root.errorText()
                    visible: root.activeVehicle && root.activeVehicle.gps.systemErrors.value > 0
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                heading: qsTr("RTK GPS Status")
                visible: root._rtkConnected || root._receiver.hasReceiver || !root.activeVehicle

                QGCLabel {
                    objectName: "rtkReceiverStatus"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: 0
                    wrapMode: Text.Wrap
                    text: !root._rtkConnected
                          ? (root._receiver.hasReceiver ? qsTr("Connecting to receiver...")
                                                       : qsTr("No RTK receiver connected. Expand for settings."))
                          : root._activePresentation.passive ? qsTr("Passive RTCM/NMEA input connected")
                          : root._averagingConnected ? qsTr("Receiver-managed averaging — no accuracy guarantee")
                          : QGroundControl.gpsRtk.active.value ? qsTr("Survey-in Active") : qsTr("Receiver connected")
                }
                LabelledLabel {
                    objectName: "rtkSatellitesInView"
                    visible: root._rtkConnected
                    label: qsTr("Satellites in View")
                    labelText: QGroundControl.gpsRtk.numSatellites.rawValue < 0
                               ? root.na : QGroundControl.gpsRtk.numSatellites.valueString
                }
                LabelledLabel {
                    objectName: "rtkSatellitesUsed"
                    visible: root._rtkConnected
                    label: qsTr("Satellites Used")
                    labelText: QGroundControl.gpsRtk.numSatellitesUsed.rawValue < 0
                               ? root.na : QGroundControl.gpsRtk.numSatellitesUsed.valueString
                }
                LabelledLabel {
                    label: root._activePresentation.acceptedObservationTime ? qsTr("Accepted observation time") : qsTr("Duration")
                    visible: root._rtkConnected && root._activePresentation.reportsSurveyDuration
                             && !root._averagingConnected
                    //: %1 is Survey-In duration in seconds
                    labelText: qsTr("%1 s").arg(QGroundControl.gpsRtk.currentDuration.value)
                }
                LabelledLabel {
                    label: QGroundControl.gpsRtk.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
                    labelText: QGroundControl.gpsRtk.currentAccuracy.valueString + " " + QGroundControl.gpsRtk.currentAccuracy.units
                    visible: root._rtkConnected && !root._activePresentation.passive && !root._averagingConnected
                             && QGroundControl.gpsRtk.currentAccuracy.value > 0
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: 0
                wrapMode: Text.Wrap
                visible: root._receiver.errorMessage.length > 0
                text: root._receiver.errorMessage
                textFormat: Text.PlainText
            }
        }
    }

    expandedComponent: Component {
        ColumnLayout {
            width: root._settingsWidth
            spacing: ScreenTools.defaultFontPixelHeight / 2

            GPSReceiverSettings {
                id: settingsPanel
                objectName: "gpsReceiverSettings"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                receiver: root._receiver
                settings: root.rtkSettings
                baseFacts: QGroundControl.gpsRtk
                autoConnectFact: QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS
                serialPorts: root._serialPortManager ? root._serialPortManager.serialPorts : []
                serialBaudRates: root._serialPortManager ? root._serialPortManager.serialBaudRates : []
                consent: connectionConsent
                Component.onCompleted: root._settingsPanel = settingsPanel
                Component.onDestruction: root._settingsPanel = null
            }
            QGCLabel {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: 0
                wrapMode: Text.Wrap
                visible: root._compact && root._receiver.errorMessage.length > 0
                text: root._receiver.errorMessage
                textFormat: Text.PlainText
            }
        }
    }
}
