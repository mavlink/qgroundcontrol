#include "WaypointGlobal.h"

#include <QPointF>

WaypointGlobal::WaypointGlobal(const QPointF coordinate):
         Waypoint(id, x, y, z, yaw, autocontinue, current, orbit, holdTime)
{
    coordinateWP = coordinate;

}
