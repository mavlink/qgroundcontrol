pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

// Drop-down of the GPS indicator: the status of the vehicle's GPS, of the receivers, of the corrections and of the
// ground station position, with the actions that are needed in flight. Its last group opens the settings pages that
// set them up.
ToolIndicatorPage {
    id: root

    readonly property NTRIPSettings _ntripSettings: QGroundControl.settingsManager.ntripSettings
    // Connecting to a configured caster is the common field action, so it is offered without opening settings.
    readonly property bool _ntripConfigured: root._ntripSettings.userVisible
                                             && QGroundControl.gpsManager.ntrip.canConnect
    readonly property VehicleGPSFactGroup _gps: root.activeVehicle ? root.activeVehicle.gps : null
    readonly property bool _vehicleGps: !!root._gps && root._gps.telemetryAvailable
    readonly property real _preferredStatusWidth: ScreenTools.defaultFontPixelWidth * 36
    property real availableWidth: root.Window.width > 0 ? root.Window.width - ScreenTools.defaultFontPixelHeight * 4
                                                        : root._preferredStatusWidth

    /// Shows a page of the application settings in place of the drop-down. @a page is the page's name in
    /// SettingsPages.json, which is not translated.
    function _showSettingsPage(page) {
        if (mainWindow.allowViewSwitch()) {
            mainWindow.closeIndicatorDrawer()
            mainWindow.showSettingsTool(page)
        }
    }

    // A vehicle GPS value, labelled with its description, shown while the vehicle reports it.
    component MeasurementLabel: LabelledLabel {
        property Fact fact: null
        property bool nonNegative: false

        visible: !!fact && Number.isFinite(fact.value) && (!nonNegative || fact.value >= 0)
        label: fact ? fact.shortDescription : ""
        labelText: fact ? fact.valueString + (fact.units ? " " + fact.units : "") : ""
        labelTextFormat: Text.PlainText
    }

    // A resilience state, shown while the receiver reports it.
    component StateLabel: LabelledLabel {
        required property bool reported
        required property Fact fact

        visible: reported
        labelText: reported ? fact.enumStringValue : ""
        labelTextFormat: Text.PlainText
    }

    // Only states the receiver reports are listed.
    component ResilienceGroup: QGCGroupBox {
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

    // The page's loader takes the view's size from its implicit size once the page's layout has sized the loader. A
    // layout computes its implicit width from its contents, so a plain item supplies the view's width.
    contentComponent: Component {
        Item {
            implicitWidth: Math.min(root._preferredStatusWidth, root.availableWidth)
            implicitHeight: statusColumn.implicitHeight

            ColumnLayout {
                id: statusColumn
                width: parent.width
                spacing: ScreenTools.defaultFontPixelHeight / 2

                QGCGroupBox {
                    objectName: "gpsIndicatorVehicle"
                    Layout.minimumWidth: 0
                    heading: qsTr("Vehicle GPS Status")
                    visible: root._vehicleGps

                    LabelledLabel {
                        label: qsTr("Satellites")
                        labelText: root._gps?.count.valueString ?? ""
                        labelTextFormat: Text.PlainText
                    }
                    // The name of an RTK fix is wider than a narrow drop-down has room for beside its label, so it
                    // wraps, as the receiver's fix does.
                    GPSWrappingRow {
                        objectName: "vehicleGpsLock"
                        label: root._gps?.lock.shortDescription ?? ""
                        labelText: root._gps?.lock.enumStringValue ?? ""
                        labelWrapMode: Text.Wrap
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
                        labelTextFormat: Text.PlainText
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

                // It has Connect, so beside a vehicle's GPS status it is shown once a receiver is set up, as the caster
                // is.
                GPSReceiverStatus {
                    id: receiverStatus

                    objectName: "gpsIndicatorReceiver"
                    Layout.minimumWidth: 0
                    showWhenDisconnected: !root._vehicleGps || receiverStatus.receiver.hasEndpoint
                                          || receiverStatus.receiver.errorMessage !== ""
                }

                // Shown while the position receiver is in use.
                GPSPositionReceiverStatus { objectName: "gpsIndicatorPositionReceiver" }

                CorrectionsStatus {
                    objectName: "gpsIndicatorCorrections"
                    Layout.minimumWidth: 0
                    showWhenInactive: false
                    showCoordinates: false
                    // The active vehicle's RTK baseline and correction rate are rows of its GPS status above.
                    rtkShownElsewhere: root._vehicleGps ? root.activeVehicle as Vehicle : null
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

                // What the status rows report is set up on these pages, which the drop-down has no room for.
                QGCGroupBox {
                    objectName: "gpsIndicatorSettings"
                    Layout.minimumWidth: 0

                    LabelledButton {
                        objectName: "gpsIndicatorReceiverSettings"
                        label: qsTr("GNSS Receiver")
                        buttonText: qsTr("Configure")
                        onClicked: root._showSettingsPage("GNSS Receiver")
                    }

                    LabelledButton {
                        objectName: "gpsIndicatorCorrectionsSettings"
                        label: qsTr("RTK Corrections")
                        buttonText: qsTr("Configure")
                        onClicked: root._showSettingsPage("RTK Corrections")
                    }
                }
            }
        }
    }
}
