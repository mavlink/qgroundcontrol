import QtQuick
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.GeoMap

/// GeoMap-engine drop-in for the QtLocation FlightMap of Plan View. GeoMapView provides the editorMap contract Plan
/// View and its panels consume (center, zoomLevel, toCoordinate/fromCoordinate, setVisibleRegion, mapClicked, ...).
/// The plan is displayed, and the current simple mission item and rally point can be dragged. Editing fence points
/// and pattern items on this map is not supported yet.
GeoMapView {
    id: root

    property var planMasterController
    property rect centerViewport: Qt.rect(0, 0, width, height) ///< Part of the map not covered by Plan View tools
    property real missionOpacity: 1                            ///< Dims the mission while another layer is edited
    property bool missionInteractive: false                    ///< The current mission item can be dragged on the map
    property real rallyOpacity: 1                              ///< Dims the rally points while another layer is edited
    property bool rallyInteractive: false                      ///< Rally points can be selected and the current one dragged
    property real _nonInteractiveOpacity: 0.5                  ///< Consumed by the Plan View panels

    readonly property var _missionController: planMasterController ? planMasterController.missionController : null
    readonly property var _rallyPointController: planMasterController ? planMasterController.rallyPointController : null
    readonly property var _geoFenceController: planMasterController ? planMasterController.geoFenceController : null
    readonly property var _plannedHomePosition: _missionController ? _missionController.plannedHomePosition : QtPositioning.coordinate()

    property var _missionItemSelectionPanel: null

    /// A mission item marker was picked on the map
    signal itemSelected(int sequenceNumber)

    function _closeMissionItemSelectionPanel() {
        if (_missionItemSelectionPanel) {
            _missionItemSelectionPanel.close()
        }
    }

    allowGCSLocationCenter: true
    allowVehicleLocationCenter: true
    // The plan is edited away from the vehicle: only the one-time centering applies
    followOffscreenVehicle: false

    GeoMapPin {
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root._plannedHomePosition
        visible: root._plannedHomePosition.isValid
        label: qsTr("H")
    }

    Repeater {
        model: QGroundControl.multiVehicleManager.vehicles

        GeoMapVehicleItem {
            scene: root.scene
            surfaceModel: root.surfaceModel
            vehicle: object
        }
    }

    GeoMapMissionPath {
        objectName: "geoMapMissionPath"
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        vehicle: root.planMasterController ? root.planMasterController.controllerVehicle : null
        plannedHomePosition: root._plannedHomePosition
        opacity: root.missionOpacity
    }

    GeoMapMissionItems {
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        opacity: root.missionOpacity
        interactive: root.missionInteractive
        pickable: root.missionInteractive
        onItemClicked: (item) => root.itemSelected(item.sequenceNumber)
        // Dragging only moves the item horizontally: its altitude is kept apart from its coordinate
        onItemDragged: (item, coordinate) => item.coordinate = QtPositioning.coordinate(coordinate.latitude, coordinate.longitude)
        onGroupClicked: (items, marker) => {
            root._closeMissionItemSelectionPanel()
            const topLeft = marker.mapToItem(globals.parent, 0, 0)
            root._missionItemSelectionPanel = missionItemSelectionPanelComponent.createObject(mainWindow, {
                clickRect: Qt.rect(topLeft.x, topLeft.y, marker.width, marker.height),
                groupItems: items
            })
            if (root._missionItemSelectionPanel) {
                root._missionItemSelectionPanel.open()
            }
        }
        onGroupsInvalidated: root._closeMissionItemSelectionPanel()
    }

    GeoMapMissionDirectionArrows {
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        opacity: root.missionOpacity
    }

    GeoMapGeoFenceVisuals {
        scene: root.scene
        surfaceModel: root.surfaceModel
        geoFenceController: root._geoFenceController
        homePosition: root._plannedHomePosition
    }

    Repeater {
        model: root._rallyPointController ? root._rallyPointController.points : undefined

        GeoMapRallyPoint {
            scene: root.scene
            surfaceModel: root.surfaceModel
            rallyPoint: object
            vehicle: root.planMasterController ? root.planMasterController.controllerVehicle : null
            homeAltitude: root._plannedHomePosition.altitude
            checked: root.rallyInteractive && object === root._rallyPointController.currentRallyPoint
            opacity: root.rallyOpacity
            node3DOpacity: root.rallyOpacity
            pickable: root.rallyInteractive
            draggable: checked

            onClicked: {
                if (mainWindow.allowViewSwitch()) {
                    root._rallyPointController.currentRallyPoint = object
                }
            }
            // The drag only moves the point horizontally, keeping its altitude
            onDragged: (coordinate) => object.coordinate = QtPositioning.coordinate(coordinate.latitude, coordinate.longitude, object.coordinate.altitude)
            // A PX4 AMSL altitude is the ground height under the point, so it is read again where the point lands
            onDragFinished: {
                const vehicle = root.planMasterController.controllerVehicle
                if (vehicle && vehicle.px4Firmware && object.altitudeFrame === QGroundControl.AltitudeFrameAbsolute) {
                    object.useTerrainAltitude()
                }
            }
        }
    }

    Component {
        id: missionItemSelectionPanelComponent

        MissionItemSelectionPanel {
            id: missionItemSelectionPanel

            onItemSelected: (sequenceNumber) => root.itemSelected(sequenceNumber)
            onClosed: {
                if (root._missionItemSelectionPanel === missionItemSelectionPanel) {
                    root._missionItemSelectionPanel = null
                }
                destroy()
            }
        }
    }

    // The picker lists the old mission's items: close it when the mission is replaced
    Connections {
        target: root._missionController

        function onVisualItemsReset() {
            root._closeMissionItemSelectionPanel()
        }
    }
}
