#ifndef WAYPOINT2DICON_H
#define WAYPOINT2DICON_H

#include <QGraphicsItem>

#include "Waypoint.h"

class Waypoint2DIcon : public QGraphicsItem
{
public:
    /*!
     *
     * @param x longitude
     * @param y latitude
     * @param name name of the circle point
     * @param alignment alignment (Middle or TopLeft)
     * @param pen QPen for drawing
     */
    Waypoint2DIcon(qreal x, qreal y, QString name = QString(), Alignment alignment = Middle, QPen* pen=0);

    //!
    /*!
     *
     * @param x longitude
     * @param y latitude
     * @param radius the radius of the circle
     * @param name name of the circle point
     * @param alignment alignment (Middle or TopLeft)
     * @param pen QPen for drawing
     */
    Waypoint2DIcon(qreal x, qreal y, int radius = 20, QString name = QString(), Alignment alignment = Middle, QPen* pen=0);

    /**
     *
     * @param wp Waypoint
     * @param listIndex Index in the waypoint list
     * @param radius the radius of the circle
     * @param name name of the circle point
     * @param alignment alignment (Middle or TopLeft)
     * @param pen QPen for drawing
     */
    Waypoint2DIcon(Waypoint* wp, int listIndex, int radius = 20, Alignment alignment = Middle, QPen* pen=0);

    virtual ~Waypoint2DIcon();

    //! sets the QPen which is used for drawing the circle
    /*!
     * A QPen can be used to modify the look of the drawn circle
     * @param pen the QPen which should be used for drawing
     * @see http://doc.trolltech.com/4.3/qpen.html
     */
    virtual void setPen(QPen* pen);

    void setYaw(float yaw);

    void drawIcon(QPen* pen);

public slots:
    void updateWaypoint();

protected:
    float yaw;      ///< Yaw angle of the MAV
    int radius;     ///< Radius / diameter of the icon in pixels
    Waypoint* waypoint;   ///< Waypoint data container this icon represents

};

#endif // WAYPOINT2DICON_H
