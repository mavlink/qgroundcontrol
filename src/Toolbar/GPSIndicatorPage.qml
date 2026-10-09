pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

// Drawer of the GPS indicator: vehicle GPS, GNSS receiver, corrections, and GCS position.
ToolIndicatorPage {
    id: root
    showExpand: true

    readonly property GPSReceiver _receiver: QGroundControl.gpsManager.receiver
    readonly property NTRIPSettings _ntripSettings: QGroundControl.settingsManager.ntripSettings
    // Connecting to a configured caster is the common field action, so it is offered without opening settings.
    readonly property bool _ntripConfigured: root._ntripSettings.userVisible
                                             && root._ntripSettings.ntripServerHostAddress.rawValue !== ""
    readonly property VehicleGPSFactGroup _gps: root.activeVehicle ? root.activeVehicle.gps : null
    readonly property bool _vehicleGps: !!root._gps && root._gps.telemetryAvailable
    readonly property real _preferredStatusWidth: ScreenTools.defaultFontPixelWidth * 36
    readonly property real _preferredSettingsWidth: ScreenTools.defaultFontPixelWidth * 56
    readonly property real _sideBySideWidth: root._preferredStatusWidth + root._preferredSettingsWidth
                                             + root.spacing * 2 + 1
    property real availableWidth: root.Window.width > 0 ? root.Window.width - ScreenTools.defaultFontPixelHeight * 4
                                                        : root._sideBySideWidth
    readonly property bool _compact: root.availableWidth < root._sideBySideWidth
    // Settings stay usable, rather than collapsing, on windows narrower than the reserved margins.
    readonly property real _settingsWidth: Math.max(ScreenTools.defaultFontPixelWidth * 30,
        Math.min(root._preferredSettingsWidth,
                 root.availableWidth - (root._compact ? 0 : root._preferredStatusWidth) - root.spacing * 2 - 1))

    // A vehicle GPS value, labelled with its description, shown while the vehicle reports it.
    component MeasurementLabel: LabelledLabel {
        property Fact fact: null
        property bool nonNegative: false

        visible: !!fact && Number.isFinite(fact.value) && (!nonNegative || fact.value >= 0)
        label: fact ? fact.shortDescription : ""
        labelText: fact ? fact.valueString + (fact.units ? " " + fact.units : "") : ""
    }

    // A resilience state, shown while the receiver reports it.
    component StateLabel: LabelledLabel {
        required property bool reported
        required property Fact fact

        visible: reported
        labelText: reported ? fact.enumStringValue : ""
    }

    // Only states the receiver reports are listed.
    component ResilienceGroup: SettingsGroupLayout {
        id: group

        required property VehicleGPSFactGroup facts
        readonly property bool reported: !!facts
                                         && (facts.jammingReported || facts.spoofingReported
                                             || facts.authenticationReported)

        Layout.minimumWidth: 0
        visible: reported

        StateLabel {
            label: qsTr("Jamming")
            reported: group.facts?.jammingReported ?? false
            fact: group.facts?.jammingState ?? null
        }
        StateLabel {
            label: qsTr("Spoofing")
            reported: group.facts?.spoofingReported ?? false
            fact: group.facts?.spoofingState ?? null
        }
        StateLabel {
            label: qsTr("Authentication")
            reported: group.facts?.authenticationReported ?? false
            fact: group.facts?.authenticationState ?? null
        }
    }

    contentComponent: Component {
        ColumnLayout {
            // On narrow screens the expanded view replaces status instead of requiring horizontal scrolling.
            width: root.expanded && root._compact ? 0 : Math.min(root._preferredStatusWidth, root.availableWidth)
            visible: !root.expanded || !root._compact
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                Layout.minimumWidth: 0
                heading: qsTr("Vehicle GPS Status")
                visible: root._vehicleGps

                LabelledLabel {
                    label: qsTr("Satellites")
                    labelText: root._gps?.count.valueString ?? ""
                }
                LabelledLabel {
                    label: root._gps?.lock.shortDescription ?? ""
                    labelText: root._gps?.lock.enumStringValue ?? ""
                }
                MeasurementLabel {
                    objectName: "vehicleGpsHdop"
                    fact: root._gps?.hdop ?? null
                }
                MeasurementLabel {
                    objectName: "vehicleGpsVdop"
                    fact: root._gps?.vdop ?? null
                }
                MeasurementLabel {
                    objectName: "vehicleGpsHorizontalAccuracy"
                    fact: root._gps?.horizontalAccuracy ?? null
                    nonNegative: true
                }
                MeasurementLabel {
                    objectName: "vehicleGpsVerticalAccuracy"
                    fact: root._gps?.verticalAccuracy ?? null
                    nonNegative: true
                }
                MeasurementLabel {
                    objectName: "vehicleGpsRtkBaseline"
                    fact: root._gps?.rtkBaseline ?? null
                }
                MeasurementLabel {
                    objectName: "vehicleGpsRtkRate"
                    fact: root._gps?.rtkRate ?? null
                }
                MeasurementLabel {
                    objectName: "vehicleGpsRtkSatellites"
                    fact: root._gps?.rtkSatellites ?? null
                    nonNegative: true
                }
                MeasurementLabel {
                    fact: root._gps?.courseOverGround ?? null
                }
                LabelledLabel {
                    label: qsTr("GPS Error")
                    labelText: root._gps?.systemErrorText ?? ""
                    visible: (root._gps?.systemErrors.value ?? 0) > 0
                }
            }

            // Each receiver that reports is listed; the headings name them only when both do.
            ResilienceGroup {
                id: gps1Resilience
                objectName: "gps1Resilience"
                heading: gps2Resilience.reported ? qsTr("GPS 1 Resilience") : qsTr("GPS Resilience Status")
                facts: root._gps
            }

            ResilienceGroup {
                id: gps2Resilience
                objectName: "gps2Resilience"
                heading: gps1Resilience.reported ? qsTr("GPS 2 Resilience") : qsTr("GPS Resilience Status")
                facts: root.activeVehicle?.gps2 ?? null
            }

            GPSReceiverStatus {
                Layout.minimumWidth: 0
                showWhenDisconnected: !root._vehicleGps
                disconnectedText: qsTr("No GNSS receiver connected. Expand for settings.")
            }

            CorrectionsStatus {
                objectName: "gpsIndicatorCorrections"
                Layout.minimumWidth: 0
                showWhenInactive: false
            }

            NTRIPConnectionStatus {
                objectName: "gpsIndicatorNtrip"
                Layout.minimumWidth: 0
                heading: qsTr("NTRIP")
                visible: root._ntripConfigured
            }

            GcsPositionStatus {
                objectName: "gpsIndicatorGcsPosition"
                Layout.minimumWidth: 0
                sourceEditable: false
                showCoordinates: false
            }

            GPSNoteLabel {
                visible: root._receiver.errorMessage.length > 0
                text: root._receiver.errorMessage
            }
        }
    }

    // The indicator's loader sizes the panel from its implicit width. A layout would compute it from its contents, and
    // GPSNoteLabel's zero preferred width keeps the notes from adding any, so a plain item supplies the width.
    expandedComponent: Component {
        Item {
            implicitWidth: root._settingsWidth
            implicitHeight: settingsPanel.implicitHeight

            GPSReceiverSettings {
                id: settingsPanel
                objectName: "gpsReceiverSettings"
                width: parent.width
                // The status column shows the error unless the compact layout hides it.
                showErrorMessage: root._compact
            }
        }
    }
}
