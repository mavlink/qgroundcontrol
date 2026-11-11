import QtQuick
import QtQuick3D
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// A path known up front, such as the track of a log, drawn as a ribbon of constant screen width like
/// GeoMapFlightPath. The ribbon is built once from the path and costs nothing as the camera moves, so a track of
/// many thousand points stays cheap. It lies at the altitude of its coordinates: with none it is flat, which suits
/// a map that stays in 2D.
GeoMapItem {
    id: root

    property var path: []   ///< QGeoCoordinates
    property real lineWidth: ScreenTools.defaultFontPixelHeight / 5
    property color lineColor: "red"

    readonly property int pointCount: _geometry ? _geometry.pointCount : 0

    // The geometry lives inside the 3D delegate (created and destroyed with the scene)
    property var _geometry: null

    readonly property var _camera: scene ? scene.camera : null

    function _reloadPath() {
        if (_geometry) {
            _geometry.setPath(path)
        }
    }

    altitudeMode: GeoMapItem.Absolute
    crossfade3D: false
    coordinate: _geometry && _geometry.anchorCoordinate.isValid ? _geometry.anchorCoordinate : QtPositioning.coordinate()

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

                materials: CustomMaterial {
                    shadingMode: CustomMaterial.Unshaded
                    cullMode: Material.NoCulling
                    vertexShader: "shaders/flightpath.vert"
                    fragmentShader: "shaders/flightpath.frag"

                    property real lineWidth: root.lineWidth
                    property real screenFactor: root._camera ? root._camera.unitsPerPixelAtUnitDistance : 0
                    property color pathColor: root.lineColor
                    // Required by flightpath.frag; a track never highlights
                    property color highlightColor: root.lineColor
                    property real pathOpacity: 1.0
                    // In 2D the flattened terrain keeps a residual height above the flat path (see flightpath.vert)
                    property real depthPull: root.scene ? 0.5 * (1.0 - root.scene.terrainScale) : 0.0
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
