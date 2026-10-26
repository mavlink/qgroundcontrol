import QtQuick
import QtQuick3D

import QGroundControl
import QGroundControl.Controls

import "PX4SensorsPose.js" as Pose

/// The vehicle in 3D on the side given. When the side changes it tips once, by the shortest turn, onto the new side. While
/// rotating it then turns about the vertical. When live it follows roll, pitch and heading instead.
View3D {
    id: control

    property string side:           "level"
    property bool   rotating:       false
    /// "multiRotor", "fixedWing", "vtol", "rover" or anything else for a flat board
    property string vehicleKind:    ""
    property bool   live:           false
    /// Degrees, used when live
    property real   roll:           0
    property real   pitch:          0
    property real   heading:        0

    // When live the vehicle stays put and the ground is below the furthest it can reach in any attitude
    readonly property real  _ground:    live ? Math.hypot(vehicleModel.halfSize[0], vehicleModel.halfSize[1], vehicleModel.halfSize[2]) + 4 :
                                               Math.max(vehicleModel.halfSize[0], vehicleModel.halfSize[1], vehicleModel.halfSize[2])

    property var    _from:  Pose.sidePose("level")
    property var    _to:    Pose.sidePose("level")
    property real   _swing: 1
    property var    _pose:  Pose.slerp(_from, _to, _swing)
    readonly property var _shownPose: live ? Pose.attitudePose(roll, pitch, heading) : _pose

    onSideChanged: {
        _from = _pose
        _swing = 0
        _to = Pose.tipTarget(_from, side)
        swingAnimation.restart()
    }

    Component.onCompleted: {
        _from = Pose.sidePose(side)
        _to = _from
    }

    QGCPalette { id: qgcPal }

    NumberAnimation {
        id:             swingAnimation
        target:         control
        property:       "_swing"
        from:           0
        to:             1
        duration:       900
        easing.type:    Easing.InOutCubic
    }

    NumberAnimation {
        target:     spin
        property:   "eulerRotation.y"
        from:       0
        to:         360
        duration:   4000
        loops:      Animation.Infinite
        running:    control.rotating && control._swing >= 1
        onRunningChanged: {
            if (!running) {
                spin.eulerRotation.y = 0
            }
        }
    }

    environment: SceneEnvironment {
        backgroundMode:         SceneEnvironment.Transparent
        antialiasingMode:       SceneEnvironment.MSAA
        antialiasingQuality:    SceneEnvironment.High
    }

    // Further back when live, to fit the wider disc and the vehicle in any attitude
    PerspectiveCamera {
        position:           control.live ? Qt.vector3d(0, 300, 859) : Qt.vector3d(0, 185, 560)
        eulerRotation.x:    -21
        fieldOfView:        32
    }

    DirectionalLight {
        eulerRotation:  Qt.vector3d(-45, -30, 0)
        brightness:     1.3
    }

    DirectionalLight {
        eulerRotation:  Qt.vector3d(30, 150, 0)
        brightness:     0.5
    }

    Node {
        y:               control.live ? 20 : 0
        eulerRotation.y: -30

        // When live the disc is wider, so the north arrow on its rim stays clear of the vehicle
        Model {
            source:     "#Cylinder"
            y:          -control._ground - 1
            scale:      control.live ? Qt.vector3d(3.4, 0.01, 3.4) : Qt.vector3d(2.4, 0.01, 2.4)
            materials:  PrincipledMaterial {
                baseColor:  qgcPal.borderSubtle
                roughness:  1
            }
        }

        // Points north, heading 0, which the live pose turns the nose (-z) towards. Sits on top of the disc, which has a
        // radius of 170 when live. The cone's base is at its origin, so its tip, 36 further on, is on the rim.
        Model {
            visible:        control.live
            source:         "#Cone"
            y:              -control._ground
            z:              -134
            eulerRotation.x: -90
            scale:          Qt.vector3d(0.28, 0.36, 0.01)
            materials:      PrincipledMaterial {
                baseColor:  qgcPal.colorRed
                lighting:   PrincipledMaterial.NoLighting
            }
        }

        Node {
            id: spin

            SensorsVehicleModel {
                id:         vehicleModel
                kind:       control.vehicleKind
                rotation:   Qt.quaternion(control._shownPose[0], control._shownPose[1], control._shownPose[2], control._shownPose[3])
                y:          control.live ? 0 : Pose.restHeight(control._shownPose, halfSize) - control._ground
            }
        }
    }
}
