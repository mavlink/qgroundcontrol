#include "waypointlineitem.h"

namespace mapcontrol
{
WaypointLineItem::WaypointLineItem(WayPointItem* wp1, WayPointItem* wp2, QColor color, mapcontrol::MapGraphicItem* map) :
        QGraphicsLineItem(map),
    wp1(wp1),
    wp2(wp2),
    map(map)
{
    // Make sure this stick to the map
    //this->setFlag(QGraphicsItem::Item,true);
    setParentItem(map);

    // Set up the pen for this icon with the UAV color
    QPen pen(color);
    pen.setWidth(2);
    setPen(pen);

    point1 = wp1->Coord();
    point2 = wp2->Coord();

    // Pixel coordinates of the local points
    core::Point localPoint1 = map->FromLatLngToLocal(wp1->Coord());
    core::Point localPoint2 = map->FromLatLngToLocal(wp2->Coord());
    // Draw line
    setLine(localPoint1.X(), localPoint1.Y(), localPoint2.X(), localPoint2.Y());

    // Connect updates
    // Update line from both waypoints
    connect(wp1, SIGNAL(WPValuesChanged(WayPointItem*)), this, SLOT(updateWPValues(WayPointItem*)));
    connect(wp2, SIGNAL(WPValuesChanged(WayPointItem*)), this, SLOT(updateWPValues(WayPointItem*)));
    // Delete line if one of the waypoints get deleted
    connect(wp1, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    connect(wp2, SIGNAL(destroyed()), this, SLOT(deleteLater()));

    // Map Zoom and move
    connect(map, SIGNAL(mapChanged()), this, SLOT(updateWPValues()));
}

void WaypointLineItem::RefreshPos()
{
    // Delete if either waypoint got deleted
    if (!wp1 || !wp2)
    {
        this->deleteLater();
    }
    else
    {
        // Set new pixel coordinates based on new global coordinates
        //QTimer::singleShot(0, this, SLOT(updateWPValues()));
        core::Point localPoint1 = map->FromLatLngToLocal(point1);
        core::Point localPoint2 = map->FromLatLngToLocal(point2);
        if (!localPoint1.IsEmpty() && !localPoint2.IsEmpty())
        {
            setLine(localPoint1.X(), localPoint1.Y(), localPoint2.X(), localPoint2.Y());
        }
    }
}

void WaypointLineItem::updateWPValues(WayPointItem* waypoint)
{
    Q_UNUSED(waypoint);
    // Delete if either waypoint got deleted
    if (!wp1 || !wp2)
    {
        this->deleteLater();
    }
    else
    {
        // Set new pixel coordinates based on new global coordinates
        point1 = wp1->Coord();
        point2 = wp2->Coord();
        core::Point localPoint1 = map->FromLatLngToLocal(wp1->Coord());
        core::Point localPoint2 = map->FromLatLngToLocal(wp2->Coord());

        setLine(localPoint1.X(), localPoint1.Y(), localPoint2.X(), localPoint2.Y());
    }
}

void WaypointLineItem::updateWPValues()
{
    updateWPValues(NULL);
}

int WaypointLineItem::type()const
{
    return Type;
}

}
