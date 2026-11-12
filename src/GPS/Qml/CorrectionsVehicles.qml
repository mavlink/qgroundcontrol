pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// What reaches the vehicles and what they make of it: the corrections queued to their links, and for each vehicle
/// the fix its GPS reports, with its RTK baseline and correction rate where it reports them.
ColumnLayout {
    id: root

    required property GPSCorrectionManager corrections
    /// The vehicles listed. Each is named by its ID once there are several.
    property QmlObjectListModel vehicles: QGroundControl.multiVehicleManager.vehicles
    /// A vehicle whose RTK baseline and correction rate the view shows elsewhere, as the GPS indicator's drop-down
    /// does in the active vehicle's GPS status. The list shows its fix alone.
    property Vehicle rtkShownElsewhere: null

    readonly property int _vehicleCount: root.vehicles ? root.vehicles.count : 0
    readonly property bool _queued: root.corrections.vehicleBytesSubmitted > 0

    // A value of GPS_RTK, labelled with its description, shown while the vehicle reports it.
    component RtkLabel: LabelledLabel {
        required property Fact fact

        visible: !!fact && Number.isFinite(fact.value)
        label: fact ? fact.shortDescription : ""
        labelText: fact ? fact.valueString + (fact.units ? " " + fact.units : "") : ""
        labelTextFormat: Text.PlainText
    }

    spacing: ScreenTools.defaultFontPixelHeight / 4
    visible: root._queued || root._vehicleCount > 0

    // Admitted to the links, from whichever source the corrections came; not a count of what the vehicles received.
    LabelledLabel {
        objectName: "correctionsQueuedToVehicles"
        visible: root._queued
        label: qsTr("Queued to vehicles")
        labelText: QGroundControl.bigSizeToString(root.corrections.vehicleBytesSubmitted)
        labelTextFormat: Text.PlainText
    }

    Repeater {
        model: root.vehicles

        delegate: ColumnLayout {
            id: entry

            required property var object
            readonly property Vehicle vehicle: entry.object as Vehicle
            readonly property VehicleGPSFactGroup gps: entry.vehicle ? entry.vehicle.gps : null
            readonly property bool reportsGps: !!entry.gps && entry.gps.telemetryAvailable
            readonly property bool showsRtk: !!entry.gps && entry.vehicle !== root.rtkShownElsewhere

            Layout.fillWidth: true
            spacing: root.spacing
            visible: !!entry.vehicle

            // The fix type's text is long, so it wraps where the panel is narrow.
            GPSWrappingRow {
                objectName: "correctionsVehicleFix"
                Layout.fillWidth: true
                label: root._vehicleCount > 1
                       //: %1 is the ID of a vehicle; the row shows the GPS fix that vehicle reports
                       ? qsTr("Vehicle %1 fix").arg(entry.vehicle ? entry.vehicle.id : 0)
                       : qsTr("Vehicle fix")
                //: The vehicle sends no GPS status
                labelText: entry.reportsGps ? entry.gps.lock.enumStringValue : qsTr("Not reported")
                labelWrapMode: Text.Wrap
            }

            RtkLabel {
                objectName: "correctionsVehicleBaseline"
                fact: entry.showsRtk ? entry.gps.rtkBaseline : null
            }

            RtkLabel {
                objectName: "correctionsVehicleCorrectionRate"
                fact: entry.showsRtk ? entry.gps.rtkRate : null
            }
        }
    }
}
