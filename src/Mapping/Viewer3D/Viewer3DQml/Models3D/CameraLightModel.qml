import QtQuick3D

Node {
    property alias cameraOne: cameraPerspectiveOne
    property alias orbitCenter: orbitCenterNode.position
    property real cameraHeading: 0
    property real cameraTilt: 0
    property real cameraZoom: 1500
    property real lightsBrightness: 0.5
    property real viewDistance: 50000

    // Two directional lights maximum: mobile GPUs with small uniform buffers
    // make Qt Quick 3D reduce the directional light limit to 2 and silently
    // drop the rest ("Too many directional lights in scene, maximum is 2").

    // Shadow-casting key light, shining straight down (scene is z-up)
    DirectionalLight {
        brightness: 0.6
        castsShadow: true
        csmBlendRatio: 0.1
        csmNumSplits: 3
        eulerRotation.x: 0
        lockShadowmapTexels: true
        pcfFactor: 2.0
        shadowBias: 5
        shadowFactor: 50
        shadowMapFar: viewDistance
        shadowMapQuality: Light.ShadowMapQualityHigh
        softShadowQuality: Light.PCF16
    }

    // Slanted up/side fill so vertical faces and undersides aren't black
    DirectionalLight {
        brightness: lightsBrightness
        eulerRotation.x: 225
        eulerRotation.y: 45
    }

    // Orbit camera rig: heading rotates around the scene up axis (z),
    // tilt rotates around the local x axis, camera sits cameraZoom away
    // looking back at the orbit center.
    Node {
        id: orbitCenterNode

        eulerRotation.z: cameraHeading

        Node {
            id: camNode

            eulerRotation.x: 90 + cameraTilt

            Node {
                id: cameraPerspectiveTwo

                position.y: cameraZoom

                PerspectiveCamera {
                    id: cameraPerspectiveOne

                    clipFar: viewDistance
                    clipNear: 25
                    frustumCullingEnabled: true

                    eulerRotation {
                        x: -90
                    }
                }
            }
        }
    }
}
