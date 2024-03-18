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
    property bool _rtlActive:                   false


    function isItemAcceptable(missionItem){
        const acceptableCmdIds = [
                                   16, // Waypoint
                                   20, // Return To Launch
                                   22, //Takeoff
                                   195, // ROI
                                   201, // ROI DEPRECATED
                               ]; // based on MavCmdInfoCommon.json file
        return acceptableCmdIds.includes(missionItem.command);
    }

    function isLaunchItem(missionItem){
        return missionItem.abbreviation === "L";
    }

    function isReturnToLaunchItem(missionItem){
        return missionItem.command === 20;
    }


    function getItemName(missionItem){
        if(isLaunchItem(missionItem) || isReturnToLaunchItem(missionItem)){
            return qsTr("L");
        }

        if(missionItem.isTakeoffItem){
            return qsTr("T"); //Takeoff
        }

        if(missionItem.specifiesCoordinate){
            switch(missionItem.command){
            case 16:
                return qsTr("W"); //Waypoint
            case 22:
                return qsTr("T"); //Takeoff
            case 195:
                return qsTr("R"); //ROI
            case 201:
                return qsTr("R"); //ROI DEPRECATED
            }
        }
        return qsTr("null")
    }

    function addMissionItemsToListModel() {
        missionWaypointListModel.clear()
        var _geo2EnuCopy = goe2Enu
        var launchItemCoordinate = null;
        var _missionItemPrevious = null;
        _rtlActive = false;

        for (var i = 0; i < _missionController.visualItems.count; i++) {
            var _missionItem = _missionController.visualItems.get(i); // list of all properties in VisualMissionItem.h and SimpleMissionItem.h
            if(isLaunchItem(_missionItem)){
                launchItemCoordinate = _missionItem.coordinate;
                continue;
            }
            if(isItemAcceptable(_missionItem)){
                if(isReturnToLaunchItem(_missionItem)){
                    _rtlActive = true;
                    _geo2EnuCopy.coordinate = (_vehicle.homePosition)?(_vehicle.homePosition):(_missionItem.coordinate);
                    _geo2EnuCopy.coordinate.altitude = _missionItemPrevious.altitude.value;
                }else{
                    _geo2EnuCopy.coordinate = _missionItem.coordinate;
                    _geo2EnuCopy.coordinate.altitude = _missionItem.altitude.value;
                }
                missionWaypointListModel.append({
                                                    "x": _geo2EnuCopy.localCoordinate.x,
                                                    "y": _geo2EnuCopy.localCoordinate.y,
                                                    "z": _geo2EnuCopy.coordinate.altitude,
                                                    "itemName": getItemName(_missionItem),
                                                    "index": _missionItem.sequenceNumber,
                                                });
                if(getItemName(_missionItem) === "L" || getItemName(_missionItem) === "W"){
                    _missionItemPrevious = _missionItem;
                }
            }
        }
    }

    function addSegmentToMissionPathModel() {
        missionPathModel.clear()
        var _geo2EnuCopy = goe2Enu

        var _missionItem;
        var _missionItemPrevious = null
        var launchItemCoordinate;

        for (var i = 0; i < _missionController.visualItems.count; i++) {
            _missionItem = _missionController.visualItems.get(i);
            if(isLaunchItem(_missionItem)){
                launchItemCoordinate = _missionItem.coordinate;
                continue;
            }
            if(_missionItem.isTakeoffItem){
                _missionItemPrevious = _missionItem;
                continue;
            }
            if(_missionItemPrevious === null){
                continue;
            }
            if(getItemName(_missionItem) === "L" || getItemName(_missionItem) === "W"){
                _geo2EnuCopy.coordinate = _missionItemPrevious.coordinate;
                _geo2EnuCopy.coordinate.altitude = _missionItemPrevious.altitude.value;
                var p1 = Qt.vector3d(_geo2EnuCopy.localCoordinate.x, _geo2EnuCopy.localCoordinate.y, _geo2EnuCopy.coordinate.altitude);

                if(isReturnToLaunchItem(_missionItem)){
                    _geo2EnuCopy.coordinate = (_vehicle.homePosition)?(_vehicle.homePosition):(_missionItem.coordinate);;
                    _geo2EnuCopy.coordinate.altitude = _missionItemPrevious.altitude.value;
                }else{
                    _geo2EnuCopy.coordinate = _missionItem.coordinate;
                    _geo2EnuCopy.coordinate.altitude = _missionItem.altitude.value;
                }
                var p2 = Qt.vector3d(_geo2EnuCopy.localCoordinate.x, _geo2EnuCopy.localCoordinate.y, _geo2EnuCopy.coordinate.altitude);

                missionPathModel.append({
                                            "x_1": p1.x,
                                            "y_1": p1.y,
                                            "z_1": p1.z,
                                            "x_2": p2.x,
                                            "y_2": p2.y,
                                            "z_2": p2.z,
                                            "color": (isReturnToLaunchItem(_missionItem))?("red"):("orange"),
                                        });
                _missionItemPrevious = _missionItem;
            }
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
            color: model.color
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

    Connections {
        target: _vehicle
        onHomePositionChanged: {
            if(_rtlActive){
                addMissionItemsToListModel()
                addSegmentToMissionPathModel()
            }
        }
    }
}
