/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtPositioning

import QGroundControl
import QGroundControl.GeoMap

/// Fly-view 2D/3D map: the GeoMap-engine counterpart of FlyViewMap.
/// Fly-view specific map content (vehicle items, trajectory, ...) lands here.
GeoMap {
    id: root

    allowGCSLocationCenter: true
    allowVehicleLocationCenter: true
    keepVehicleCentered: QGroundControl.settingsManager.flyViewSettings.keepMapCenteredOnVehicle.rawValue

    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _missionController: globals.planMasterControllerFlyView ? globals.planMasterControllerFlyView.missionController : null

    // DEM height minus vehicle-reported AMSL, sampled at home for the active
    // vehicle. Computed once here and shared by GeoMapMissionPath and every
    // GeoMapMissionItems marker instead of each re-sampling the same DEM
    // lookup independently (see GeoMapVehicleItem for the same correction
    // applied per-vehicle in the marker Repeater below, where it can't be
    // shared since each vehicle has its own home position).
    property real _activeVehicleHomeTerrainBias: 0

    function _updateActiveVehicleHomeTerrainBias() {
        if (root.surfaceModel && root._activeVehicle && root._activeVehicle.homePosition.isValid && !isNaN(root._activeVehicle.homePosition.altitude)) {
            root._activeVehicleHomeTerrainBias = root.surfaceModel.terrainHeightAt(root._activeVehicle.homePosition) - root._activeVehicle.homePosition.altitude
        } else {
            root._activeVehicleHomeTerrainBias = 0
        }
    }

    on_ActiveVehicleChanged: _updateActiveVehicleHomeTerrainBias()
    onSurfaceModelChanged: _updateActiveVehicleHomeTerrainBias()

    Connections {
        target: root.surfaceModel

        function onTerrainHeightsChanged() {
            root._updateActiveVehicleHomeTerrainBias()
        }
    }

    Connections {
        target: root._activeVehicle

        function onHomePositionChanged() {
            root._updateActiveVehicleHomeTerrainBias()
        }
    }

    // Vehicle home (launch) location
    GeoMapPin {
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root._activeVehicle ? root._activeVehicle.homePosition : QtPositioning.coordinate()
        visible: root._activeVehicle && root._activeVehicle.homePosition.isValid
        label: qsTr("L")
    }

    Repeater {
        model: QGroundControl.multiVehicleManager.vehicles

        GeoMapVehicleItem {
            scene: root.scene
            surfaceModel: root.surfaceModel
            vehicle: object
        }
    }

    GeoMapFlightPath {
        scene: root.scene
        surfaceModel: root.surfaceModel
        vehicle: root._activeVehicle
    }

    GeoMapMissionPath {
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        vehicle: root._activeVehicle
        homeTerrainBias: root._activeVehicleHomeTerrainBias
    }

    GeoMapMissionItems {
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        homeTerrainBias: root._activeVehicleHomeTerrainBias
    }

    GeoMapMissionDirectionArrows {
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        homeTerrainBias: root._activeVehicleHomeTerrainBias
    }
}
