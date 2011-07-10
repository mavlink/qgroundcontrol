#ifndef WAYPOINTLINEITEM_H
#define WAYPOINTLINEITEM_H

#include <QGraphicsLineItem>
#include "opmapcontrol.h"

namespace mapcontrol {
class WaypointLineItem : public QObject,public QGraphicsLineItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    WaypointLineItem(WayPointItem* wp1, WayPointItem* wp2, QColor color=QColor(Qt::red), MapGraphicItem* parent=0);

public slots:
    /**
    * @brief Update waypoint values
    *
    * @param waypoint The waypoint object that just changed
    */
    void updateWPValues(WayPointItem* waypoint);

protected:
    internals::PointLatLng point1;
    internals::PointLatLng point2;
    WayPointItem* wp1;
    WayPointItem* wp2;
    MapGraphicItem* map;              ///< The map this item is parent of
};
}

#endif // WAYPOINTLINEITEM_H
