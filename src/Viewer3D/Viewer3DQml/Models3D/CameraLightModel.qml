import QtQuick3D

Node {
    property real _pan: 0.001
    property real _tilt: 0.001
    property real _zoom: 1500
    property alias cameraOne: cameraPerspectiveOne
    property alias cameraOneRotation: cameraPerspectiveOne.eulerRotation
    property alias cameraTwoPosition: cameraPerspectiveTwo.position
    property real lightsBrightness: 0.3
    property real viewDistance: 50000

    function resetCamera() {
        camNode.position = Qt.vector3d(0, 0, 0);
        camNode.eulerRotation = Qt.vector3d(90, 0, 0);

        cameraPerspectiveTwo.position = Qt.vector3d(_zoom * Math.sin(_tilt) * Math.cos(_pan), _zoom * Math.cos(_tilt), _zoom * Math.sin(_tilt) * Math.sin(_pan));
        cameraPerspectiveTwo.eulerRotation = Qt.vector3d(0, 0, 0);

        cameraPerspectiveOne.position = Qt.vector3d(0, 0, 0);
        cameraPerspectiveOne.eulerRotation = Qt.vector3d(-90, 0, 0);
    }

    DirectionalLight {
        brightness: lightsBrightness
        eulerRotation.x: 180
    }

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

    DirectionalLight {
        brightness: lightsBrightness
        eulerRotation.x: 90
    }

    DirectionalLight {
        brightness: lightsBrightness
        eulerRotation.x: 270
    }

    DirectionalLight {
        brightness: lightsBrightness
        eulerRotation.y: 90
    }

    DirectionalLight {
        brightness: lightsBrightness
        eulerRotation.y: -90
    }

    Node {
        id: camNode

        eulerRotation {
            x: 90
        }

        Node {
            id: cameraPerspectiveTwo

            position {
                x: _zoom * Math.sin(_tilt) * Math.cos(_pan)
                y: _zoom * Math.cos(_tilt)
                z: _zoom * Math.sin(_tilt) * Math.sin(_pan)
            }

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
