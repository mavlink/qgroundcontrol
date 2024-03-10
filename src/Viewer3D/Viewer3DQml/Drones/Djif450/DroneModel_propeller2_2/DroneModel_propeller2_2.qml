import QtQuick
import QtQuick3D

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Node {
    property int flightMode: 0 // DisArmed:0, Armed:1, InAir:2
    readonly property int prop_animation_duration_flying: 300
    readonly property int prop_animation_duration_armed: 1000

    id: node
    position: _STL_BINARY_.pivot
    Node {
        id: _STL_BINARY_
        Model {
            id: model
            source: "node.mesh"
            materials: [
                PrincipledMaterial {
                    baseColor: "gray"
                    metalness: 0.1
                    roughness: 0.1
                    specularAmount: 1.0
                    indexOfRefraction: 1.0
                    opacity: 1.0
                }
            ]
        }
        pivot: Qt.vector3d(119.51, 402.66, 557.75)
        PropertyAnimation{
            id: prop_animation
            running: (flightMode)?(true):(false)
            target: _STL_BINARY_
            properties: "eulerRotation.y"
            from: 0
            to: 360
            duration: 1000
            loops: Animation.Infinite

        }
    }

    onFlightModeChanged: {
        switch(flightMode)
        {
        case 1:
            prop_animation.duration = prop_animation_duration_armed
            break
        case 2:
            prop_animation.duration = prop_animation_duration_flying
            break
        }
        if(flightMode)
            prop_animation.restart()
    }
}
