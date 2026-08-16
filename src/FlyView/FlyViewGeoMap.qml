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
}
