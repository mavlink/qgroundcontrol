pragma ComponentBehavior: Bound

import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.PlanView

/// The base station of the correction stream vehicles receive, once the stream has told where it is: a marker, a
/// line from it to the active vehicle, and on the line the RTK baseline length that vehicle reports.
MapItemGroup {
    id: root

    /// The map the items are on, whose imagery decides the colours of the baseline's text.
    required property var map
    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections
    /// The vehicle the line is drawn to; none without one.
    property Vehicle vehicle: QGroundControl.multiVehicleManager.activeVehicle

    readonly property geoCoordinate _basePosition: root.corrections.basePosition
    readonly property bool _baseKnown: root.corrections.state === GPSCorrectionManager.Fresh
                                       && root._basePosition.isValid
    readonly property geoCoordinate _vehicleCoordinate: root.vehicle ? root.vehicle.coordinate
                                                                     : QtPositioning.coordinate()
    readonly property bool _lineShown: root._baseKnown && root._vehicleCoordinate.isValid
    readonly property Fact _baseline: root.vehicle ? root.vehicle.gps.rtkBaseline : null
    readonly property bool _baselineReported: !!root._baseline && Number.isFinite(root._baseline.value)

    MapPolyline {
        objectName: "baseStationLine"
        visible: root._lineShown
        path: root._lineShown ? [root._basePosition, root._vehicleCoordinate] : []
        line.width: 2
        line.color: QGroundControl.globalPalette.mapIndicator
        z: QGroundControl.zOrderTrajectoryLines
    }

    MapQuickItem {
        objectName: "baseStationMarker"
        visible: root._baseKnown
        coordinate: root._basePosition
        anchorPoint.x: sourceItem.anchorPointX
        anchorPoint.y: sourceItem.anchorPointY
        z: QGroundControl.zOrderMapItems

        sourceItem: MissionItemIndexLabel {
            label: qsTr("B", "RTK base station map item label")
        }
    }

    MapQuickItem {
        id: baselineItem

        objectName: "baseStationBaseline"
        visible: root._lineShown && root._baselineReported
        // Halfway along the line.
        coordinate: baselineItem.visible
                    ? root._basePosition.atDistanceAndAzimuth(
                          root._basePosition.distanceTo(root._vehicleCoordinate) / 2,
                          root._basePosition.azimuthTo(root._vehicleCoordinate))
                    : QtPositioning.coordinate()
        anchorPoint.x: sourceItem.width / 2
        anchorPoint.y: sourceItem.height / 2
        z: QGroundControl.zOrderMapItems

        sourceItem: QGCMapLabel {
            map: root.map
            text: root._baselineReported ? root._baseline.valueString + " " + root._baseline.units : ""
            textFormat: Text.PlainText
        }
    }
}
