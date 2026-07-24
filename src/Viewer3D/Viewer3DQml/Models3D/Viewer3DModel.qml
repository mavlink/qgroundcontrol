import QtPositioning
import QtQuick3D

import QGroundControl
import QGroundControl.Controls

View3D {
    id: topView

    objectName: "viewer3DView"

    readonly property real _viewDistance: 50000
    readonly property var _gpsRef: QGCViewer3DManager.gpsRef
    readonly property bool _mapLoaded: QGCViewer3DManager.mapProvider ? QGCViewer3DManager.mapProvider.mapLoaded : false

    // Exposed so FlyView can position the scale bar alongside the other widgets
    readonly property Viewer3DCameraController cameraController: _cameraController

    camera: standAloneScene.cameraOne

    environment: SceneEnvironment {
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
        backgroundMode: SceneEnvironment.Color
        clearColor: _skyColor

        fog: Fog {
            color: _skyColor
            depthCurve: 1.0
            depthEnabled: true
            depthFar: _viewDistance
            depthNear: 1000
            enabled: true
        }
    }

    QGCPalette { id: qgcPal }

    readonly property color _skyColor: qgcPal.window
    importScene: CameraLightModel {
        id: standAloneScene

        cameraHeading: cameraController.heading
        cameraTilt: cameraController.tilt
        cameraZoom: cameraController.distance
        orbitCenter: cameraController.orbitCenter
        viewDistance: _viewDistance
    }

    Viewer3DCameraController {
        id: _cameraController

        fieldOfView: standAloneScene.cameraOne.fieldOfView
        viewportSize: Qt.size(topView.width, topView.height)
    }

    function _resetCameraForContent() {
        if (_mapLoaded) {
            cameraController.reset();
        } else {
            // Without an OSM map there is no terrain; a top-down view shows
            // nothing useful. Look at the vehicle from the side, slightly above.
            cameraController.lookAt(Qt.vector3d(0, 0, 0), 0, 75, 400);
        }
    }

    Component.onCompleted: {
        vehicle3DLoader.active = true;
        mapGeometryLoader.active = true;
        _resetCameraForContent();
    }
    on_GpsRefChanged: {
        _resetCameraForContent();
    }
    on_MapLoadedChanged: {
        _resetCameraForContent();
    }

    Component {
        id: buildingsGeometryComponent

        Node {
            Model {
                id: cityMapModel

                scale: Qt.vector3d(10, 10, 10)
                visible: true

                geometry: CityMapGeometry {
                    id: cityMapGeometry

                    mapProvider: QGCViewer3DManager.mapProvider
                    modelName: "city_map"
                }
                materials: [
                    PrincipledMaterial {
                        baseColor: "gray"
                        indexOfRefraction: 4.0
                        metalness: 0.1
                        opacity: 1.0
                        roughness: 0.5
                        specularAmount: 1.0
                    }
                ]
            }

            Model {
                id: pointModel

                scale: Qt.vector3d(10, 10, 10)
                visible: true

                geometry: Viewer3DTerrainGeometry {
                    id: terrainGeometryManager

                    refCoordinate: _gpsRef
                }
                materials: CustomMaterial {
                    property TextureInput someTextureMap: TextureInput {
                        texture: Texture {
                            textureData: _terrainTextureManager
                        }
                    }

                    fragmentShader: "/qml/QGroundControl/Viewer3D/ShaderFragment/earthMaterial.frag"
                    vertexShader: "/qml/QGroundControl/Viewer3D/ShaderVertex/earthMaterial.vert"
                }
            }

            Viewer3DTerrainTexture {
                id: _terrainTextureManager

                readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                fallbackCenter: _activeVehicle ? _activeVehicle.homePosition : QtPositioning.coordinate()
                mapProvider: QGCViewer3DManager.mapProvider

                onTextureGeometryDoneChanged: {
                    if (textureGeometryDone === true) {
                        terrainGeometryManager.sectorCount = tileCount.width;
                        terrainGeometryManager.stackCount = tileCount.height;
                        terrainGeometryManager.roiMin = roiMinCoordinate;
                        terrainGeometryManager.roiMax = roiMaxCoordinate;
                        terrainGeometryManager.updateEarthData();
                    }
                }
            }
        }
    }

    Loader3D {
        id: mapGeometryLoader

        active: false
        sourceComponent: buildingsGeometryComponent
    }

    Component {
        id: vehicle3DComponent

        Repeater3D {
            model: QGroundControl.multiVehicleManager.vehicles

            delegate: Viewer3DVehicleItems {
                _backendQml: QGCViewer3DManager
                _camera: standAloneScene.cameraOne
                _planMasterController: masterController
                _vehicle: object

                PlanMasterController {
                    id: masterController

                    Component.onCompleted: startStaticActiveVehicle(object)
                }
            }
        }
    }

    Loader3D {
        id: vehicle3DLoader

        active: false
        sourceComponent: vehicle3DComponent
    }

    Viewer3DCameraControls {
        anchors.fill: parent
        controller: cameraController
    }

    TapHandler {
        function deselectAllWaypoints() {
            if (!vehicle3DLoader.item)
                return;
            for (var i = 0; i < vehicle3DLoader.item.count; i++) {
                vehicle3DLoader.item.objectAt(i).waypointInstancing.selectedIndex = -1;
            }
        }

        onTapped: function (eventPoint) {
            if (!vehicle3DLoader.item)
                return;

            var models = [];
            for (var i = 0; i < vehicle3DLoader.item.count; i++) {
                models.push(vehicle3DLoader.item.objectAt(i).waypointConeModel);
            }

            var results = topView.pickSubset(eventPoint.position.x, eventPoint.position.y, models);
            if (results.length === 0) {
                deselectAllWaypoints();
                return;
            }

            var result = results[0];
            for (var i = 0; i < vehicle3DLoader.item.count; i++) {
                var vehicleItem = vehicle3DLoader.item.objectAt(i);
                if (result.objectHit === vehicleItem.waypointConeModel) {
                    vehicleItem.waypointInstancing.selectedIndex = result.instanceIndex;
                    return;
                }
            }

            deselectAllWaypoints();
        }
    }
}
