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

    property real lightsBrightness: 0.3

    function resetCamera(){
        camNode.position = Qt.vector3d(0, 0, 0);
        camNode.eulerRotation = Qt.vector3d(90, 0, 0);

        cameraPerspectiveThree.position = Qt.vector3d(0, 0, 0);
        cameraPerspectiveThree.eulerRotation = Qt.vector3d(0, 0, 0);

        cameraPerspectiveTwo.position = Qt.vector3d(_zoom * Math.sin(_tilt) * Math.cos(_pan),
                                                    _zoom * Math.cos(_tilt),
                                                    _zoom * Math.sin(_tilt) * Math.sin(_pan));
        cameraPerspectiveTwo.eulerRotation = Qt.vector3d(0, 0, 0);

        cameraPerspectiveOne.position = Qt.vector3d(0, 0, 0);
        cameraPerspectiveOne.eulerRotation = Qt.vector3d(-90, 0, 0);
    }


    DirectionalLight {
        eulerRotation.x: 180
        brightness: lightsBrightness
    }

    DirectionalLight {
        eulerRotation.x: 0
        brightness: 0.6
    }

    DirectionalLight {
        eulerRotation.x: 90
        brightness: lightsBrightness
    }

    DirectionalLight {
        eulerRotation.x: 270
        brightness: lightsBrightness
    }

    DirectionalLight {
        eulerRotation.y: 90
        brightness: lightsBrightness
    }

    DirectionalLight {
        eulerRotation.y: -90
        brightness: lightsBrightness
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
                    clipFar: 100000

                    eulerRotation{
                        x: -90
                    }

                }
            }
        }
    }
}
