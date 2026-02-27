#pragma once

#include "CoordFixtures.h"

class MissionController;

/// @file
/// @brief Pre-built mission structures for testing

namespace TestFixtures {
namespace Mission {

/// Add simple waypoints to a mission controller
/// @param mc Mission controller to add waypoints to
/// @param count Number of waypoints to add
/// @param startCoord Starting coordinate (subsequent waypoints offset from here)
void addWaypoints(MissionController* mc, int count, const QGeoCoordinate& startCoord = Coord::zurich());

/// Add a takeoff item at the specified coordinate
/// @param mc Mission controller
/// @param coord Takeoff coordinate
/// @param altitude Takeoff altitude in meters
void addTakeoff(MissionController* mc, const QGeoCoordinate& coord, double altitude = 50.0);

/// Add a land item at the specified coordinate
/// @param mc Mission controller
/// @param coord Landing coordinate
void addLand(MissionController* mc, const QGeoCoordinate& coord);

/// Add a complete takeoff-waypoints-land sequence
/// @param mc Mission controller
/// @param waypointCount Number of waypoints between takeoff and land
void addTakeoffWaypointsLand(MissionController* mc, int waypointCount = 3);

/// Add a return-to-launch item
/// @param mc Mission controller
void addRTL(MissionController* mc);

/// Remove all mission items (reset to empty)
/// @param mc Mission controller
void clear(MissionController* mc);

}  // namespace Mission
}  // namespace TestFixtures
