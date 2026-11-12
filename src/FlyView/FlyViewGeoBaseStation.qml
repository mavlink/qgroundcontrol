pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// The GeoMap-engine counterpart of FlyViewBaseStation: the base station of the correction stream vehicles receive,
/// a line on the ground from it to the active vehicle, and on the line the RTK baseline length that vehicle reports.
Item {
    id: root

    property GeoScene scene
    property SurfacePatchModel surfaceModel
    /// Whether the map shows satellite imagery, which decides the colours of the baseline's text.
    property bool isSatelliteMap: false
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
    readonly property bool _lineProjected: root._lineShown && linePath.projected && linePath.screenPoints.length > 1

    QGCMapPalette { id: mapPal; lightColors: root.isSatelliteMap }

    GeoMapProjectedPath {
        id: linePath

        objectName: "geoBaseStationLine"
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinates: root._lineShown ? [root._basePosition, root._vehicleCoordinate] : []
    }

    Shape {
        visible: root._lineProjected

        ShapePath {
            strokeColor: QGroundControl.globalPalette.mapIndicator
            strokeWidth: 2
            fillColor: "transparent"

            PathPolyline { path: linePath.screenPoints }
        }
    }

    GeoMapPin {
        objectName: "geoBaseStationMarker"
        scene: root.scene
        surfaceModel: root.surfaceModel
        visible: root._baseKnown
        coordinate: root._basePosition
        label: qsTr("B", "RTK base station map item label")
    }

    QGCLabel {
        objectName: "geoBaseStationBaseline"
        visible: root._lineProjected && root._baselineReported
        // Halfway along the line.
        x: root._lineProjected ? (linePath.screenPoints[0].x + linePath.screenPoints[1].x - width) / 2 : 0
        y: root._lineProjected ? (linePath.screenPoints[0].y + linePath.screenPoints[1].y - height) / 2 : 0
        text: root._baselineReported ? root._baseline.valueString + " " + root._baseline.units : ""
        textFormat: Text.PlainText
        color: mapPal.text
        style: Text.Outline
        styleColor: mapPal.textOutline
    }
}
