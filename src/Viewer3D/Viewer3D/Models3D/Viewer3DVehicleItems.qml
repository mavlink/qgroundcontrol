import QtQuick
import QtQuick3D
import QtPositioning

import Viewer3D.Models3D.Drones
import Viewer3D.Models3D
import QGroundControl.Viewer3D

import QGroundControl
import QGroundControl.Controllers
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Vehicle

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Node {
    id: vehicel3DBody
    property var  _backendQml:                  null
    property var  _vehicle:                     null
    property var  _planMasterController:        null
    property var  _missionController:           (_planMasterController)?(_planMasterController.missionController):(null)
    property var _viewer3DSetting:              QGroundControl.settingsManager.viewer3DSettings
    property var _altitudeBias:                 _viewer3DSetting.altitudeBias.rawValue


    function addMissionItemsToListModel() {
        missionWaypointListModel.clear()
        var _geo2EnuCopy = goe2Enu

        for (var i = 1; i < _missionController.visualItems.count; i++) {
            var _missionItem = _missionController.visualItems.get(i)
            _geo2EnuCopy.coordinate = _missionItem.coordinate
            _geo2EnuCopy.coordinate.altitude = 0
            missionWaypointListModel.append({
                                                "x": _geo2EnuCopy.localCoordinate.x,
                                                "y": _geo2EnuCopy.localCoordinate.y,
                                                "z": _missionItem.altitude.value,
                                                "isTakeoffItem": _missionItem.isTakeoffItem,
                                                "index": _missionItem.sequenceNumber,
                                            })
        }
    }

    function addSegmentToMissionPathModel() {
        missionPathModel.clear()
        var _geo2EnuCopy = goe2Enu

        for (var i = 2; i < _missionController.visualItems.count; i++) {
            var _missionItem = _missionController.visualItems.get(i-1)
            _geo2EnuCopy.coordinate = _missionItem.coordinate
            _geo2EnuCopy.coordinate.altitude = 0
            var p1 = Qt.vector3d(_geo2EnuCopy.localCoordinate.x, _geo2EnuCopy.localCoordinate.y, _missionItem.altitude.value)

            _missionItem = _missionController.visualItems.get(i)
            _geo2EnuCopy.coordinate = _missionItem.coordinate
            _geo2EnuCopy.coordinate.altitude = 0
            var p2 = Qt.vector3d(_geo2EnuCopy.localCoordinate.x, _geo2EnuCopy.localCoordinate.y, _missionItem.altitude.value)

            missionPathModel.append({
                                        "x_1": p1.x,
                                        "y_1": p1.y,
                                        "z_1": p1.z,
                                        "x_2": p2.x,
                                        "y_2": p2.y,
                                        "z_2": p2.z,
                                    })
        }
    }

    GeoCoordinateType{
        id:goe2Enu
        gpsRef: _backendQml.gpsRef
    }

    ListModel{
        id: missionWaypointListModel
    }

    ListModel{
        id: missionPathModel
    }

    DroneModelDjiF450{
        id: droneDji3DModel
        vehicle: _vehicle
        modelScale: Qt.vector3d(0.05, 0.05, 0.05)
        altitudeBias: _altitudeBias
        gpsRef: _backendQml.gpsRef
    }

    Repeater3D{
        id:waypints3DRepeater
        model: missionWaypointListModel

        delegate: Waypoint3DModel{
            opacity: 0.8
            missionItem: model
            altitudeBias: _altitudeBias
        }
    }

    Repeater3D{
        id:mission3DPathRepeater
        model: missionPathModel

        delegate: Line3D{
            p_1: Qt.vector3d(model.x_1 * 10, model.y_1 * 10, (model.z_1 + _altitudeBias) * 10)
            p_2: Qt.vector3d(model.x_2 * 10, model.y_2 * 10, (model.z_2 + _altitudeBias) * 10)
            lineWidth:8
            color: "orange"
        }
    }

    Connections {
        target: _missionController
        onVisualItemsChanged: {
            addMissionItemsToListModel()
            addSegmentToMissionPathModel()

        }
    }

    Connections {
        target: _backendQml
        onGpsRefChanged: {
            addMissionItemsToListModel()
            addSegmentToMissionPathModel()

        }
    }
}
