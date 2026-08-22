/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Shapes

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Google Earth-style GeoMap camera controls: round 2D/3D mode toggle plus a
/// compass whose needle tracks the camera heading and tilts with the camera.
Row {
    id: root

    required property var geoMap

    spacing: ScreenTools.defaultFontPixelWidth / 2

    readonly property var _camera: root.geoMap ? root.geoMap.camera : null
    readonly property real _diameter: ScreenTools.defaultFontPixelHeight * 2.25

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Rectangle {
        objectName: "flyViewGeoMapModeButton"

        // Shows the mode a click switches to (UI tests read this too)
        property string text: root._camera && root._camera.mode === GeoMapCamera.Mode2D ? qsTr("3D") : qsTr("2D")

        width: root._diameter
        height: root._diameter
        radius: root._diameter / 2
        color: modeMouseArea.pressed ? qgcPal.buttonHighlight : qgcPal.button

        QGCLabel {
            anchors.centerIn: parent
            text: parent.text
            font.bold: true
            color: modeMouseArea.pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
        }

        MouseArea {
            id: modeMouseArea
            anchors.fill: parent
            onClicked: {
                if (root._camera) {
                    root._camera.mode = (root._camera.mode === GeoMapCamera.Mode2D)
                        ? GeoMapCamera.Mode3D : GeoMapCamera.Mode2D
                }
            }
        }
    }

    Rectangle {
        objectName: "flyViewGeoMapCompassButton"
        width: root._diameter
        height: root._diameter
        radius: root._diameter / 2
        color: compassMouseArea.pressed ? qgcPal.buttonHighlight : qgcPal.button

        // The face tilts with the camera like Google Earth, foreshortening
        // the needle as the view moves off top-down
        Item {
            id: compassFace
            anchors.fill: parent
            transform: Rotation {
                origin.x: compassFace.width / 2
                origin.y: compassFace.height / 2
                axis { x: 1; y: 0; z: 0 }
                angle: root._camera ? root._camera.tilt : 0
            }

            // Nested so the in-plane heading rotation applies before the tilt
            Item {
                id: needle
                anchors.fill: parent
                rotation: root._camera ? -root._camera.heading : 0

                Shape {
                    anchors.fill: parent
                    preferredRendererType: Shape.CurveRenderer

                    // North half: conventional red needle tip
                    ShapePath {
                        strokeWidth: 1
                        strokeColor: "#80404040"
                        fillColor: "#e8594b"
                        startX: needle.width / 2
                        startY: needle.height * 0.14
                        PathLine { x: needle.width * 0.32; y: needle.height / 2 }
                        PathLine { x: needle.width * 0.68; y: needle.height / 2 }
                        PathLine { x: needle.width / 2; y: needle.height * 0.14 }
                    }

                    // South half
                    ShapePath {
                        strokeWidth: 1
                        strokeColor: "#80404040"
                        fillColor: "white"
                        startX: needle.width / 2
                        startY: needle.height * 0.86
                        PathLine { x: needle.width * 0.32; y: needle.height / 2 }
                        PathLine { x: needle.width * 0.68; y: needle.height / 2 }
                        PathLine { x: needle.width / 2; y: needle.height * 0.86 }
                    }
                }
            }
        }

        MouseArea {
            id: compassMouseArea
            anchors.fill: parent
            onClicked: {
                if (root.geoMap) {
                    root.geoMap.animateHeadingToNorth()
                }
            }
        }
    }
}
