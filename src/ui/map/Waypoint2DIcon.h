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

    //! sets the QPen which is used for drawing the circle
    /*!
     * A QPen can be used to modify the look of the drawn circle
     * @param pen the QPen which should be used for drawing
     * @see http://doc.trolltech.com/4.3/qpen.html
     */
    virtual void setPen(QPen* pen);

    void SetHeading(float heading);

    /** @brief Rectangle to be updated on changes */
    QRectF boundingRect() const;
    /** @brief Draw the icon in a double buffer */
    void drawIcon();
    /** @brief Draw the icon on a QPainter device (map) */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /** @brief UNUSED FUNCTION: Waypoints in QGC are purely passive */
    void SetReached(const bool &value);

public:
    void updateWaypoint();

protected:
    mapcontrol::OPMapWidget* parent; ///< Parent widget
    int radius;           ///< Radius / diameter of the icon in pixels
    Waypoint* waypoint;   ///< Waypoint data container this icon represents
    QPen* mypen;
    QColor color;
    bool showAcceptanceRadius;
    bool showOrbit;
//    QSize size;

};

#endif // WAYPOINT2DICON_H
