import QtQuick
import QtQuick3D
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// A path known up front, such as the track of a log, drawn as a ribbon of constant screen width
/// (FlightPathGeometry). The ribbon is built once from the path and costs nothing as the camera moves, so a track
/// of many thousand points stays cheap. It lies at the altitude of its coordinates: with none it is flat, which
/// suits a map that stays in 2D. GeoMapFlightPath is built on it.
GeoMapItem {
    id: root

    property var path: []   ///< QGeoCoordinates
    property real lineWidth: ScreenTools.defaultFontPixelHeight / 5
    property color lineColor: "red"
    property real altitudeBias: 0   ///< Meters that a path with altitudes is drawn above them

    readonly property int pointCount: _geometry ? _geometry.pointCount : 0

    // The geometry lives inside the 3D delegate (created and destroyed with the scene). GeoMapFlightPath adds the
    // points of a growing path to it.
    property var _geometry: null

    function _reloadPath() {
        if (_geometry) {
            _geometry.setPath(path)
        }
    }

    altitudeMode: GeoMapItem.Absolute
    crossfade3D: false
    // The geometry holds offsets from its anchor, so the bias on the anchor moves the whole path
    coordinate: {
        if (!_geometry || !_geometry.anchorCoordinate.isValid) {
            return QtPositioning.coordinate()
        }
        const anchor = _geometry.anchorCoordinate
        return QtPositioning.coordinate(anchor.latitude, anchor.longitude, anchor.altitude + altitudeBias)
    }

    onPathChanged: _reloadPath()

    delegate3D: Component {
        // z offsets are relative altitudes: they flatten with the terrain in 2D
        Node {
            scale: Qt.vector3d(1, 1, root.scene ? root.scene.terrainScale : 1)

            Model {
                geometry: FlightPathGeometry {
                    id: pathGeometry

                    scene: root.scene
                }

                materials: FlightPathMaterial {
                    scene: root.scene
                    ribbonWidth: root.lineWidth
                    pathColor: root.lineColor
                }

                Component.onCompleted: {
                    root._geometry = pathGeometry
                    root._reloadPath()
                }
                Component.onDestruction: root._geometry = null
            }
        }
    }
}
