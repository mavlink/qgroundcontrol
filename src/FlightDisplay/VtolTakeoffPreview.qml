import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtLocation                   5.3
import QtPositioning                5.3
import QtQuick.Dialogs              1.2
import QtQuick.Layouts              1.2
import QtQml                        2.0

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightMap     1.0

//import Custom.Widgets               1.0

Item {
    id: root

    property var flightMap
    property var _guidedActionsController
    property var    _loiterComponent: null
    property var    _lineComponent: null
    property var    activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property bool _takeoff_executed: false

    property var distHomeToLoiter : activeVehicle ? activeVehicle.loiterRadiusMeters * 3 : 0

    function destroyVisuals()
    {
        if ( _loiterComponent) {
            _loiterComponent.destroy()
        }

        if (_lineComponent) {
            _lineComponent.destroy()
        }
    }

    Component {
        id: lineComponent

        MapPolyline {
            id:         approachPath
            line.width: 3
            line.color: "orange"
            z:          QGroundControl.zOrderTrajectoryLines
            visible:    true

            function updatePath() {
                var dist_vehicle_center = activeVehicle.coordinate.distanceTo(vtLoiterCircle.center)
                var azimuth_vehicle_center = activeVehicle.coordinate.azimuthTo(vtLoiterCircle.center)
                approachPath.path = [ activeVehicle.coordinate,  activeVehicle.coordinate.atDistanceAndAzimuth(dist_vehicle_center - vtLoiterCircle.radius.rawValue, azimuth_vehicle_center)]
            }

            Connections {
                target: vtLoiterCircle

                onCenterChanged: {
                    updatePath()

                }
            }

            Component.onCompleted: {
                updatePath()
            }
        }
    }

    Component {
        id: loiterComponent

        QGCMapCircleVisuals {
            id: vtLoiterCircleVisuals

            mapControl:         flightMap
            mapCircle:          vtLoiterCircle
            borderWidth:        5
            radiusDragable:     false
            externalCenterCoordHandling : true

            readonly property real delta_heading : 45

            function wrap_pi_degrees(input) {
                while(input >= 180) {
                    input -= 2*180
                }

                while(input < -180) {
                    input += 2*180
                }

                return input
            }

            Timer {
                // used to periodically recalculate the loiter point in case the vehicle heading changed
                interval: 1000; running: !_takeoff_executed; repeat: true
                onTriggered: {
                    vtLoiterCircleVisuals.updateLoiter(null)
                }
            }

            function updateLoiter(new_coordinate) {

                if (new_coordinate === null) {
                    new_coordinate = vtLoiterCircle.center
                }
                distHomeToLoiter = new_coordinate.distanceTo(activeVehicle.homePosition)

                distHomeToLoiter = Math.max(distHomeToLoiter, 2*activeVehicle.loiterRadius)
                distHomeToLoiter = Math.min(distHomeToLoiter, 4 * activeVehicle.loiterRadius)

                var heading = wrap_pi_degrees(activeVehicle.homePosition.azimuthTo(new_coordinate))

                var heading_error = wrap_pi_degrees(heading-activeVehicle.heading.rawValue)

                if (heading_error < -delta_heading) {
                    vtLoiterCircle.center = activeVehicle.homePosition.atDistanceAndAzimuth(distHomeToLoiter, wrap_pi_degrees(activeVehicle.heading.rawValue - (delta_heading-1)))
                } else if (heading_error > delta_heading) {
                    vtLoiterCircle.center = activeVehicle.homePosition.atDistanceAndAzimuth(distHomeToLoiter, wrap_pi_degrees(activeVehicle.heading.rawValue + (delta_heading-1)))
                } else {
                    vtLoiterCircle.center = activeVehicle.homePosition.atDistanceAndAzimuth(distHomeToLoiter, heading)
                }

                _guidedActionsController.vtolTakeoffLoiterCoordinate = vtLoiterCircle.center
            }

            onCenterDragCoordChanged: {
                vtLoiterCircleVisuals.updateLoiter(coordinate)
            }
        }
    }

    Connections {
        target: activeVehicle ? activeVehicle.parameterManager : null
        onParametersReadyChanged: {
            if (parametersReady ) {
                vtLoiterCircle.radius.rawValue = activeVehicle.loiterRadius
            }
        }
    }

    QGCMapCircle {
        id:             vtLoiterCircle
        interactive:    true
        showRotation:   true
    }

    Connections {
        target: (activeVehicle && activeVehicle.vtol) ? activeVehicle : null

        onVtolInFwdFlightChanged: {
            if (activeVehicle.vtolInFwdFlight) {
                destroyVisuals()
            }
        }

        onVtolTakeoffResult: {
            _takeoff_executed = success
            if (success) {
                vtLoiterCircle.interactive = false
            } else {
                destroyVisuals()
            }
        }

        onFlightModeChanged: {
            if (_takeoff_executed && activeVehicle.flightMode !== activeVehicle.vtolTakeoffFlightMode) {
                // this happens if a user aborts the vtol takeoff by changing flight mode
                destroyVisuals()
            }
        }
    }

    Connections {
        target: _guidedActionsController

        onShowVtolTakeoffLoiter: {
            _loiterComponent = loiterComponent.createObject(flightMap)
            _lineComponent = lineComponent.createObject(flightMap)
            flightMap.addMapItem(_lineComponent)
            distHomeToLoiter = activeVehicle.loiterRadius * 3
            vtLoiterCircle.radius.rawValue = activeVehicle.loiterRadius
            vtLoiterCircle.center = activeVehicle.homePosition.atDistanceAndAzimuth(distHomeToLoiter, activeVehicle.heading.rawValue)
            _guidedActionsController.vtolTakeoffLoiterCoordinate = vtLoiterCircle.center
            vtLoiterCircle.interactive = true
            _takeoff_executed = false
        }

        onVtolTakeoffExecuted: {
            _takeoff_executed = true
        }

        onActionCancelled: {
            if (!_takeoff_executed) {
                destroyVisuals()
            }
        }
    }
}
