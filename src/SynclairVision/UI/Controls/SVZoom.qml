import QtQuick
import QtQuick.Effects
import QtQuick.Shapes 2.15

import QGroundControl

Item {
    id: root

    readonly property bool controlsLocked: SVState.lockControls
    readonly property url lockedTextureSource: Qt.resolvedUrl("../Resources/Images/halftone_texture_2.png")
    readonly property int lockedTextureTileSize: 250

    QGCPalette { id: qgcPalette }

    Rectangle {
        id: border
        anchors.fill: parent
        radius: width / 2
        color: qgcPalette.window
        border.width: 1
        border.color: qgcPalette.windowShadeLight
    }

    SVZoomArea {
        id: zoomArea
        anchors.fill: parent
        anchors.margins: SVUnits.margin / 2
    }

    Item {
        id: lockOverlay
        anchors.fill: parent
        anchors.margins: SVUnits.margin
        visible: root.controlsLocked
        z: 1

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
}
