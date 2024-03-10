import QtQuick
import QtQuick3D

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Node {
    id: node

    Node {
        id: _STL_BINARY_
        Model {
            id: model
            source: "node.mesh"
            materials: [
                PrincipledMaterial {
                    baseColor: "black"
                    metalness: 0.9
                    roughness: 0.1
                    specularAmount: 1.0
                    indexOfRefraction: 2.0
                    opacity: 1.0
                }
            ]
        }
    }

}
