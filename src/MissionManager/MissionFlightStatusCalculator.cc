#include "MissionFlightStatusCalculator.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "QmlObjectListModel.h"
#include "VisualMissionItem.h"
#include "SimpleMissionItem.h"
#include "ComplexMissionItem.h"
#include "MissionSettingsItem.h"
#include "AppSettings.h"
#include "PlanViewSettings.h"
#include "SettingsManager.h"

#include <QtMath>

void MissionFlightStatusCalculator::reset(Vehicle* controllerVehicle, Vehicle* managerVehicle, bool missionContainsVTOLTakeoff)
{
    _status.totalDistance =        0.0;
    _status.plannedDistance =      0.0;
    _status.maxTelemetryDistance = 0.0;
    _status.totalTime =            0.0;
    _status.hoverTime =            0.0;
    _status.cruiseTime =           0.0;
    _status.hoverDistance =        0.0;
    _status.cruiseDistance =       0.0;
    _status.cruiseSpeed =          controllerVehicle->defaultCruiseSpeed();
    _status.hoverSpeed =           controllerVehicle->defaultHoverSpeed();
    _status.vehicleSpeed =         controllerVehicle->multiRotor() || managerVehicle->vtol() ? _status.hoverSpeed : _status.cruiseSpeed;
    _status.vehicleYaw =           qQNaN();
    _status.gimbalYaw =            qQNaN();
    _status.gimbalPitch =          qQNaN();
    _status.mAhBattery =           0;
    _status.hoverAmps =            0;
    _status.cruiseAmps =           0;
    _status.ampMinutesAvailable =  0;
    _status.hoverAmpsTotal =       0;
    _status.cruiseAmpsTotal =      0;
    _status.batteryChangePoint =   -1;
    _status.batteriesRequired =    -1;
    _status.vtolMode =             missionContainsVTOLTakeoff ? QGCMAVLink::VehicleClassMultiRotor : QGCMAVLink::VehicleClassFixedWing;

    controllerVehicle->firmwarePlugin()->batteryConsumptionData(controllerVehicle, _status.mAhBattery, _status.hoverAmps, _status.cruiseAmps);
    if (_status.mAhBattery != 0) {
        double batteryPercentRemainingAnnounce = SettingsManager::instance()->appSettings()->batteryPercentRemainingAnnounce()->rawValue().toDouble();
        _status.ampMinutesAvailable = static_cast<double>(_status.mAhBattery) / 1000.0 * 60.0 * ((100.0 - batteryPercentRemainingAnnounce) / 100.0);
    }
}

void MissionFlightStatusCalculator::recalc(QmlObjectListModel* visualItems,
                                            MissionSettingsItem* settingsItem,
                                            Vehicle* controllerVehicle,
                                            Vehicle* managerVehicle,
                                            AppSettings* appSettings,
                                            PlanViewSettings* planViewSettings,
                                            bool missionContainsVTOLTakeoff)
{
    bool                firstCoordinateItem =           true;
    VisualMissionItem*  lastFlyThroughVI =   qobject_cast<VisualMissionItem*>(visualItems->get(0));

    bool homePositionValid = settingsItem->coordinate().isValid();

    // If home position is valid we can calculate distances between all waypoints.
    // If home position is not valid we can only calculate distances between waypoints which are
    // both relative altitude.

    // No values for first item
    lastFlyThroughVI->setAltDifference(0);
    lastFlyThroughVI->setAzimuth(0);
    lastFlyThroughVI->setDistance(0);
    lastFlyThroughVI->setDistanceFromStart(0);

    _minAMSLAltitude = _maxAMSLAltitude = qQNaN();

    reset(controllerVehicle, managerVehicle, missionContainsVTOLTakeoff);

    bool   linkStartToHome =            false;
    bool   foundRTL =                   false;
    bool   pastLandCommand =            false;
    double totalHorizontalDistance =    0;

    for (int i=0; i<visualItems->count(); i++) {
        VisualMissionItem*  item =          qobject_cast<VisualMissionItem*>(visualItems->get(i));
        SimpleMissionItem*  simpleItem =    qobject_cast<SimpleMissionItem*>(item);
        ComplexMissionItem* complexItem =   qobject_cast<ComplexMissionItem*>(item);

        if (simpleItem && simpleItem->mavCommand() == MAV_CMD_NAV_RETURN_TO_LAUNCH) {
            foundRTL = true;
        }

        // Assume the worst
        item->setAzimuth(0);
        item->setDistance(0);
        item->setDistanceFromStart(0);

        // Gimbal states reflect the state AFTER executing the item

        // ROI commands cancel out previous gimbal yaw/pitch
        if (simpleItem) {
            switch (simpleItem->command()) {
            case MAV_CMD_NAV_ROI:
            case MAV_CMD_DO_SET_ROI_LOCATION:
            case MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET:
            case MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW:
                _status.gimbalYaw      = qQNaN();
                _status.gimbalPitch    = qQNaN();
                break;
            default:
                break;
            }
        }

        // Look for specific gimbal changes
        double gimbalYaw = item->specifiedGimbalYaw();
        if (!qIsNaN(gimbalYaw) || planViewSettings->showGimbalOnlyWhenSet()->rawValue().toBool()) {
            _status.gimbalYaw = gimbalYaw;
        }
        double gimbalPitch = item->specifiedGimbalPitch();
        if (!qIsNaN(gimbalPitch) || planViewSettings->showGimbalOnlyWhenSet()->rawValue().toBool()) {
            _status.gimbalPitch = gimbalPitch;
        }

        // We don't need to do any more processing if:
        //  Mission Settings Item
        //  We are after an RTL command
        if (i != 0 && !foundRTL) {
            // We must set the mission flight status prior to querying for any values from the item. This is because things like
            // current speed, gimbal, vtol state  impact the values.
            item->setMissionFlightStatus(_status);

            // Link back to home if first item is takeoff and we have home position
            if (firstCoordinateItem && simpleItem && (simpleItem->mavCommand() == MAV_CMD_NAV_TAKEOFF || simpleItem->mavCommand() == MAV_CMD_NAV_VTOL_TAKEOFF)) {
                if (homePositionValid) {
                    linkStartToHome = true;
                    if (controllerVehicle->multiRotor() || controllerVehicle->vtol()) {
                        // We have to special case takeoff, assuming vehicle takes off straight up to specified altitude
                        double azimuth, distance, altDifference;
                        calcPrevWaypointValues(settingsItem, simpleItem, &azimuth, &distance, &altDifference);
                        double takeoffTime = qAbs(altDifference) / appSettings->offlineEditingAscentSpeed()->rawValue().toDouble();
                        _addHoverTime(takeoffTime, 0, -1);
                    }
                }
            }

            if (!pastLandCommand)
                _addTimeDistance(controllerVehicle, _status.vtolMode == QGCMAVLink::VehicleClassMultiRotor, 0, 0, item->additionalTimeDelay(), 0, -1);

            if (item->specifiesCoordinate()) {

                // Keep track of the min/max AMSL altitude for entire mission so we can calculate altitude percentages in terrain status display
                if (simpleItem) {
                    double amslAltitude = item->amslEntryAlt();
                    _minAMSLAltitude = std::fmin(_minAMSLAltitude, amslAltitude);
                    _maxAMSLAltitude = std::fmax(_maxAMSLAltitude, amslAltitude);
                } else {
                    // Complex item
                    double complexMinAMSLAltitude = complexItem->minAMSLAltitude();
                    double complexMaxAMSLAltitude = complexItem->maxAMSLAltitude();
                    _minAMSLAltitude = std::fmin(_minAMSLAltitude, complexMinAMSLAltitude);
                    _maxAMSLAltitude = std::fmax(_maxAMSLAltitude, complexMaxAMSLAltitude);
                }

                if (!item->isStandaloneCoordinate()) {
                    firstCoordinateItem = false;

                    // Update vehicle yaw assuming direction to next waypoint and/or mission item change
                    if (simpleItem) {
                        double newVehicleYaw = simpleItem->specifiedVehicleYaw();
                        if (qIsNaN(newVehicleYaw)) {
                            // No specific vehicle yaw set. Current vehicle yaw is determined from flight path segment direction.
                            if (simpleItem != lastFlyThroughVI) {
                                _status.vehicleYaw = lastFlyThroughVI->exitCoordinate().azimuthTo(simpleItem->entryCoordinate());
                            }
                        } else {
                            _status.vehicleYaw = newVehicleYaw;
                        }
                        simpleItem->setMissionVehicleYaw(_status.vehicleYaw);
                    }

                    if (lastFlyThroughVI != settingsItem || linkStartToHome) {
                        // This is a subsequent waypoint or we are forcing the first waypoint back to home
                        double azimuth, distance, altDifference;

                        calcPrevWaypointValues(item, lastFlyThroughVI, &azimuth, &distance, &altDifference);

                        // If the last waypoint was a land command, there's a discontinuity at this point
                        if (!lastFlyThroughVI->isLandCommand()) {
                            totalHorizontalDistance += distance;
                            item->setDistance(distance);

                            if (!pastLandCommand) {
                                // Calculate time/distance
                                double hoverTime = distance / _status.hoverSpeed;
                                double cruiseTime = distance / _status.cruiseSpeed;
                                _addTimeDistance(controllerVehicle, _status.vtolMode == QGCMAVLink::VehicleClassMultiRotor, hoverTime, cruiseTime, 0, distance, item->sequenceNumber());
                            }
                        }

                        item->setAltDifference(altDifference);
                        item->setAzimuth(azimuth);
                        item->setDistanceFromStart(totalHorizontalDistance);

                        _status.maxTelemetryDistance = qMax(_status.maxTelemetryDistance, calcDistanceToHome(item, settingsItem));
                    }

                    if (complexItem) {
                        // Add in distance/time inside complex items as well
                        double distance = complexItem->complexDistance();
                        _status.maxTelemetryDistance = qMax(_status.maxTelemetryDistance, complexItem->greatestDistanceTo(complexItem->exitCoordinate()));

                        if (!pastLandCommand) {
                            double hoverTime = distance / _status.hoverSpeed;
                            double cruiseTime = distance / _status.cruiseSpeed;
                            _addTimeDistance(controllerVehicle, _status.vtolMode == QGCMAVLink::VehicleClassMultiRotor, hoverTime, cruiseTime, 0, distance, item->sequenceNumber());
                        }

                        totalHorizontalDistance += distance;
                    }


                    lastFlyThroughVI = item;
                }
            }
        }

        // Speed, VTOL states changes are processed last since they take affect on the next item

        double newSpeed = item->specifiedFlightSpeed();
        if (!qIsNaN(newSpeed)) {
            if (controllerVehicle->multiRotor()) {
                _status.hoverSpeed = newSpeed;
            } else if (controllerVehicle->vtol()) {
                if (_status.vtolMode == QGCMAVLink::VehicleClassMultiRotor) {
                    _status.hoverSpeed = newSpeed;
                } else {
                    _status.cruiseSpeed = newSpeed;
                }
            } else {
                _status.cruiseSpeed = newSpeed;
            }
            _status.vehicleSpeed = newSpeed;
        }

        // Update VTOL state
        if (simpleItem && controllerVehicle->vtol()) {
            switch (simpleItem->command()) {
            case MAV_CMD_NAV_TAKEOFF:       // This will do a fixed wing style takeoff
            case MAV_CMD_NAV_VTOL_TAKEOFF:  // Vehicle goes straight up and then transitions to FW
            case MAV_CMD_NAV_LAND:
                _status.vtolMode = QGCMAVLink::VehicleClassFixedWing;
                break;
            case MAV_CMD_NAV_VTOL_LAND:
                _status.vtolMode = QGCMAVLink::VehicleClassMultiRotor;
                break;
            case MAV_CMD_DO_VTOL_TRANSITION:
            {
                int transitionState = simpleItem->missionItem().param1();
                if (transitionState == MAV_VTOL_STATE_MC) {
                    _status.vtolMode = QGCMAVLink::VehicleClassMultiRotor;
                } else if (transitionState == MAV_VTOL_STATE_FW) {
                    _status.vtolMode = QGCMAVLink::VehicleClassFixedWing;
                }
            }
                break;
            default:
                break;
            }
        }

        if (item->isLandCommand()) {
            pastLandCommand = true;
        }
    }
    lastFlyThroughVI->setMissionVehicleYaw(_status.vehicleYaw);

    // Add the information for the final segment back to home
    if (foundRTL && lastFlyThroughVI != settingsItem && homePositionValid) {
        double azimuth, distance, altDifference;
        calcPrevWaypointValues(lastFlyThroughVI, settingsItem, &azimuth, &distance, &altDifference);

        if (!pastLandCommand) {
            // Calculate time/distance
            double hoverTime = distance / _status.hoverSpeed;
            double cruiseTime = distance / _status.cruiseSpeed;
            double landTime = qAbs(altDifference) / appSettings->offlineEditingDescentSpeed()->rawValue().toDouble();
            _addTimeDistance(controllerVehicle, _status.vtolMode == QGCMAVLink::VehicleClassMultiRotor, hoverTime, cruiseTime, landTime, distance, -1);
        }
    }

    _status.totalDistance = totalHorizontalDistance;

    if (_status.mAhBattery != 0 && _status.batteryChangePoint == -1) {
        _status.batteryChangePoint = 0;
    }

    if (linkStartToHome) {
        // Home position is taken into account for min/max values
        _minAMSLAltitude = std::fmin(_minAMSLAltitude, settingsItem->plannedHomePositionAltitude()->rawValue().toDouble());
        _maxAMSLAltitude = std::fmax(_maxAMSLAltitude, settingsItem->plannedHomePositionAltitude()->rawValue().toDouble());
    }

    // Walk the list calculating altitude percentages
    double altRange = _maxAMSLAltitude - _minAMSLAltitude;
    for (int i=0; i<visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(visualItems->get(i));

        if (item->specifiesCoordinate()) {
            double amslAlt = item->amslEntryAlt();
            if (altRange == 0.0) {
                item->setAltPercent(0.0);
                item->setTerrainPercent(qQNaN());
                item->setTerrainCollision(false);
            } else {
                item->setAltPercent((amslAlt - _minAMSLAltitude) / altRange);
                double terrainAltitude = item->terrainAltitude();
                if (qIsNaN(terrainAltitude)) {
                    item->setTerrainPercent(qQNaN());
                    item->setTerrainCollision(false);
                } else {
                    item->setTerrainPercent((terrainAltitude - _minAMSLAltitude) / altRange);
                    item->setTerrainCollision(amslAlt < terrainAltitude);
                }
            }
        }
    }
}

void MissionFlightStatusCalculator::calcPrevWaypointValues(VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference)
{
    QGeoCoordinate  currentCoord =  currentItem->entryCoordinate();
    QGeoCoordinate  prevCoord =     prevItem->exitCoordinate();

    *altDifference = currentItem->amslEntryAlt() - prevItem->amslExitAlt();
    *distance = prevCoord.distanceTo(currentCoord);
    *azimuth = prevCoord.azimuthTo(currentCoord);
}

double MissionFlightStatusCalculator::calcDistanceToHome(VisualMissionItem* currentItem, VisualMissionItem* homeItem)
{
    QGeoCoordinate  currentCoord =  currentItem->entryCoordinate();
    QGeoCoordinate  homeCoord =     homeItem->exitCoordinate();

    return homeCoord.distanceTo(currentCoord);
}

void MissionFlightStatusCalculator::_updateBatteryInfo(int waypointIndex)
{
    if (_status.mAhBattery != 0) {
        _status.hoverAmpsTotal = (_status.hoverTime / 60.0) * _status.hoverAmps;
        _status.cruiseAmpsTotal = (_status.cruiseTime / 60.0) * _status.cruiseAmps;
        _status.batteriesRequired = ceil((_status.hoverAmpsTotal + _status.cruiseAmpsTotal) / _status.ampMinutesAvailable);
        if (waypointIndex != -1 && _status.batteriesRequired == 2 && _status.batteryChangePoint == -1) {
            _status.batteryChangePoint = waypointIndex - 1;
        }
    }
}

void MissionFlightStatusCalculator::_addHoverTime(double hoverTime, double hoverDistance, int waypointIndex)
{
    _status.totalTime += hoverTime;
    _status.hoverTime += hoverTime;
    _status.hoverDistance += hoverDistance;
    _status.plannedDistance += hoverDistance;
    _updateBatteryInfo(waypointIndex);
}

void MissionFlightStatusCalculator::_addCruiseTime(double cruiseTime, double cruiseDistance, int waypointIndex)
{
    _status.totalTime += cruiseTime;
    _status.cruiseTime += cruiseTime;
    _status.cruiseDistance += cruiseDistance;
    _status.plannedDistance += cruiseDistance;
    _updateBatteryInfo(waypointIndex);
}

void MissionFlightStatusCalculator::_addTimeDistance(Vehicle* controllerVehicle, bool vtolInHover, double hoverTime, double cruiseTime, double extraTime, double distance, int seqNum)
{
    if (controllerVehicle->vtol()) {
        if (vtolInHover) {
            _addHoverTime(hoverTime, distance, seqNum);
            _addHoverTime(extraTime, 0, -1);
        } else {
            _addCruiseTime(cruiseTime, distance, seqNum);
            _addCruiseTime(extraTime, 0, -1);
        }
    } else {
        if (controllerVehicle->multiRotor()) {
            _addHoverTime(hoverTime, distance, seqNum);
            _addHoverTime(extraTime, 0, -1);
        } else {
            _addCruiseTime(cruiseTime, distance, seqNum);
            _addCruiseTime(extraTime, 0, -1);
        }
    }
}
