import QtQuick
import QtQuick.Effects
import QtQuick.Shapes 2.15

import QGroundControl


Item {
    id: root

    property string joystickType: "standard"
    readonly property bool controlsLocked: SVState.lockControls
    readonly property url lockedTextureSource: Qt.resolvedUrl("../Resources/Images/halftone_texture_2.png")
    readonly property int lockedTextureTileSize: 250

    QGCPalette { id: qgcPalette }

    Rectangle {
        id: ackground
        anchors.fill: parent
        radius: width / 2
        color: qgcPalette.window
        border.width: 1
        border.color: qgcPalette.windowShadeLight
        visible: false
    }

    Item {
        id: innerJoystick
        anchors.fill: parent
        //anchors.margins: SVUnits.margin / 2

        Loader {
            id: joystickLoader
            anchors.fill: parent

            sourceComponent: {
                if(root.joystickType === "standard") {
                    return standardComponent
                } else if(root.joystickType === "simple") {
                    return simpleComponent
                } else if(root.joystickType === "drag") {
                    return dragComponent
                }
            }
        }

        Item {
            id: lockOverlay
            anchors.fill: parent
            anchors.margins: SVUnits.margin / 2
            visible: root.controlsLocked

            Image {
                id: lockOverlayTexture
                anchors.fill: parent
                fillMode: Image.Tile
                sourceSize.width: root.lockedTextureTileSize
                sourceSize.height: root.lockedTextureTileSize
                visible: false
                source: root.lockedTextureSource
            }

            MultiEffect {
                anchors.fill: parent
                source: lockOverlayTexture
                maskEnabled: true
                maskSource: lockOverlayMask
                opacity: 0.05
            }

            Item {
                id: lockOverlayMask
                anchors.fill: parent
                layer.enabled: true
                visible: false

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "black"
                }
            }
        }

        Rectangle {
            id: outerBorder
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: qgcPalette.statusPassedText
            radius: width / 2
        }
    }

    Component {
        id: standardComponent
        SVJoystickArea {
            hasInnerRing: true
        }
    }

    Component {
        id: simpleComponent
        SVJoystickArea {
            hasInnerRing: false
        }
    }

    Component {
        id: dragComponent
        SVJoystickDrag {}
    }
}