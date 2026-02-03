import QGroundControl
import QGroundControl.Controls
import QtQuick3D
import QtQuick3D.Helpers

Node {
    id: vehicle3DBody

    property var _altitudeBias: _viewer3DSetting.altitudeBias.rawValue
    property var _backendQml: null
    property Node _camera: null
    property var _missionController: (_planMasterController) ? (_planMasterController.missionController) : (null)
    property var _planMasterController: null
    property bool _rtlActive: false
    property var _vehicle: null
    property var _viewer3DSetting: QGroundControl.settingsManager.viewer3DSettings
    property alias waypointConeModel: _waypointConeModel
    property alias waypointInstancing: waypointInstancing

    function addMissionItemsToListModel() {
        missionWaypointListModel.clear();
        waypointInstancing.selectedIndex = -1;
        waypointInstancing.clear();
        var _geo2EnuCopy = geo2Enu;
        var launchItemCoordinate = null;
        var _missionItemPrevious = null;
        _rtlActive = false;

        for (var i = 0; i < _missionController.visualItems.count; i++) {
            var _missionItem = _missionController.visualItems.get(i);
            if (isLaunchItem(_missionItem)) {
                launchItemCoordinate = _missionItem.coordinate;
                continue;
            }
            if (isItemAcceptable(_missionItem)) {
                if (isReturnToLaunchItem(_missionItem)) {
                    _rtlActive = true;
                    _geo2EnuCopy.coordinate = (_vehicle.homePosition) ? (_vehicle.homePosition) : (_missionItem.coordinate);
                    _geo2EnuCopy.coordinate.altitude = _missionItemPrevious.altitude.value;
                } else {
                    _geo2EnuCopy.coordinate = _missionItem.coordinate;
                    _geo2EnuCopy.coordinate.altitude = _missionItem.altitude.value;
                }
                var name = getItemName(_missionItem);
                var lx = _geo2EnuCopy.localCoordinate.x;
                var ly = _geo2EnuCopy.localCoordinate.y;
                var lz = _geo2EnuCopy.coordinate.altitude;
                missionWaypointListModel.append({
                    "x": lx,
                    "y": ly,
                    "z": lz,
                    "itemName": name,
                    "index": _missionItem.sequenceNumber
                });
                waypointInstancing.addEntry(Qt.vector3d(lx * 10, ly * 10, (lz + _altitudeBias) * 10), Qt.vector3d(0.1, 0.1, 0.1), Quaternion.fromEulerAngles(Qt.vector3d(90, 0, 0)), waypointColor(name));
                if (name === "L" || name === "W") {
                    _missionItemPrevious = _missionItem;
                }
            }
        }
    }

    function addSegmentToMissionPathModel() {
        pathInstancing.clear();
        var _geo2EnuCopy = geo2Enu;

        var _missionItem;
        var _missionItemPrevious = null;
        var launchItemCoordinate;

        for (var i = 0; i < _missionController.visualItems.count; i++) {
            _missionItem = _missionController.visualItems.get(i);
            if (isLaunchItem(_missionItem)) {
                launchItemCoordinate = _missionItem.coordinate;
                continue;
            }
            if (_missionItem.isTakeoffItem) {
                _missionItemPrevious = _missionItem;
                continue;
            }
            if (_missionItemPrevious === null) {
                continue;
            }
            if (getItemName(_missionItem) === "L" || getItemName(_missionItem) === "W") {
                _geo2EnuCopy.coordinate = _missionItemPrevious.coordinate;
                _geo2EnuCopy.coordinate.altitude = _missionItemPrevious.altitude.value;
                var p1 = Qt.vector3d(_geo2EnuCopy.localCoordinate.x * 10, _geo2EnuCopy.localCoordinate.y * 10, (_geo2EnuCopy.coordinate.altitude + _altitudeBias) * 10);

                if (isReturnToLaunchItem(_missionItem)) {
                    _geo2EnuCopy.coordinate = (_vehicle.homePosition) ? (_vehicle.homePosition) : (_missionItem.coordinate);
                    _geo2EnuCopy.coordinate.altitude = _missionItemPrevious.altitude.value;
                } else {
                    _geo2EnuCopy.coordinate = _missionItem.coordinate;
                    _geo2EnuCopy.coordinate.altitude = _missionItem.altitude.value;
                }
                var p2 = Qt.vector3d(_geo2EnuCopy.localCoordinate.x * 10, _geo2EnuCopy.localCoordinate.y * 10, (_geo2EnuCopy.coordinate.altitude + _altitudeBias) * 10);

                var color = isReturnToLaunchItem(_missionItem) ? "red" : "orange";
                pathInstancing.addLineSegment(p1, p2, 8, color);
                _missionItemPrevious = _missionItem;
            }
        }
    }

    function getItemName(missionItem) {
        if (isLaunchItem(missionItem) || isReturnToLaunchItem(missionItem)) {
            return qsTr("L");
        }

        if (missionItem.isTakeoffItem) {
            return qsTr("T"); //Takeoff
        }

        if (missionItem.specifiesCoordinate) {
            switch (missionItem.command) {
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
        return qsTr("null");
    }

    function isItemAcceptable(missionItem) {
        const acceptableCmdIds = [16 // Waypoint
            , 20 // Return To Launch
            , 22 //Takeoff
            , 195 // ROI
            , 201 // ROI DEPRECATED
            ,]; // based on MavCmdInfoCommon.json file
        return acceptableCmdIds.includes(missionItem.command);
    }

    function isLaunchItem(missionItem) {
        return missionItem.abbreviation === "L";
    }

    function isReturnToLaunchItem(missionItem) {
        return missionItem.command === 20;
    }

    function waypointColor(itemName) {
        if (itemName === "T")
            return "green";
        if (itemName === "R")
            return "red";
        if (itemName === "L")
            return "orange";
        return "black";
    }

    Viewer3DGeoCoordinateType {
        id: geo2Enu

        gpsRef: _backendQml.gpsRef
    }

    ListModel {
        id: missionWaypointListModel

    }

    DroneModelDjiF450 {
        id: droneDji3DModel

        altitudeBias: _altitudeBias
        gpsRef: _backendQml.gpsRef
        modelScale: Qt.vector3d(0.05, 0.05, 0.05)
        vehicle: _vehicle
    }

    Viewer3DInstancing {
        id: waypointInstancing

    }

    Model {
        id: _waypointConeModel

        instancing: waypointInstancing
        instancingLodMax: 5000
        opacity: 0.8
        pickable: true
        source: "#Cone"

        materials: DefaultMaterial {
            diffuseColor: "white"
        }
    }

    Repeater3D {
        model: missionWaypointListModel

        delegate: Node {
            position: Qt.vector3d(model.x * 10, model.y * 10, (model.z + _altitudeBias) * 10)

            LookAtNode {
                position.x: -6
                position.z: 30
                target: _camera

                QGCLabel {
                    color: "black"
                    font.pixelSize: 20
                    text: (model.itemName === "W") ? String(model.index) : model.itemName
                }
            }
        }
    }

    Viewer3DInstancing {
        id: pathInstancing

    }

    Model {
        instancing: pathInstancing
        instancingLodMax: 8000
        source: "#Cylinder"

        materials: DefaultMaterial {
            diffuseColor: "white"
        }
    }

    Connections {
        function onVisualItemsChanged() {
            addMissionItemsToListModel();
            addSegmentToMissionPathModel();
        }

        enabled: target !== null
        target: _missionController
    }

    Connections {
        function onGpsRefChanged() {
            addMissionItemsToListModel();
            addSegmentToMissionPathModel();
        }

        enabled: target !== null
        target: _backendQml
    }

    Connections {
        function onHomePositionChanged() {
            if (_rtlActive) {
                addMissionItemsToListModel();
                addSegmentToMissionPathModel();
            }
        }

        enabled: target !== null
        target: _vehicle
    }
}
