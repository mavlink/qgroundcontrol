#ifndef WAYPOINT2DICON_H
#define WAYPOINT2DICON_H

#include <QGraphicsItem>

#include "Waypoint.h"
#include "opmapcontrol.h"

class Waypoint2DIcon : public mapcontrol::WayPointItem
{
public:
    /**
     *
     * @param latitude
     * @param longitude
     * @param name name of the circle point
     */
    Waypoint2DIcon(mapcontrol::MapGraphicItem* map, mapcontrol::OPMapWidget* parent, qreal latitude, qreal longitude, qreal altitude, int listindex, QString name = QString(), QString description = QString(), int radius=30);

    /**
     *
     * @param wp Waypoint
     * @param radius the radius of the circle
     */
    Waypoint2DIcon(mapcontrol::MapGraphicItem* map, mapcontrol::OPMapWidget* parent, Waypoint* wp, const QColor& color, int listindex, int radius = 31);

    virtual ~Waypoint2DIcon();

    void SetHeading(float heading);

    /** @brief Rectangle to be updated on changes */
    QRectF boundingRect() const;
    /** @brief Draw the icon in a double buffer */
    void drawIcon();
    /** @brief Draw the icon on a QPainter device (map) */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /** @brief Enable and format the waypoint number display */
    void SetShowNumber(const bool &value);

public:
    void updateWaypoint();

protected:
    mapcontrol::OPMapWidget* parent; ///< Parent widget
    Waypoint* waypoint;   ///< Waypoint data container this icon represents
    int radius;           ///< Radius / diameter of the icon in pixels
    bool showAcceptanceRadius;
    bool showOrbit;
    QColor color;
//    QSize size;

};

#endif // WAYPOINT2DICON_H
