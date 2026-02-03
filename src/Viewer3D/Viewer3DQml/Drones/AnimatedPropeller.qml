import QtQuick
import QtQuick3D

Node {
    id: root

    required property url meshSource
    required property vector3d pivotPoint
    property int flightMode: 0
    property real rotationTarget: 360

    readonly property int _armedDuration: 1000
    readonly property int _flyingDuration: 300

    position: pivotPoint

    onFlightModeChanged: {
        switch (flightMode) {
        case 1:
            propAnimation.duration = _armedDuration;
            break;
        case 2:
            propAnimation.duration = _flyingDuration;
            break;
        }
        if (flightMode)
            propAnimation.restart();
    }

    Node {
        id: innerNode

        pivot: root.pivotPoint

        Model {
            source: root.meshSource
            materials: PrincipledMaterial {
                baseColor: "gray"
                indexOfRefraction: 1.0
                metalness: 0.1
                opacity: 1.0
                roughness: 0.1
                specularAmount: 1.0
            }
        }

        PropertyAnimation {
            id: propAnimation

            target: innerNode
            properties: "eulerRotation.y"
            from: 0
            to: root.rotationTarget
            duration: 1000
            loops: Animation.Infinite
            running: root.flightMode > 0
        }
    }
}
