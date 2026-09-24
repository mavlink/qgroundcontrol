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
    readonly property bool _rtkConnected: _receiver.facts.connected.value
    readonly property var _activePresentation: _receiver.capabilitiesForManufacturer(_receiver.activeManufacturer)
    readonly property bool _averagingConnected: _receiver.activeBaseMode === BaseModeDefinition.BaseReceiverAveraging
    readonly property bool _surveyConnected: _receiver.activeBaseMode === BaseModeDefinition.BaseSurveyIn
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

            SettingsGroupLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                heading: qsTr("RTK GPS Status")
                visible: root._rtkConnected || root._receiver.hasReceiver || root._receiver.reconnecting
                         || !root.activeVehicle

                QGCLabel {
                    objectName: "rtkReceiverStatus"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: 0
                    wrapMode: Text.Wrap
                    text: !root._rtkConnected
                          ? (root._receiver.hasReceiver ? qsTr("Connecting to receiver...")
                             : root._receiver.reconnecting ? qsTr("Receiver connection lost. Reconnecting...")
                             : qsTr("No RTK receiver connected. Expand for settings."))
                          : root._activePresentation.passive ? qsTr("Passive RTCM/NMEA input connected")
                          : root._averagingConnected ? qsTr("Receiver-managed averaging — no accuracy guarantee")
                          : root._receiver.activeBaseMode === BaseModeDefinition.BaseFixed ? qsTr("Fixed base position")
                          : root._receiver.facts.active.value ? qsTr("Survey-in Active") : qsTr("Receiver connected")
                }
                LabelledLabel {
                    objectName: "rtkReceiverIdentity"
                    visible: root._rtkConnected && root._receiver.receiverIdentity.length > 0
                    label: qsTr("Receiver")
                    labelText: root._receiver.receiverIdentity
                }
                LabelledLabel {
                    objectName: "rtkReceiverEndpoint"
                    visible: root._receiver.hasReceiver && root._receiver.activeEndpoint.length > 0
                    label: qsTr("Connection")
                    labelText: root._receiver.activeEndpoint
                }
                LabelledLabel {
                    objectName: "rtkFixType"
                    visible: root._rtkConnected
                    label: qsTr("Receiver Fix")
                    labelText: root._receiver.facts.fixType.rawValue === 0
                               ? root.na : root._receiver.facts.fixType.enumStringValue
                }
                LabelledLabel {
                    objectName: "rtkSatellitesInView"
                    visible: root._rtkConnected
                    label: qsTr("Satellites in View")
                    labelText: root._receiver.facts.numSatellites.rawValue < 0
                               ? root.na : root._receiver.facts.numSatellites.valueString
                }
                LabelledLabel {
                    objectName: "rtkSatellitesUsed"
                    visible: root._rtkConnected
                    label: qsTr("Satellites Used")
                    labelText: root._receiver.facts.numSatellitesUsed.rawValue < 0
                               ? root.na : root._receiver.facts.numSatellitesUsed.valueString
                }
                LabelledLabel {
                    objectName: "rtkJamming"
                    visible: root._rtkConnected && root._receiver.facts.jammingState.rawValue > 0
                    label: qsTr("Jamming")
                    labelText: root._receiver.facts.jammingState.enumStringValue
                }
                LabelledLabel {
                    objectName: "rtkSpoofing"
                    visible: root._rtkConnected && root._receiver.facts.spoofingState.rawValue > 0
                    label: qsTr("Spoofing")
                    labelText: root._receiver.facts.spoofingState.enumStringValue
                }
                LabelledLabel {
                    objectName: "rtkSurveyDuration"
                    label: root._activePresentation.acceptedObservationTime ? qsTr("Accepted observation time") : qsTr("Duration")
                    visible: root._rtkConnected && root._activePresentation.reportsSurveyDuration
                             && root._surveyConnected
                    //: %1 is Survey-In duration in seconds
                    labelText: qsTr("%1 s").arg(root._receiver.facts.currentDuration.value)
                }
                LabelledLabel {
                    objectName: "rtkSurveyAccuracy"
                    label: root._receiver.facts.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
                    labelText: root._receiver.facts.currentAccuracy.valueString + " " + root._receiver.facts.currentAccuracy.units
                    visible: root._rtkConnected && root._surveyConnected
                             && root._receiver.facts.currentAccuracy.value > 0
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
