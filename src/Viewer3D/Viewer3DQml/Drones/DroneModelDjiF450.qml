import QtPositioning
import QtQuick3D

import QGroundControl
import QGroundControl.Controls

Node {
    id: body

    function finiteOr(value, fallback) {
        return Number.isFinite(value) ? value : fallback;
    }

    property int _angleAnimationDuration: 100
    property int _poseAnimationDuration: 200
    property double altitudeBias: 0
    property var gpsRef: QtPositioning.coordinate(0, 0, 0)
    property double heading: vehicle ? finiteOr(vehicle.heading.value, 0) : 0
    property alias modelScale: innerNode.scale
    property double pitch: vehicle ? finiteOr(vehicle.pitch.value, 0) : 0
    property double pose_x: finiteOr(geo2Enu.localCoordinate.x, 0) * 10
    property double pose_y: finiteOr(geo2Enu.localCoordinate.y, 0) * 10
    property double pose_z: (finiteOr((vehicle ? vehicle.altitudeRelative.value : 0), 0) + altitudeBias) * 10
    property double roll: vehicle ? finiteOr(vehicle.roll.value, 0) : 0
    property var vehicle

    rotation: Quaternion.fromEulerAngles(Qt.vector3d(0, 0, (90 - body.heading)))

    Behavior on rotation {
        QuaternionAnimation {
            duration: _angleAnimationDuration
            easing.amplitude: 3.0
            easing.period: 2.0
            easing.type: Easing.Linear
        }
    }

    Viewer3DGeoCoordinateType {
        id: geo2Enu

        coordinate: vehicle ? vehicle.coordinate : QtPositioning.coordinate(0, 0, 0)
        gpsRef: body.gpsRef
    }

    Node {
        id: labelText

        eulerRotation.x: 90
        position.x: -10
        position.z: 30

        QGCLabel {
            color: "red"
            font.pixelSize: 25
            text: vehicle ? Number(vehicle.id) : ""
        }
    }

    position: Qt.vector3d(body.pose_x, body.pose_y, body.pose_z)

    Behavior on position {
        Vector3dAnimation {
            duration: _poseAnimationDuration
            easing.type: Easing.Linear
        }
    }

    Node {
        id: rollPitchRotationNode

        rotation: Quaternion.fromEulerAngles(Qt.vector3d(body.roll, -body.pitch, 0))

        Behavior on rotation {
            QuaternionAnimation {
                duration: _angleAnimationDuration
                easing.amplitude: 3.0
                easing.period: 2.0
                easing.type: Easing.Linear
            }
        }

        Node {
            eulerRotation {
                x: 90
            }

            Node {
                id: innerNode

                eulerRotation: Qt.vector3d(0, 90, 0)

                Node {
                    eulerRotation: Qt.vector3d(0, 45, 0)
                    position: Qt.vector3d(-640, -360, -155)

                    // Arms
                    DronePart { meshSource: "Djif450/DroneModel_arm_1/node.mesh"; baseColor: "white"; metalness: 0.2 }
                    DronePart { meshSource: "Djif450/DroneModel_arm_2/node.mesh"; baseColor: "red"; metalness: 0.2 }
                    DronePart { meshSource: "Djif450/DroneModel_arm_3/node.mesh"; baseColor: "red"; metalness: 0.2 }
                    DronePart { meshSource: "Djif450/DroneModel_arm_4/node.mesh"; baseColor: "red"; metalness: 0.2 }

                    // Motors
                    DronePart { meshSource: "Djif450/DroneModel_BLDC_1/node.mesh"; baseColor: "black"; indexOfRefraction: 2.0; metalness: 0.9 }
                    DronePart { meshSource: "Djif450/DroneModel_BLDC_2/node.mesh"; baseColor: "black"; indexOfRefraction: 2.0; metalness: 0.9 }
                    DronePart { meshSource: "Djif450/DroneModel_BLDC_3/node.mesh"; baseColor: "black"; indexOfRefraction: 2.0; metalness: 0.9 }
                    DronePart { meshSource: "Djif450/DroneModel_BLDC_4/node.mesh"; baseColor: "black"; indexOfRefraction: 2.0; metalness: 0.9 }

                    // Frame
                    DronePart { meshSource: "Djif450/DroneModel_Base_Top_1/node.mesh"; baseColor: "black"; indexOfRefraction: 2.0; metalness: 0.9 }
                    DronePart { meshSource: "Djif450/DroneModel_Base_bottom_1/node.mesh"; baseColor: "gray"; indexOfRefraction: 2.0; metalness: 0.9 }

                    // Propellers — counter-rotating pairs
                    AnimatedPropeller { meshSource: "Djif450/DroneModel_propeller22_1/node.mesh"; pivotPoint: Qt.vector3d(343.50, 404.07, 783.00); rotationTarget: -360; flightMode: vehicle ? (vehicle.armed + vehicle.flying) : 0 }
                    AnimatedPropeller { meshSource: "Djif450/DroneModel_propeller22_2/node.mesh"; pivotPoint: Qt.vector3d(343.42, 404.16, 333.06); rotationTarget: -360; flightMode: vehicle ? (vehicle.armed + vehicle.flying) : 0 }
                    AnimatedPropeller { meshSource: "Djif450/DroneModel_propeller2_2/node.mesh"; pivotPoint: Qt.vector3d(119.51, 402.66, 557.75); rotationTarget: 360; flightMode: vehicle ? (vehicle.armed + vehicle.flying) : 0 }
                    AnimatedPropeller { meshSource: "Djif450/DroneModel_propeller2_7/node.mesh"; pivotPoint: Qt.vector3d(567.97, 404.00, 558.26); rotationTarget: 360; flightMode: vehicle ? (vehicle.armed + vehicle.flying) : 0 }
                }
            }
        }
    }
}
