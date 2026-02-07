#include "MissionFixtures.h"

#include "MissionController.h"
#include <QtCore/QLoggingCategory>
#include "SimpleMissionItem.h"

Q_STATIC_LOGGING_CATEGORY(MissionFixturesLog, "Test.MissionFixtures")

namespace TestFixtures {
namespace Mission {

void addWaypoints(MissionController* mc, int count, const QGeoCoordinate& startCoord)
{
    if (!mc) {
        qCWarning(MissionFixturesLog) << "addWaypoints: null MissionController";
        return;
    }

    const QList<QGeoCoordinate> coords = Coord::waypointPath(startCoord, count, 100.0, 45.0);
    int insertIndex = mc->visualItems()->count();

    for (const QGeoCoordinate& coord : coords) {
        mc->insertSimpleMissionItem(coord, insertIndex++);
    }
}

void addTakeoff(MissionController* mc, const QGeoCoordinate& coord, double altitude)
{
    if (!mc) {
        qCWarning(MissionFixturesLog) << "addTakeoff: null MissionController";
        return;
    }

    const int insertIndex = mc->visualItems()->count();
    mc->insertTakeoffItem(coord, insertIndex);

    // Set takeoff altitude if we can access the item
    if (insertIndex < mc->visualItems()->count()) {
        SimpleMissionItem* item = mc->visualItems()->value<SimpleMissionItem*>(insertIndex);
        if (item && item->altitude()) {
            item->altitude()->setRawValue(altitude);
        }
    }
}

void addLand(MissionController* mc, const QGeoCoordinate& coord)
{
    if (!mc) {
        qCWarning(MissionFixturesLog) << "addLand: null MissionController";
        return;
    }

    const int insertIndex = mc->visualItems()->count();
    mc->insertLandItem(coord, insertIndex);
}

void addTakeoffWaypointsLand(MissionController* mc, int waypointCount)
{
    if (!mc) {
        qCWarning(MissionFixturesLog) << "addTakeoffWaypointsLand: null MissionController";
        return;
    }

    const QGeoCoordinate start = Coord::zurich();
    addTakeoff(mc, start);
    addWaypoints(mc, waypointCount, start.atDistanceAndAzimuth(100, 0));

    // Land near the last waypoint
    const QGeoCoordinate landCoord = start.atDistanceAndAzimuth(100 * (waypointCount + 1), 45);
    addLand(mc, landCoord);
}

void addRTL(MissionController* mc)
{
    if (!mc) {
        qCWarning(MissionFixturesLog) << "addRTL: null MissionController";
        return;
    }

    const int insertIndex = mc->visualItems()->count();
    mc->insertROIMissionItem(QGeoCoordinate(), insertIndex);
}

void clear(MissionController* mc)
{
    if (!mc) {
        qCWarning(MissionFixturesLog) << "clear: null MissionController";
        return;
    }

    mc->removeAll();
}

}  // namespace Mission
}  // namespace TestFixtures
