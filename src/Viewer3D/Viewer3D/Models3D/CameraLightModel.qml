import QtQuick3D
import QtQuick
import QtQuick.Controls

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Node {
    property alias cameraOneRotation: cameraPerspectiveOne.eulerRotation
    property alias cameraTwoPosition: cameraPerspectiveTwo.position
    property alias cameraOne: cameraPerspectiveOne

    property real _tilt: 0.001
    property real _pan: 0.001
    property real _zoom: 1500


    DirectionalLight {
        eulerRotation.x: 180
    }

    DirectionalLight {
        eulerRotation.x: 0
    }

    DirectionalLight {
        eulerRotation.x: 90
    }

    DirectionalLight {
        eulerRotation.x: 270
    }

    DirectionalLight {
        eulerRotation.y: 90
    }

    DirectionalLight {
        eulerRotation.y: -90
    }

    Node {
        id: camNode
        eulerRotation{
            x:90
        }
        Node {
            id: cameraPerspectiveThree
            Node {
                id: cameraPerspectiveTwo

                position{
                    x: _zoom * Math.sin(_tilt) * Math.cos(_pan)
                    z: _zoom * Math.sin(_tilt) * Math.sin(_pan)
                    y: _zoom * Math.cos(_tilt)
                }

                PerspectiveCamera {

                    id: cameraPerspectiveOne

                    eulerRotation{
                        x: -90
                    }

                }
            }
        }
    }
}
