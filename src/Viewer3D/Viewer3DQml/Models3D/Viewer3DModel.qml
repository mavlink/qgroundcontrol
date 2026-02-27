import QtQuick3D

import QGroundControl
import QGroundControl.Controls

View3D {
    id: topView

    readonly property real _viewDistance: 50000
    readonly property var _gpsRef: QGCViewer3DManager.gpsRef

    property real movementSpeed: 1
    property real rotationSpeed: 0.1
    property real zoomSpeed: 0.3

    function moveCamera(newPose: vector2d, lastPose: vector2d) {
        let _roll = standAloneScene.cameraOneRotation.x * (Math.PI / 180);
        let _pitch = standAloneScene.cameraOneRotation.y * (Math.PI / 180);

        let dx_l = (newPose.x - lastPose.x) * movementSpeed * movementSpeedAdjustment(2000.0, 4);
        let dy_l = (newPose.y - lastPose.y) * movementSpeed * movementSpeedAdjustment(2000.0, 4);

        //Note: Rotation Matrix is computed as: R = R(-_pitch) * R(_roll)
        // Then the corerxt tramslation is: d = R * [dx_l; dy_l; dz_l]

        let dx = dx_l * Math.cos(_pitch) - dy_l * Math.sin(_pitch) * Math.sin(_roll);
        let dy = dy_l * Math.cos(_roll);
        let dz = dx_l * Math.sin(_pitch) + dy_l * Math.cos(_pitch) * Math.sin(_roll);

        standAloneScene.cameraTwoPosition.x -= dx;
        standAloneScene.cameraTwoPosition.y += dy;
        standAloneScene.cameraTwoPosition.z += dz;
    }

    function movementSpeedAdjustment(adjustmentScale, maxValue) {
        let _adjustmentValue = standAloneScene.cameraTwoPosition.length() / adjustmentScale;
        return Math.min(Math.max(1, _adjustmentValue), maxValue);
    }

    function rotateCamera(newPose: vector2d, lastPose: vector2d) {
        let rotation_vec = Qt.vector2d(newPose.y - lastPose.y, newPose.x - lastPose.x);

        let dx_l = rotation_vec.x * rotationSpeed;
        let dy_l = rotation_vec.y * rotationSpeed;

        standAloneScene.cameraOneRotation.x += dx_l;
        standAloneScene.cameraOneRotation.y += dy_l;
    }

    function zoomCamera(zoomValue) {
        let dz_l = zoomValue * zoomSpeed * movementSpeedAdjustment(2000.0, 4);

        let _roll = standAloneScene.cameraOneRotation.x * (Math.PI / 180);
        let _pitch = standAloneScene.cameraOneRotation.y * (Math.PI / 180);

        let dx = -dz_l * Math.cos(_roll) * Math.sin(_pitch);
        let dy = -dz_l * Math.sin(_roll);
        let dz = dz_l * Math.cos(_pitch) * Math.cos(_roll);

        standAloneScene.cameraTwoPosition.x -= dx;
        standAloneScene.cameraTwoPosition.y += dy;
        standAloneScene.cameraTwoPosition.z += dz;
    }

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

        viewDistance: _viewDistance
    }

    Component.onCompleted: {
        vehicle3DLoader.active = true;
        mapGeometryLoader.active = true;
    }
    on_GpsRefChanged: {
        standAloneScene.resetCamera();
    }

    Viewer3DProgressBar {
        id: _terrainProgressBar

        progressText: qsTr("Downloading Imageries: ")
        width: ScreenTools.screenWidth * 0.2

        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            margins: ScreenTools.defaultFontPixelWidth
        }
    }

    Binding {
        property: "progressValue"
        target: _terrainProgressBar
        value: (mapGeometryLoader.active) ? (mapGeometryLoader.item.textureDownloadProgress) : (100)
        when: mapGeometryLoader.status == Loader.Ready
    }

    Component {
        id: buildingsGeometryComponent

        Node {
            property real textureDownloadProgress: _terrainTextureManager.textureDownloadProgress

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

    DragHandler {
        id: cameraMovementDragHandler

        property bool _isMoving: false
        property point _lastPose

        acceptedButtons: Qt.LeftButton
        acceptedModifiers: Qt.NoModifier
        target: null

        onActiveChanged: {
            if (active) { // When mouse is pressed
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
                _isMoving = true;
            } else { // When mouse is released
                _isMoving = false;
            }
        }
        onCentroidChanged: {
            if (_isMoving) {
                moveCamera(centroid.position, _lastPose);
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
            }
        }
    }

    DragHandler {
        id: cameraRotationDragHandler

        property bool _isRotating: false
        property point _lastPose

        acceptedButtons: Qt.RightButton
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        acceptedModifiers: Qt.NoModifier
        target: null

        onActiveChanged: {
            if (active) { // When mouse is pressed
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
                _isRotating = true;
            } else {// When mouse is released
                _isRotating = false;
            }
        }
        onCentroidChanged: {
            if (_isRotating) {
                rotateCamera(centroid.position, _lastPose);
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
            }
        }
    }

    PinchHandler {
        id: zoomRotationPinchHandler

        property bool _isRotating: false
        property point _lastPose
        property real _lastZoomValue

        target: null

        onActiveChanged: {
            if (active) {
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
                _lastZoomValue = 0;
                _isRotating = true;
            } else {
                _isRotating = false;
            }
        }
        onActiveScaleChanged: {
            let zoomValue = (activeScale > 1) ? (activeScale - 1) : (-((1 / activeScale) - 1));
            zoomCamera(-1000 * (zoomValue - _lastZoomValue));
            _lastZoomValue = zoomValue;
        }
        onCentroidChanged: {
            if (_isRotating) {
                rotateCamera(centroid.position, _lastPose);
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
            }
        }
    }

    WheelHandler {
        id: wheelHandler

        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        orientation: Qt.Vertical
        target: null

        onWheel: event => {
            zoomCamera(-event.angleDelta.y);
        }
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
