pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.AppSettings
import QGroundControl.Controls

// Used with a connected vehicle and as the standalone receiver indicator.
ToolIndicatorPage {
    id: root
    showExpand: true

    property string na: qsTr("N/A", "No data to display")
    property string valueNA: qsTr("–.––", "No data to display")
    property var rtkSettings: QGroundControl.settingsManager.rtkSettings
    readonly property var _receiver: QGroundControl.gpsManager.gpsRtk
    readonly property bool _vehicleGps: !!activeVehicle && !!activeVehicle.gps && activeVehicle.gps.telemetryAvailable
    readonly property real _preferredStatusWidth: ScreenTools.defaultFontPixelWidth * 36
    readonly property real _preferredSettingsWidth: ScreenTools.defaultFontPixelWidth * 56
    property real availableWidth: drawer && drawer.parent
                                  ? drawer.parent.width - ScreenTools.defaultFontPixelHeight * 4
                                  : root.Window.window
                                    ? root.Window.window.width - ScreenTools.defaultFontPixelHeight * 4
                                    : _preferredStatusWidth + _preferredSettingsWidth + spacing * 2 + 1
    readonly property bool _compact: availableWidth < _preferredStatusWidth + _preferredSettingsWidth + spacing * 2 + 1
    // Settings stay usable, rather than collapsing, on windows narrower than the reserved margins.
    readonly property real _settingsWidth: Math.max(ScreenTools.defaultFontPixelWidth * 30,
        Math.min(_preferredSettingsWidth, availableWidth - (_compact ? 0 : _preferredStatusWidth) - spacing * 2 - 1))
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
                visible: root._vehicleGps

                LabelledLabel {
                    label: qsTr("Satellites")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.count.valueString : root.na
                }
                LabelledLabel {
                    label: qsTr("GPS Lock")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.lock.enumStringValue : root.na
                }
                LabelledLabel {
                    objectName: "vehicleGpsHdop"
                    label: qsTr("HDOP")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.hdop.valueString : root.valueNA
                }
                LabelledLabel {
                    objectName: "vehicleGpsVdop"
                    label: qsTr("VDOP")
                    labelText: root.activeVehicle ? root.activeVehicle.gps.vdop.valueString : root.valueNA
                }
                LabelledLabel {
                    objectName: "vehicleGpsHorizontalAccuracy"
                    label: qsTr("Horizontal accuracy")
                    visible: root.activeVehicle
                             && Number.isFinite(root.activeVehicle.gps.horizontalAccuracy.value)
                             && root.activeVehicle.gps.horizontalAccuracy.value >= 0
                    labelText: root.activeVehicle
                               ? qsTr("%1 %2").arg(root.activeVehicle.gps.horizontalAccuracy.valueString)
                                             .arg(root.activeVehicle.gps.horizontalAccuracy.units)
                               : root.valueNA
                }
                LabelledLabel {
                    objectName: "vehicleGpsVerticalAccuracy"
                    label: qsTr("Vertical accuracy")
                    visible: root.activeVehicle
                             && Number.isFinite(root.activeVehicle.gps.verticalAccuracy.value)
                             && root.activeVehicle.gps.verticalAccuracy.value >= 0
                    labelText: root.activeVehicle
                               ? qsTr("%1 %2").arg(root.activeVehicle.gps.verticalAccuracy.valueString)
                                             .arg(root.activeVehicle.gps.verticalAccuracy.units)
                               : root.valueNA
                }
                LabelledLabel {
                    objectName: "vehicleGpsRtkBaseline"
                    label: qsTr("RTK baseline")
                    visible: root.activeVehicle && Number.isFinite(root.activeVehicle.gps.rtkBaseline.value)
                    labelText: root.activeVehicle
                               ? qsTr("%1 %2").arg(root.activeVehicle.gps.rtkBaseline.valueString)
                                             .arg(root.activeVehicle.gps.rtkBaseline.units)
                               : root.valueNA
                }
                LabelledLabel {
                    objectName: "vehicleGpsRtkRate"
                    label: qsTr("RTK correction rate")
                    visible: root.activeVehicle && Number.isFinite(root.activeVehicle.gps.rtkRate.value)
                    labelText: root.activeVehicle
                               ? qsTr("%1 %2").arg(root.activeVehicle.gps.rtkRate.valueString)
                                             .arg(root.activeVehicle.gps.rtkRate.units)
                               : root.valueNA
                }
                LabelledLabel {
                    objectName: "vehicleGpsRtkSatellites"
                    label: qsTr("RTK satellites")
                    visible: root.activeVehicle && root.activeVehicle.gps.rtkSatellites.value >= 0
                    labelText: root.activeVehicle ? root.activeVehicle.gps.rtkSatellites.valueString : root.na
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

            GPSReceiverStatus {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                receiver: root._receiver
                showWhenDisconnected: !root._vehicleGps
                disconnectedText: qsTr("No GNSS receiver connected. Expand for settings.")
            }

            GcsPositionStatus {
                objectName: "gpsIndicatorGcsPosition"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                sourceEditable: false
                showCoordinates: false
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
                consent: connectionConsent
                showErrorMessage: false
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
