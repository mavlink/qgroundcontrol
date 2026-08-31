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
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Debug-build-only test harness for GeoMap items: renders the selected item
/// type at the map center for visual verification in 2D and 3D. Re-select
/// from the combo to re-place the item at the current center.
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
            icon.source: "/res/godwitLogoWhite.svg"
            logo: true
            onClicked: mainWindow.showToolSelectDialog()
        }

        QGCLabel {
            anchors.centerIn: parent
            text: "GeoMap Test"     // debug-only developer tool: not translated
            font.pointSize: ScreenTools.largeFontPointSize
        }

        QGCComboBox {
            id: itemCombo
            anchors.right: parent.right
            anchors.rightMargin: ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
            sizeToContents: true
            model: ["Polygon", "Circle", "Item Indicator", "Pin", "Waypoint", "Height Badge", "Segment Arrow", "Projected Path", "Fence Walls", "ADSB", "Camera Triggers"]
            onActivated: {
                if (sceneLoader.item) {
                    sceneLoader.item.placeAtCenter()
                }
            }
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
            id: testHost

            property var testCenter: QtPositioning.coordinate()

            function placeAtCenter() {
                testHost.testCenter = geoMap.camera.center
            }

            function squareCoordinates(center, halfMeters) {
                if (!center || !center.isValid) {
                    return []
                }
                const diagonal = halfMeters * Math.SQRT2
                return [
                    center.atDistanceAndAzimuth(diagonal, 315),
                    center.atDistanceAndAzimuth(diagonal, 45),
                    center.atDistanceAndAzimuth(diagonal, 135),
                    center.atDistanceAndAzimuth(diagonal, 225)
                ]
            }

            function elevatedCoordinate(center, aglMeters) {
                if (!center || !center.isValid) {
                    return QtPositioning.coordinate()
                }
                return QtPositioning.coordinate(center.latitude, center.longitude,
                                                geoMap.surfaceModel.terrainHeightAt(center) + aglMeters)
            }

            // 500-point survey-style grid to stress the batched sprite layer
            function triggerGridCoordinates(center) {
                if (!center || !center.isValid) {
                    return []
                }
                const coords = []
                for (let row = 0; row < 20; row++) {
                    const rowStart = center.atDistanceAndAzimuth((row - 10) * 60, 0)
                    for (let col = 0; col < 25; col++) {
                        coords.push(rowStart.atDistanceAndAzimuth((col - 12) * 60, 90))
                    }
                }
                return coords
            }

            // Climbing west->east zigzag: 4 segments, 25 m altitude steps
            function zigzagCoordinates(center) {
                if (!center || !center.isValid) {
                    return []
                }
                const coords = []
                for (let i = 0; i < 5; i++) {
                    const east = center.atDistanceAndAzimuth(-600 + i * 300, 90)
                    const point = east.atDistanceAndAzimuth(150, (i % 2) ? 0 : 180)
                    coords.push(elevatedCoordinate(point, 50 + i * 25))
                }
                return coords
            }

            Component.onCompleted: placeAtCenter()

            GeoMap {
                id: geoMap
                anchors.fill: parent
                allowGCSLocationCenter: true

                Column {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    // 2D<->3D mode switch (same conventions as FlyViewGeoMapChrome)
                    QGCButton {
                        objectName: "geoMapTest_ModeButton"
                        text: geoMap.camera.mode === GeoMapCamera.Mode2D ? "3D" : "2D"
                        onClicked: geoMap.camera.mode = (geoMap.camera.mode === GeoMapCamera.Mode2D)
                                       ? GeoMapCamera.Mode3D : GeoMapCamera.Mode2D
                    }

                    // Compass: needle tracks heading, click animates back to north-up
                    QGCButton {
                        objectName: "geoMapTest_CompassButton"
                        text: "N"
                        rotation: -geoMap.camera.heading
                        onClicked: geoMap.animateHeadingToNorth()
                    }
                }

                GeoMapPolygon {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 0
                    coordinates: testHost.squareCoordinates(testHost.testCenter, 300)
                    strokeColor: "orange"
                    fillColor: Qt.alpha("orange", 0.2)
                }

                GeoMapCircle {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 1
                    center: testHost.testCenter
                    radiusMeters: 300
                    strokeColor: "orange"
                    fillColor: Qt.alpha("orange", 0.2)
                }

                GeoMapMissionLabel {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 2
                    // 100 m above terrain to exercise Absolute altitude rendering in 3D
                    coordinate: testHost.elevatedCoordinate(testHost.testCenter, 100)
                    altitudeMode: GeoMapItem.Absolute
                    checked: true
                    label: "Test"
                }

                GeoMapPin {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 3
                    coordinate: testHost.testCenter
                    label: "P"
                }

                GeoMapWaypointItem {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 4
                    item: fakeMissionItem

                    // VisualMissionItem stand-in: just the properties the marker reads
                    QtObject {
                        id: fakeMissionItem
                        property var coordinate: testHost.testCenter
                        property real amslEntryAlt: testHost.elevatedCoordinate(testHost.testCenter, 100).altitude
                        property bool isSimpleItem: true
                        property bool specifiesCoordinate: true
                        property bool isCurrentItem: false
                        property bool hasCurrentChildItem: false
                        property int sequenceNumber: 1
                    }
                }

                GeoMapMissionHeightBadge {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 5
                    coordinate: testHost.testCenter
                    text: "100 m"
                }

                GeoMapSegmentArrow {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 6
                    // 50 m AGL west->east segment (projector runs in Absolute mode)
                    fromCoord: testHost.testCenter.isValid ? testHost.elevatedCoordinate(testHost.testCenter.atDistanceAndAzimuth(300, 270), 50) : QtPositioning.coordinate()
                    toCoord: testHost.testCenter.isValid ? testHost.elevatedCoordinate(testHost.testCenter.atDistanceAndAzimuth(300, 90), 50) : QtPositioning.coordinate()
                }

                // Raw projected polyline: the primitive under mission/survey/approach lines
                Item {
                    visible: itemCombo.currentIndex === 7

                    GeoMapProjectedPath {
                        id: testProjectedPath
                        scene: geoMap.scene
                        surfaceModel: geoMap.surfaceModel
                        altitudeMode: GeoMapItem.Absolute
                        coordinates: (parent.visible) ? testHost.zigzagCoordinates(testHost.testCenter) : []
                    }

                    Shape {
                        visible: testProjectedPath.projected

                        ShapePath {
                            strokeColor: "orange"
                            strokeWidth: 3
                            fillColor: "transparent"
                            PathPolyline { path: testProjectedPath.screenPoints }
                        }
                    }
                }

                // Extruded fence walls (400 ft AGL): polygon west, circle east
                GeoMapPolygon {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 8
                    coordinates: testHost.testCenter.isValid ? testHost.squareCoordinates(testHost.testCenter.atDistanceAndAzimuth(400, 270), 300) : []
                    strokeColor: "orange"
                    strokeWidth: 2
                    extrudeHeightMeters: 121.92
                }

                GeoMapCircle {
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    visible: itemCombo.currentIndex === 8
                    center: testHost.testCenter.isValid ? testHost.testCenter.atDistanceAndAzimuth(400, 90) : QtPositioning.coordinate()
                    radiusMeters: 300
                    strokeColor: "orange"
                    strokeWidth: 2
                    extrudeHeightMeters: 121.92
                }

                // Fake ADS-B traffic: aloft, alert, and no-altitude (clamped) targets
                Repeater {
                    model: [
                        { azimuth: 315, agl: 150, callsign: "N123AB", heading: 45, alert: false },
                        { azimuth: 45, agl: 300, callsign: "N987CD", heading: 270, alert: true },
                        { azimuth: 180, agl: Number.NaN, callsign: "N555EF", heading: 0, alert: false }
                    ]

                    GeoMapADSBVehicleItem {
                        scene: geoMap.scene
                        surfaceModel: geoMap.surfaceModel
                        visible: itemCombo.currentIndex === 9
                        coordinate: testHost.testCenter.isValid ? testHost.testCenter.atDistanceAndAzimuth(400, modelData.azimuth) : QtPositioning.coordinate()
                        altitude: isNaN(modelData.agl) ? Number.NaN : testHost.elevatedCoordinate(testHost.testCenter, modelData.agl).altitude
                        callsign: modelData.callsign
                        heading: modelData.heading
                        alert: modelData.alert
                    }
                }

                // Fake camera trigger points through the batched sprite layer
                GeoMapSpriteLayer {
                    visible: itemCombo.currentIndex === 10
                    scene: geoMap.scene
                    surfaceModel: geoMap.surfaceModel
                    coordinates: visible ? testHost.triggerGridCoordinates(testHost.testCenter) : []
                    sprite: testTriggerIconSource
                    spriteSize: testTriggerIcon.width

                    CameraTriggerIcon { id: testTriggerIcon }
                    ShaderEffectSource {
                        id: testTriggerIconSource
                        sourceItem: testTriggerIcon
                        hideSource: true
                        textureSize: Qt.size(testTriggerIcon.width * Screen.devicePixelRatio, testTriggerIcon.height * Screen.devicePixelRatio)
                    }
                }
            }
        }
    }
}
