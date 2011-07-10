#include "waypointlineitem.h"

namespace mapcontrol
{
WaypointLineItem::WaypointLineItem(WayPointItem* wp1, WayPointItem* wp2, QColor color, mapcontrol::MapGraphicItem* map) :
    wp1(wp1),
    wp2(wp2),
    map(map)
{
    core::Point localPoint1 = map->FromLatLngToLocal(wp1->Coord());
    core::Point localPoint2 = map->FromLatLngToLocal(wp2->Coord());

    QPen pen(color);
    pen.setWidth(2);
    setPen(pen);

    setLine(localPoint1.X(), localPoint1.Y(), localPoint2.X(), localPoint2.Y());

    // Update line from both waypoints
    connect(wp1, SIGNAL(WPValuesChanged(WayPointItem*)), this, SLOT(updateWPValues(WayPointItem*)));
    connect(wp2, SIGNAL(WPValuesChanged(WayPointItem*)), this, SLOT(updateWPValues(WayPointItem*)));
    // Delete line if one of the waypoints get deleted
    connect(wp1, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    connect(wp2, SIGNAL(destroyed()), this, SLOT(deleteLater()));
}

void WaypointLineItem::updateWPValues(WayPointItem* waypoint)
{
    Q_UNUSED(waypoint);
    if (!wp1 || !wp2)
    {
        this->deleteLater();
    }
    else
    {
        core::Point localPoint1 = map->FromLatLngToLocal(wp1->Coord());
        core::Point localPoint2 = map->FromLatLngToLocal(wp2->Coord());

        setLine(localPoint1.X(), localPoint1.Y(), localPoint2.X(), localPoint2.Y());
    }
}

}
