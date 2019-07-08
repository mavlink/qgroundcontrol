import QtQuick 2.11
import QtQuick.Controls 2.4
import QtGraphicalEffects 1.0

//import QGroundControl.Controls.VirtualJoystick
import Auterion.Controls 1.0

Item {
    id: _root

    property alias mainColor: joystickCtrl.mainColor
    property alias secondaryColor: joystickCtrl.secondaryColor
    property alias stickColor: joystickCtrl.stickColor
    property alias returnAnimationDurationMs: joystickCtrl.returnAnimationDurationMs
    property alias xUnitVal: joystickCtrl.xUnitVal
    property alias yUnitVal: joystickCtrl.yUnitVal
    property alias darkerBorders: joystickCtrl.darkerBorders
    // Do not anchor the root item if enabled
    property bool dragActive: false
    property real dragMinX: -10000
    property real dragMaxX: 10000
    property real dragMinY: -10000
    property real dragMaxY: 10000

    property color buttonBackgroundColor: "#FFFFFF"
    property color buttonContentColor: "#000000"
    property real textPointSize: 30
    property color textColor: "white"

    property real panDegrees: NaN
    property real tiltDegrees: NaN

    property real touchScale: 1.5   // Increase button press area, usefull for touch display

    property real sizeScale: textPointSize/10

    property string dpadPos: "Right"

    VirtualJoystick {
        id: joystickCtrl
        sideSize: 116 * _root.sizeScale
        z: 2
    }

    Rectangle {
        id: dpadCtrl

        radius: width * 0.05
        color: _root.secondaryColor

        z: joystickCtrl.z - 1

        Item {
            id: textContent

            Column {
                id: dpadContentCtrl

                spacing: _root.textPointSize

                Column {
                    spacing: parent.spacing/3
                    Text {
                        id: panRefTextCtrl
                        text: qsTr("Pan")
                        color: _root.textColor
                        font.pointSize: _root.textPointSize * 10/16
                    }
                    Text {
                        id: panValueRefTextCtrl
                        text: !isNaN(_root.panDegrees) ? ((_root.panDegrees > 0 ? "+" : "" ) + _root.panDegrees.toFixed(1) + "\u00B0") : "-"
                        color: panRefTextCtrl.color
                        font.pointSize: _root.textPointSize
                    }
                }
                Column {
                    spacing: parent.spacing/3
                    Text {
                        text: qsTr("Tilt")
                        color: panRefTextCtrl.color
                        font.pointSize: panRefTextCtrl.font.pointSize
                    }
                    Text {
                        text: !isNaN(_root.tiltDegrees) ? ((_root.tiltDegrees > 0 ? "+" : "" ) + _root.tiltDegrees.toFixed(1) + "\u00B0") : "-"
                        color: panValueRefTextCtrl.color
                        font.pointSize: panValueRefTextCtrl.font.pointSize
                    }
                }
            }

            Drag.active: true

            MouseArea {
                id: dragArea

                anchors.fill: parent

                drag.target: _root
                drag.minimumX: _root.dragMinX
                drag.maximumX: _root.dragMaxX
                drag.minimumY: _root.dragMinY
                drag.maximumY: _root.dragMaxY
            }
        } // id: textContent
    }

    Button {
        id: closeButton
        width: touchScale * _contentSize
        height: width

        property real _contentSize: 16 * _root.sizeScale

        z: dpadCtrl.z + 1
        flat: true

        background: Item {
            anchors.centerIn: closeButton
            Rectangle {
                anchors.centerIn: parent
                width: closeButton._contentSize
                height: width

                radius: width/2
                color: _root.buttonBackgroundColor
            }
        }

        contentItem: Item {
            anchors.centerIn: parent
            width: parent._contentSize
            height: width

            Image {
                id: closeImg
                anchors.centerIn: parent
                width: parent.width/2
                height: width

                smooth: true
                antialiasing: true
                visible: false
                fillMode: Image.PreserveAspectFit
                source: "/qmlimages/x_sign.svg"
                sourceSize.height: height
                sourceSize.width: width
            }
            ColorOverlay {
                anchors.fill: closeImg
                source: closeImg
                color: _root.buttonContentColor
            }
        }

        onReleased: {
            _root.dpadPos = "None"
        }
    }


    states: [
        State {
            name: "DpadNone"
            when: _root.dpadPos === "None"
            PropertyChanges {
                target: dpadCtrl
                visible: false
            }
            PropertyChanges {
                target: closeButton
                visible: false
            }
        },
        State {
            name: "DpadRight"
            when: _root.dpadPos === "Right"

            PropertyChanges {
                target: _root
                width: joystickCtrl.width + dpadCtrl.visible ? (dpadCtrl.width + closeButton.width/2) : 0
                height: joystickCtrl.height
            }
            AnchorChanges {
                target: joystickCtrl
                anchors.left: joystickCtrl.parent.left
                anchors.top: joystickCtrl.parent.top
            }
            AnchorChanges {
                target: dpadCtrl
                anchors.left: joystickCtrl.horizontalCenter
                anchors.verticalCenter: joystickCtrl.verticalCenter
            }
            PropertyChanges {
                target: dpadCtrl
                width: 125 * _root.sizeScale
                height: 87 * _root.sizeScale
            }
            PropertyChanges {
                target: textContent

                x: joystickCtrl.width/2.0
                y: 0
                width: joystickCtrl.width/2.0
                height: textContent.parent.height
            }
            AnchorChanges {
                target: dpadContentCtrl

                anchors.verticalCenter: dpadContentCtrl.parent.verticalCenter
            }
            PropertyChanges {
                target: dpadContentCtrl

                x: dpadContentCtrl.parent.width * 0.1
            }
            PropertyChanges {
                target: closeButton
                x: dpadCtrl.x + dpadCtrl.width - (closeButton.width/2)
                y: dpadCtrl.y - (closeButton.height/2)
            }
        },
        State {
            name: "DpadLeft"
            when: _root.dpadPos === "Left"

            PropertyChanges {
                target: _root
                width: joystickCtrl.width + dpadCtrl.visible ? dpadCtrl.width + closeButton.width/2 : 0
                height: joystickCtrl.height
            }
            AnchorChanges {
                target: joystickCtrl
                anchors.right: joystickCtrl.parent.right
                anchors.top: joystickCtrl.parent.top
            }
            AnchorChanges {
                target: dpadCtrl
                anchors.right: joystickCtrl.horizontalCenter
                anchors.verticalCenter: joystickCtrl.verticalCenter
            }
            PropertyChanges {
                target: dpadCtrl
                width: 125 * _root.sizeScale
                height: 87 * _root.sizeScale
            }
            PropertyChanges {
                target: textContent

                x: 0
                y: 0
                width: joystickCtrl.width/2.0
                height: textContent.parent.height
            }
            AnchorChanges {
                target: dpadContentCtrl

                anchors.verticalCenter: dpadContentCtrl.parent.verticalCenter
            }
            PropertyChanges {
                target: dpadContentCtrl

                x: dpadContentCtrl.parent.width * 0.1
            }
            PropertyChanges {
                target: closeButton
                x: dpadCtrl.x - (closeButton.width/2)
                y: dpadCtrl.y - (closeButton.height/2)
            }
        },
        State {
            name: "DpadTop"
            when: _root.dpadPos === "Top"

            PropertyChanges {
                target: _root
                width: joystickCtrl.width
                height: joystickCtrl.height + dpadCtrl.visible ? dpadCtrl.height + closeButton.height/2 : 0
            }
            AnchorChanges {
                target: joystickCtrl
                anchors.left: joystickCtrl.parent.left
                anchors.bottom: joystickCtrl.parent.bottom
            }
            AnchorChanges {
                target: dpadCtrl
                anchors.bottom: joystickCtrl.verticalCenter
                anchors.horizontalCenter: joystickCtrl.horizontalCenter
            }
            PropertyChanges {
                target: dpadCtrl
                width: 67 * _root.sizeScale
                height: 135 * _root.sizeScale
            }
            PropertyChanges {
                target: textContent

                x: 0
                y: 0
                width: joystickCtrl.width
                height: textContent.parent.height/2.0
            }
            PropertyChanges {
                target: dpadContentCtrl

                x: dpadContentCtrl.parent.width * 0.05
                y: dpadContentCtrl.parent.width * 0.05
            }
            PropertyChanges {
                target: closeButton
                x: dpadCtrl.x + dpadCtrl.width - (closeButton.width/2)
                y: dpadCtrl.y - (height/2)
            }
        },
        State {
            name: "DpadBottom"
            when: _root.dpadPos === "Bottom"

            PropertyChanges {
                target: _root
                width: joystickCtrl.width
                height: joystickCtrl.height + dpadCtrl.visible ? dpadCtrl.height + closeButton.height/2 : 0
            }
            AnchorChanges {
                target: joystickCtrl
                anchors.left: joystickCtrl.parent.left
                anchors.top: joystickCtrl.parent.top
            }
            AnchorChanges {
                target: dpadCtrl
                anchors.top: joystickCtrl.verticalCenter
                anchors.horizontalCenter: joystickCtrl.horizontalCenter
            }
            PropertyChanges {
                target: dpadCtrl
                width: 67 * _root.sizeScale
                height: 135 * _root.sizeScale
            }
            PropertyChanges {
                target: textContent

                y: joystickCtrl.width/2.0
                x: 0
                width: joystickCtrl.width
                height: textContent.parent.height/2.0
            }
            PropertyChanges {
                target: dpadContentCtrl

                x: dpadContentCtrl.parent.width * 0.05
                y: dpadContentCtrl.parent.width * 0.05
            }
            PropertyChanges {
                target: closeButton
                x: dpadCtrl.x + dpadCtrl.width - (closeButton.width/2)
                y: dpadCtrl.y + dpadCtrl.height - (closeButton.height/2)
            }
        }
    ]
}
