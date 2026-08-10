/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Experimental 2D/3D map view (preview feature): the GeoMap-engine
/// counterpart of FlyView. View chrome around a FlyViewGeoMap.
Item {
    id: root

    QGCPalette { id: qgcPal }

    Rectangle {
        id: toolbar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: ScreenTools.toolbarHeight
        color: qgcPal.toolbarBackground

        QGCToolBarButton {
            objectName: "toolbar_qgcLogo"
            height: parent.height
            icon.source: "/res/QGCLogoFull.svg"
            logo: true
            onClicked: mainWindow.showToolSelectDialog()
        }

        QGCLabel {
            anchors.centerIn: parent
            text: qsTr("GeoView (preview)")
            font.pointSize: ScreenTools.largeFontPointSize
        }
    }

    // The 3D scene only exists while this is the active view: View3D
    // costs RHI/scene-graph resources even when hidden.
    Loader {
        id: sceneLoader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: toolbar.bottom
        anchors.bottom: parent.bottom
        active: root.visible
        sourceComponent: Item {
            FlyViewGeoMap {
                id: geoMap
                anchors.fill: parent
            }

            // Debug overlay: live SurfaceModel stats for manual verification
            QGCLabel {
                objectName: "flyViewGeoDebugOverlay"
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: ScreenTools.defaultFontPixelWidth
                font.family: ScreenTools.fixedFontFamily
                text: qsTr("patches: %1  pending: %2  max zoom: %3")
                          .arg(geoMap.patchCount)
                          .arg(geoMap.pendingCount)
                          .arg(geoMap.maxZoomLevel)
            }

            Column {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: ScreenTools.defaultFontPixelWidth
                spacing: ScreenTools.defaultFontPixelHeight / 2

                // 2D<->3D mode switch: the mode flips immediately (2D locks tilt
                // gestures); GeoMap's mode-change handler animates tilt and terrain
                QGCButton {
                    objectName: "flyViewGeoModeButton"
                    text: geoMap.camera.mode === GeoMapCamera.Mode2D ? qsTr("3D") : qsTr("2D")
                    onClicked: geoMap.camera.mode = (geoMap.camera.mode === GeoMapCamera.Mode2D)
                                   ? GeoMapCamera.Mode3D : GeoMapCamera.Mode2D
                }

                // Compass: needle tracks heading, click animates back to north-up
                QGCButton {
                    objectName: "flyViewGeoCompassButton"
                    text: qsTr("N")
                    rotation: -geoMap.camera.heading
                    onClicked: geoMap.animateHeadingToNorth()
                }

                // Diagnostic: report holes, terrain cliffs, and bad height
                // data to the debug output
                QGCButton {
                    objectName: "flyViewGeoAnalyzeButton"
                    text: qsTr("Analyze")
                    onClicked: geoMap.analyzeSurface()
                }
            }
        }
    }
}
