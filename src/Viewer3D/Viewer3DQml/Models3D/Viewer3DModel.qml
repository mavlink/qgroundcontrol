import QtQuick3D
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtPositioning

import Viewer3D
import Viewer3D.Models3D.Drones
import Viewer3D.Models3D
import QGroundControl.Viewer3D

import QGroundControl
import QGroundControl.Controllers
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Vehicle

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

View3D {
    id: topView
    property var viewer3DManager:               null
    readonly property var _gpsRef:              (viewer3DManager)?(viewer3DManager.qmlBackend.gpsRef):(QtPositioning.coordinate(0, 0, 0))
    property bool isViewer3DOpen:               false
    property real rotationSpeed:                0.1
    property real movementSpeed:                1
    property real zoomSpeed:                    0.3
    property bool _viewer3DEnabled:             QGroundControl.settingsManager.viewer3DSettings.enabled.rawValue


    function movementSpeedAdjustment(adjustmentScale, maxValue){
        let _adjustmentValue = standAloneScene.cameraTwoPosition.length() / adjustmentScale;
        return Math.min(Math.max(1, _adjustmentValue), maxValue)
    }

    function rotateCamera(newPose: vector2d, lastPose: vector2d) {
        let rotation_vec = Qt.vector2d(newPose.y - lastPose.y, newPose.x - lastPose.x);

        let dx_l = rotation_vec.x * rotationSpeed
        let dy_l = rotation_vec.y * rotationSpeed

        standAloneScene.cameraOneRotation.x += dx_l
        standAloneScene.cameraOneRotation.y += dy_l
    }

    function moveCamera(newPose: vector2d, lastPose: vector2d) {
        let _roll = standAloneScene.cameraOneRotation.x * (3.1415/180)
        let _pitch = standAloneScene.cameraOneRotation.y * (3.1415/180)

        let dx_l = (newPose.x - lastPose.x) * movementSpeed * movementSpeedAdjustment(2000.0, 4)
        let dy_l = (newPose.y - lastPose.y) * movementSpeed * movementSpeedAdjustment(2000.0, 4)

        //Note: Rotation Matrix is computed as: R = R(-_pitch) * R(_roll)
        // Then the corerxt tramslation is: d = R * [dx_l; dy_l; dz_l]

        let dx = dx_l * Math.cos(_pitch) - dy_l * Math.sin(_pitch) * Math.sin(_roll)
        let dy =  dy_l * Math.cos(_roll)
        let dz = dx_l * Math.sin(_pitch) + dy_l * Math.cos(_pitch) * Math.sin(_roll)

        standAloneScene.cameraTwoPosition.x -= dx
        standAloneScene.cameraTwoPosition.y += dy
        standAloneScene.cameraTwoPosition.z += dz
    }

    function zoomCamera(zoomValue){
        let dz_l = zoomValue * zoomSpeed * movementSpeedAdjustment(2000.0, 4);

        let _roll = standAloneScene.cameraOneRotation.x * (3.1415/180)
        let _pitch = standAloneScene.cameraOneRotation.y * (3.1415/180)

        let dx = -dz_l * Math.cos(_roll) * Math.sin(_pitch)
        let dy =  -dz_l * Math.sin(_roll)
        let dz = dz_l * Math.cos(_pitch) * Math.cos(_roll)

        standAloneScene.cameraTwoPosition.x -= dx
        standAloneScene.cameraTwoPosition.y += dy
        standAloneScene.cameraTwoPosition.z += dz
    }

    on_Viewer3DEnabledChanged: {
        if(_viewer3DEnabled === false){
            mapGeometryLoader.active = false;
            vehicle3DLoader.active = false;
            viewer3DManager = null;
        }
    }

    on_GpsRefChanged:{
        if(_viewer3DEnabled){
            standAloneScene.resetCamera();
        }
    }

    camera: standAloneScene.cameraOne
    importScene: CameraLightModel{
        id: standAloneScene
    }

    //    renderMode: View3D.Inline

    environment: SceneEnvironment {
        clearColor: "#F9F9F9"
        backgroundMode: SceneEnvironment.Color
    }

    Viewer3DProgressBar{
        id: _terrainProgressBar
        anchors{
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            margins: ScreenTools.defaultFontPixelWidth
        }
        width:          ScreenTools.screenWidth * 0.2
        progressText: qsTr("Downloading Imageries: ")
    }

    Binding{
        target: _terrainProgressBar
        property: "progressValue"
        value: (mapGeometryLoader.active)?(mapGeometryLoader.item.textureDownloadProgress):(100)
        when: mapGeometryLoader.status == Loader.Ready
    }

    Component{
        id: buildingsGeometryComponent

        Node{
            property real textureDownloadProgress: _terrainTextureManager.textureDownloadProgress

            Model {
                id: cityMapModel
                visible: true
                scale: Qt.vector3d(10, 10, 10)
                geometry: CityMapGeometry {
                    id: cityMapGeometry
                    modelName: "city_map"
                    osmParser: (viewer3DManager)?(viewer3DManager.osmParser):(null)
                }

                materials: [
                    PrincipledMaterial {
                        baseColor: "gray"
                        metalness: 0.1
                        roughness: 0.5
                        specularAmount: 1.0
                        indexOfRefraction: 4.0
                        opacity: 1.0
                    }
                ]
            }

            Model {
                id: pointModel
                visible: true
                scale: Qt.vector3d(10, 10, 10)

                geometry: Viewer3DTerrainGeometry {
                    id: terrainGeometryManager
                    refCoordinate: _gpsRef
                }

                materials: CustomMaterial {
                    vertexShader: "/ShaderVertex/earthMaterial.vert"
                    fragmentShader: "/ShaderFragment/earthMaterial.frag"
                    property TextureInput someTextureMap: TextureInput {
                        texture: Texture {
                            textureData: _terrainTextureManager
                        }
                    }
                }
            }

            Viewer3DTerrainTexture {
                id: _terrainTextureManager
                osmParser: (viewer3DManager)?(viewer3DManager.osmParser):(null)

                onTextureGeometryDoneChanged: {
                    if(textureGeometryDone === true){
                        terrainGeometryManager.sectorCount = tileCount.width
                        terrainGeometryManager.stackCount = tileCount.height
                        terrainGeometryManager.roiMin = roiMinCoordinate;
                        terrainGeometryManager.roiMax = roiMaxCoordinate;
                        terrainGeometryManager.updateEarthData();
                    }
                }
            }
        }
    }

    Loader3D{
        id: mapGeometryLoader
        active: false
        sourceComponent: buildingsGeometryComponent
    }

    Component{
        id: vehicle3DComponent
        Repeater3D{
            model: QGroundControl.multiVehicleManager.vehicles

            delegate: Viewer3DVehicleItems{
                _vehicle: object
                _backendQml: (viewer3DManager)?(viewer3DManager.qmlBackend):(null)
                _planMasterController: masterController

                PlanMasterController {
                    id: masterController
                    Component.onCompleted: startStaticActiveVehicle(object)
                }
            }
        }
    }

    Loader3D{
        id: vehicle3DLoader
        active: false
        sourceComponent: vehicle3DComponent
    }

    onViewer3DManagerChanged: {
        if(viewer3DManager){
            vehicle3DLoader.active = true;
            mapGeometryLoader.active = true;
        }
    }

    DragHandler {
        property bool _isMoving: false
        property point _lastPose;

        id: cameraMovementDragHandler
        target: null
        acceptedModifiers: Qt.NoModifier
        acceptedButtons: Qt.LeftButton
        enabled: isViewer3DOpen

        onCentroidChanged: {
            if(_isMoving){
                moveCamera(centroid.position, _lastPose);
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
            }
        }

        onActiveChanged: {
            if(active){ // When mouse is pressed
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
                _isMoving = true
            }else{ // When mouse is released
                _isMoving = false
            }
        }
    }

    DragHandler {
        property bool _isRotating: false
        property point _lastPose;

        id: cameraRotationDragHandler
        target: null
        acceptedModifiers: Qt.NoModifier
        acceptedButtons: Qt.RightButton
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        enabled: isViewer3DOpen

        onCentroidChanged: {
            if(_isRotating){
                rotateCamera(centroid.position, _lastPose);
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
            }
        }

        onActiveChanged: {
            if(active){ // When mouse is pressed
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
                _isRotating = true
            }else{// When mouse is released
                _isRotating = false
            }
        }
    }

    PinchHandler {
        id: zoomRotationPinchHandler
        target: null

        property bool _isRotating: false
        property point _lastPose;
        property real _lastZoomValue;
        enabled: isViewer3DOpen

        onCentroidChanged: {
            if(_isRotating){
                rotateCamera(centroid.position, _lastPose);
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
            }
        }

        onActiveChanged: {
            if (active) {
                _lastPose = Qt.point(centroid.position.x, centroid.position.y);
                _lastZoomValue = 0
                _isRotating = true;
            } else {
                _isRotating = false;
            }
        }
        onActiveScaleChanged: {
            let zoomValue = (activeScale > 1)?(activeScale - 1):(-((1/activeScale) - 1))
            zoomCamera(- 1000 * (zoomValue - _lastZoomValue))
            _lastZoomValue = zoomValue
        }
    }

    WheelHandler {
        id: wheelHandler
        orientation: Qt.Vertical
        target: null
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        enabled: isViewer3DOpen

        onWheel: event => {
                     zoomCamera(-event.angleDelta.y)
                 }
    }
}
