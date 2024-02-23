import QtQuick3D
import QtQuick
import QtQuick.Controls
import QtQuick.Window

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
    property var viewer3DManager: null
    property bool isViewer3DOpen: false
    property real rotationSpeed: 0.1
    property real movementSpeed: 1
    property real zoomSpeed: 0.3

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

        let dx_l = (newPose.x - lastPose.x) * movementSpeed
        let dy_l = (newPose.y - lastPose.y) * movementSpeed

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
        let dz_l = zoomValue * zoomSpeed;

        let _roll = standAloneScene.cameraOneRotation.x * (3.1415/180)
        let _pitch = standAloneScene.cameraOneRotation.y * (3.1415/180)

        let dx = -dz_l * Math.cos(_roll) * Math.sin(_pitch)
        let dy =  -dz_l * Math.sin(_roll)
        let dz = dz_l * Math.cos(_pitch) * Math.cos(_roll)

        standAloneScene.cameraTwoPosition.x -= dx
        standAloneScene.cameraTwoPosition.y += dy
        standAloneScene.cameraTwoPosition.z += dz
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

    Model {
        id: cityMapModel
        visible: true
        scale: Qt.vector3d(10, 10, 10)
        geometry: CityMapGeometry {
            id: cityMapGeometry
            modelName: "city_map"
            osmParser: (viewer3DManager)?(viewer3DManager.osmParser):(null)
        }

        materials: [ DefaultMaterial {
                diffuseColor: "gray"
            }
        ]
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
    }

    onViewer3DManagerChanged: {
        vehicle3DLoader.sourceComponent = vehicle3DComponent
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
