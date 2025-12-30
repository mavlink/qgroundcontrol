/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl.Viewer3D

Node {
    id: node

    // Resources

    // Nodes:
    Node {
        id: _STL_BINARY_
        Model {
            id: model
            source: "node.mesh"
            materials: [
                PrincipledMaterial {
                    baseColor: "red"
                    metalness: 0.2
                    roughness: 0.1
                    specularAmount: 1.0
                    indexOfRefraction: 1.0
                    opacity: 1.0
                }
            ]
        }
    }


    // Animations:
}
