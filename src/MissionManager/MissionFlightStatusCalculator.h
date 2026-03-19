#pragma once

#include "MissionFlightStatus.h"

class AppSettings;
class ComplexMissionItem;
class MissionSettingsItem;
class PlanViewSettings;
class QmlObjectListModel;
class SimpleMissionItem;
class Vehicle;
class VisualMissionItem;

/// Computes mission flight status (distances, times, battery, altitude range)
/// from a list of visual mission items and vehicle properties.
/// Extracted from MissionController to reduce its complexity.
class MissionFlightStatusCalculator
{
public:
    /// Resets the flight status fields to defaults based on vehicle properties.
    void reset(Vehicle* controllerVehicle, Vehicle* managerVehicle, bool missionContainsVTOLTakeoff);

    /// Runs the full recalculation over all visual items, updating per-item
    /// display properties and computing aggregate flight statistics.
    void recalc(QmlObjectListModel* visualItems,
                MissionSettingsItem* settingsItem,
                Vehicle* controllerVehicle,
                Vehicle* managerVehicle,
                AppSettings* appSettings,
                PlanViewSettings* planViewSettings,
                bool missionContainsVTOLTakeoff);

    const MissionFlightStatus_t& status() const { return _status; }
    double minAMSLAltitude() const { return _minAMSLAltitude; }
    double maxAMSLAltitude() const { return _maxAMSLAltitude; }

    static void calcPrevWaypointValues(VisualMissionItem* currentItem, VisualMissionItem* prevItem,
                                       double* azimuth, double* distance, double* altDifference);
    static double calcDistanceToHome(VisualMissionItem* currentItem, VisualMissionItem* homeItem);

private:
    void _updateBatteryInfo(int waypointIndex);
    void _addHoverTime(double hoverTime, double hoverDistance, int waypointIndex);
    void _addCruiseTime(double cruiseTime, double cruiseDistance, int waypointIndex);
    void _addTimeDistance(Vehicle* controllerVehicle, bool vtolInHover,
                          double hoverTime, double cruiseTime, double extraTime,
                          double distance, int seqNum);

    MissionFlightStatus_t _status {};
    double _minAMSLAltitude = 0;
    double _maxAMSLAltitude = 0;
};
