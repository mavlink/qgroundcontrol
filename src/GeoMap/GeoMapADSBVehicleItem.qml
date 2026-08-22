/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick3D
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// ADS-B traffic marker: the GeoMap counterpart of ADSBVehicleMapItem.qml.
/// The 2D screen-space icon crossfades into a flat quad lying on the map
/// plane in 3D (traffic reports heading only — no attitude — so no full 3D
/// model), anchored at the reported AMSL altitude with a circular drop
/// shadow on the terrain for vertical situational awareness. Targets
/// without an altitude clamp to the ground.
Item {
    id: root

    property var scene
    property var surfaceModel
    property var coordinate                 ///< QGeoCoordinate lat/lon (altitude passed separately)
    property double altitude: Number.NaN    ///< AMSL meters, NaN to clamp to ground
    property string callsign: ""
    property double heading: Number.NaN     ///< Degrees from north, NaN for none
    property bool alert: false              ///< Collision alert
    property real size: ScreenTools.defaultFontPixelHeight * 3
    property real homeTerrainBias: 0        ///< DEM-vs-AMSL bias at the active vehicle's home (see FlyViewGeoMap)

    readonly property bool _hasAltitude: !isNaN(altitude)
    readonly property var _camera: scene ? scene.camera : null
    readonly property string _iconSource: alert ? "/qmlimages/AlertAircraft.svg" : "/qmlimages/AwarenessAircraft.svg"

    // Constant apparent size: scene units per screen pixel at the camera's
    // look-at distance. The built-in #Rectangle mesh is 100 units across.
    readonly property real _quadScale: _camera
        ? size * _camera.distance * _camera.unitsPerPixelAtUnitDistance / 100
        : 1

    // Drop shadow on the terrain below the target, draped as a filled circle
    // so it follows slopes instead of clipping into them (a flat 3D quad at
    // the sampled ground height gets partially buried on sloped terrain).
    // Fades with the same terrainScale ramp as the icon crossfade.
    GeoMapCircle {
        readonly property real _terrainScale: root.scene ? root.scene.terrainScale : 0

        visible: iconItem.visible && root._hasAltitude && _terrainScale > 0
        opacity: _terrainScale
        scene: root.scene
        surfaceModel: root.surfaceModel
        center: root.coordinate && root.coordinate.isValid ? root.coordinate : QtPositioning.coordinate()
        radiusMeters: root._camera
            ? ScreenTools.defaultFontPixelHeight * root._camera.distance * root._camera.unitsPerPixelAtUnitDistance
            : 1
        strokeColor: "transparent"
        strokeWidth: 0
        fillColor: Qt.alpha("black", 0.35)
    }

    GeoMapItem {
        id: iconItem
        visible: root.visible && root.coordinate && root.coordinate.isValid
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: root._hasAltitude ? GeoMapItem.Absolute : GeoMapItem.ClampToGround
        coordinate: {
            if (!root.coordinate || !root.coordinate.isValid) {
                return QtPositioning.coordinate()
            }
            return root._hasAltitude
                ? QtPositioning.coordinate(root.coordinate.latitude, root.coordinate.longitude,
                                           root.altitude + root.homeTerrainBias)
                : root.coordinate
        }
        width: root.size
        height: root.size
        anchorPoint: Qt.point(width / 2, height / 2)

        Image {
            anchors.fill: parent
            source: root._iconSource
            mipmap: true
            sourceSize.width: root.size
            fillMode: Image.PreserveAspectFit
            opacity: iconItem.contentOpacity2D
            // Camera heading rotates map north on screen; the icon follows
            rotation: (isNaN(root.heading) ? 0 : root.heading) + (root._camera ? root._camera.heading : 0)
        }

        // Flat map-plane icon for 3D: crossfades in against the 2D icon
        delegate3D: Component {
            // Scene axes are x east, y north, z up; heading is CW from north
            // while positive z-rotation is CCW, hence the negation
            Node {
                eulerRotation.z: -(isNaN(root.heading) ? 0 : root.heading)
                scale: Qt.vector3d(root._quadScale, root._quadScale, 1)

                Model {
                    source: "#Rectangle"

                    materials: PrincipledMaterial {
                        lighting: PrincipledMaterial.NoLighting
                        alphaMode: PrincipledMaterial.Blend
                        cullMode: Material.NoCulling
                        baseColorMap: Texture {
                            sourceItem: Image {
                                source: root._iconSource
                                sourceSize: Qt.size(128, 128)
                                fillMode: Image.PreserveAspectFit
                            }
                        }
                    }
                }
            }
        }

        QGCLabel {
            anchors.top: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            font.bold: true
            visible: root._hasAltitude
            text: visible ? QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnitsString(root.altitude, 0) + "\n" + root.callsign : ""
        }
    }
}
