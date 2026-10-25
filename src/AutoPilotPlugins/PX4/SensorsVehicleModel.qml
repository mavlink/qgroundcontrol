import QtQuick
import QtQuick3D

/// A simple model of the vehicle for the calibration stage. The top is blue, the underside grey and the nose orange so the
/// side it is on can be told at a glance. The vehicle's nose points along -z and its right wing along +x.
Node {
    id: root

    /// "multiRotor", "fixedWing", "vtol", "rover" or anything else for a flat board
    property string kind: "board"

    /// Half the size of the box around the model along x, y and z
    readonly property var halfSize: {
        switch (kind) {
        case "multiRotor":
            return [112, 16, 112]
        case "fixedWing":
            return [110, 17, 105]
        case "vtol":
            return [110, 20, 105]
        case "rover":
            return [66, 20, 84]
        }
        return [60, 6, 40]
    }

    readonly property color _top:       "#3b82f6"
    readonly property color _under:     "#8a8f98"
    readonly property color _nose:      "#f59e0b"
    readonly property color _dark:      "#374151"
    readonly property color _spinning:  Qt.rgba(0.8, 0.85, 0.95, 0.45)

    component Part: Model {
        property color tint:    root._top
        property bool  glass:   false

        materials: PrincipledMaterial {
            baseColor:  tint
            roughness:  0.55
            alphaMode:  glass ? PrincipledMaterial.Blend : PrincipledMaterial.Opaque
        }
    }

    Node {
        visible: root.kind === "multiRotor"

        Part { source: "#Cube"; position: Qt.vector3d(0, 3, 0);  scale: Qt.vector3d(0.6, 0.1, 0.6) }
        Part { source: "#Cube"; position: Qt.vector3d(0, -5, 0); scale: Qt.vector3d(0.6, 0.06, 0.6); tint: root._under }
        Part { source: "#Cube"; position: Qt.vector3d(0, 3, -38); scale: Qt.vector3d(0.22, 0.1, 0.14); tint: root._nose }
        Part { source: "#Cube"; position: Qt.vector3d(0, 0, 0); eulerRotation.y: 45;  scale: Qt.vector3d(1.9, 0.06, 0.1); tint: root._under }
        Part { source: "#Cube"; position: Qt.vector3d(0, 0, 0); eulerRotation.y: -45; scale: Qt.vector3d(1.9, 0.06, 0.1); tint: root._under }

        Repeater3D {
            model: 4

            Node {
                required property int index
                position: Qt.vector3d((index & 1 ? 1 : -1) * 67, 4, (index & 2 ? 1 : -1) * 67)

                Part { source: "#Cylinder"; scale: Qt.vector3d(0.36, 0.16, 0.36); tint: root._dark }
                Part { source: "#Cylinder"; position: Qt.vector3d(0, 9, 0); scale: Qt.vector3d(0.9, 0.01, 0.9); tint: root._spinning; glass: true }
            }
        }
    }

    Node {
        visible: root.kind === "fixedWing" || root.kind === "vtol"

        Part { source: "#Cylinder"; eulerRotation.x: 90; scale: Qt.vector3d(0.28, 2, 0.28) }
        Part { source: "#Sphere"; position: Qt.vector3d(0, 0, -100); scale: Qt.vector3d(0.28, 0.28, 0.28); tint: root._nose }
        Part { source: "#Cube"; position: Qt.vector3d(0, 2, -10); scale: Qt.vector3d(2.2, 0.04, 0.4) }
        Part { source: "#Cube"; position: Qt.vector3d(0, -1, -10); scale: Qt.vector3d(2.2, 0.02, 0.4); tint: root._under }
        Part { source: "#Cube"; position: Qt.vector3d(0, 2, 90); scale: Qt.vector3d(0.8, 0.03, 0.22) }
        Part { source: "#Cube"; position: Qt.vector3d(0, 10, 92); scale: Qt.vector3d(0.03, 0.2, 0.26) }
    }

    Node {
        visible: root.kind === "vtol"

        Repeater3D {
            model: 2

            Node {
                required property int index
                position: Qt.vector3d((index === 0 ? -1 : 1) * 50, 0, 20)

                Part { source: "#Cube"; scale: Qt.vector3d(0.06, 0.05, 1.5); tint: root._under }
                Part { source: "#Cylinder"; position: Qt.vector3d(0, 6, -75); scale: Qt.vector3d(0.56, 0.01, 0.56); tint: root._spinning; glass: true }
                Part { source: "#Cylinder"; position: Qt.vector3d(0, 6, 75); scale: Qt.vector3d(0.56, 0.01, 0.56); tint: root._spinning; glass: true }
            }
        }
    }

    Node {
        visible: root.kind === "rover"
        y: 8

        Part { source: "#Cube"; position: Qt.vector3d(0, 6, 0); scale: Qt.vector3d(0.9, 0.1, 1.6) }
        Part { source: "#Cube"; position: Qt.vector3d(0, -4, 0); scale: Qt.vector3d(0.9, 0.12, 1.6); tint: root._under }
        Part { source: "#Cube"; position: Qt.vector3d(0, 2, -82); scale: Qt.vector3d(0.5, 0.12, 0.04); tint: root._nose }

        Repeater3D {
            model: 4

            Part {
                required property int index
                source:         "#Cylinder"
                eulerRotation.z: 90
                position:       Qt.vector3d((index & 1 ? 1 : -1) * 52, -6, (index & 2 ? 1 : -1) * 55)
                scale:          Qt.vector3d(0.44, 0.14, 0.44)
                tint:           root._dark
            }
        }
    }

    Node {
        visible: root.kind !== "multiRotor" && root.kind !== "fixedWing" && root.kind !== "vtol" && root.kind !== "rover"

        Part { source: "#Cube"; position: Qt.vector3d(0, 2, 0); scale: Qt.vector3d(1.2, 0.06, 0.8) }
        Part { source: "#Cube"; position: Qt.vector3d(0, -3, 0); scale: Qt.vector3d(1.2, 0.04, 0.8); tint: root._under }
        Part { source: "#Cube"; position: Qt.vector3d(0, 6, 4); scale: Qt.vector3d(0.3, 0.05, 0.3); tint: root._dark }
        Part { source: "#Cube"; position: Qt.vector3d(0, 3, -34); scale: Qt.vector3d(0.3, 0.04, 0.1); tint: root._nose }
    }
}
