import QtQuick3D
import QtQuick
import QtPositioning

import QGroundControl.Viewer3D

import "Djif450/DroneModel_arm_1"
import "Djif450/DroneModel_arm_2"
import "Djif450/DroneModel_arm_3"
import "Djif450/DroneModel_arm_4"
import "Djif450/DroneModel_BLDC_1"
import "Djif450/DroneModel_BLDC_2"
import "Djif450/DroneModel_BLDC_3"
import "Djif450/DroneModel_BLDC_4"
import "Djif450/DroneModel_Base_Top_1"
import "Djif450/DroneModel_Base_bottom_1"
import "Djif450/DroneModel_propeller22_1"
import "Djif450/DroneModel_propeller22_2"
import "Djif450/DroneModel_propeller2_2"
import "Djif450/DroneModel_propeller2_7"

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Node{
    id: body

    property alias modelScale: innerNode.scale

    property var    vehicle
    property double altitudeBias: 0
    property var gpsRef: QtPositioning.coordinate(0, 0, 0)

    property double roll:        vehicle ? vehicle.roll.value : 0
    property double pitch:        vehicle ? vehicle.pitch.value : 0
    property double heading:        vehicle ? vehicle.heading.value : 0

    property double pose_y: (geo2Enu.localCoordinate.y)?(geo2Enu.localCoordinate.y * 10) : (0)
    property double pose_z: (vehicle.altitudeRelative.value)?((vehicle.altitudeRelative.value + altitudeBias) * 10 ): (0)
    property double pose_x: (geo2Enu.localCoordinate.x)? (geo2Enu.localCoordinate.x * 10) : (0)

    property int _poseAnimationDuration: 200
    property int _angleAnimationDuration: 100


    GeoCoordinateType{
        id: geo2Enu
        gpsRef: body.gpsRef
        coordinate: body.vehicle.coordinate
    }

    Node{
        id: lableText

        Node
        {
            Text {
                color: "red"
                text: Number(vehicle.id)
                font.pixelSize: 25
            }
        }

        eulerRotation{
            x:90
            y:0
            z:0
        }

        position{
            z: 30
            x:-10
        }

    }

    rotation: Quaternion.fromEulerAngles(Qt.vector3d(0, 0, (90 - body.heading)))

    Behavior on rotation {
        QuaternionAnimation{
            easing.type: Easing.Linear
            easing.amplitude: 3.0
            easing.period: 2.0
            duration: _angleAnimationDuration
        }
    }

    position{
        x: body.pose_x
        y: body.pose_y
        z: body.pose_z

        Behavior on  x {
            NumberAnimation {
                easing.type: Easing.Linear
                easing.amplitude: 3.0
                easing.period: 2.0
                duration: _poseAnimationDuration
            }
        }

        Behavior on  y {
            NumberAnimation {
                easing.type: Easing.Linear
                easing.amplitude: 3.0
                easing.period: 2.0
                duration: _poseAnimationDuration
            }
        }

        Behavior on  z {
            NumberAnimation {
                easing.type: Easing.Linear
                easing.amplitude: 3.0
                easing.period: 2.0
                duration: _poseAnimationDuration
            }
        }
    }


    Node
    {
        id: rollPitchRotationNode

        rotation: Quaternion.fromEulerAngles(Qt.vector3d(body.roll, -body.pitch, 0))

        Behavior on rotation {
            QuaternionAnimation{
                easing.type: Easing.Linear
                easing.amplitude: 3.0
                easing.period: 2.0
                duration: _angleAnimationDuration
            }
        }

        Node
        {
            eulerRotation{
                x:90
            }
            Node
            {
                id: innerNode
                eulerRotation: Qt.vector3d(0, 90, 0)
                Node{
                    position: Qt.vector3d(-640, -360, -155)
                    eulerRotation: Qt.vector3d(0, 45, 0)

                    DroneModel_arm_1 {

                    }

                    DroneModel_arm_2 {

                    }

                    DroneModel_arm_3 {

                    }

                    DroneModel_arm_4 {

                    }

                    DroneModel_BLDC_1{

                    }

                    DroneModel_BLDC_2{

                    }

                    DroneModel_BLDC_3{

                    }

                    DroneModel_BLDC_4{

                    }

                    DroneModel_Base_Top_1{

                    }

                    DroneModel_Base_bottom_1{

                    }

                    DroneModel_propeller22_1{
                        flightMode: vehicle.armed + vehicle.flying
                    }
                    DroneModel_propeller22_2{
                        flightMode: vehicle.armed + vehicle.flying
                    }

                    DroneModel_propeller2_2{
                        flightMode: vehicle.armed + vehicle.flying
                    }

                    DroneModel_propeller2_7{
                        flightMode: vehicle.armed + vehicle.flying
                    }
                }
            }
        }
    }
}
