import QtQuick
import QtQuick3D

Node {
    id: root

    required property url meshSource
    property color baseColor: "gray"
    property real indexOfRefraction: 1.0
    property real metalness: 0.1

    Model {
        source: root.meshSource
        materials: PrincipledMaterial {
            baseColor: root.baseColor
            indexOfRefraction: root.indexOfRefraction
            metalness: root.metalness
            opacity: 1.0
            roughness: 0.1
            specularAmount: 1.0
        }
    }
}
