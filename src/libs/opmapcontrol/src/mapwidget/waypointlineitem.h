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
    enum { Type = UserType + 7 };
    WaypointLineItem(WayPointItem* wp1, WayPointItem* wp2, QColor color=QColor(Qt::red), MapGraphicItem* parent=0);
    int type() const;

public slots:
    /**
    * @brief Update waypoint values
    *
    * @param waypoint The waypoint object that just changed
    */
    void updateWPValues(WayPointItem* waypoint);
    /**
    * @brief Update waypoint values
    */
    void updateWPValues();
    /**
    * @brief Update waypoint values
    *
    */
    void RefreshPos();

protected:
    internals::PointLatLng point1;
    internals::PointLatLng point2;
    WayPointItem* wp1;
    WayPointItem* wp2;
    MapGraphicItem* map;              ///< The map this item is parent of
};
}

#endif // WAYPOINTLINEITEM_H
