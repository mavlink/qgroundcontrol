#include "MAV2DIcon.h"
#include <QPainter>

MAV2DIcon::MAV2DIcon(qreal x, qreal y, int radius, QString name, Alignment alignment, QPen* pen)
    : Point(x, y, name, alignment),
    yaw(0.0f)
{
    size = QSize(radius, radius);
    drawIcon(pen);
}

MAV2DIcon::MAV2DIcon(qreal x, qreal y, QString name, Alignment alignment, QPen* pen)
    : Point(x, y, name, alignment)
{
    int radius = 10;
    size = QSize(radius, radius);
    if (pen)
    {
        drawIcon(pen);
    }
}

MAV2DIcon::~MAV2DIcon()
{
    delete mypixmap;
}

void MAV2DIcon::setPen(QPen* pen)
{
    if (pen)
    {
        mypen = pen;
        drawIcon(pen);
    }
}

/**
 * @param yaw in radians, 0 = north, positive = clockwise
 */
void MAV2DIcon::setYaw(float yaw)
{
    this->yaw = yaw;
}

void MAV2DIcon::drawIcon(QPen* pen)
{
    if (pen)
    {
        mypixmap = new QPixmap(radius+1, radius+1);
        //mypixmap->fill(Qt::transparent);
        QPainter painter(mypixmap);

        // DRAW MICRO AIR VEHICLE
        QPointF p(radius/2, radius/2);

        float waypointSize = radius;
        QPolygonF poly(3);
        // Top point
        poly.replace(0, QPointF(p.x(), p.y()+waypointSize/2.0f));
        // Right point
        poly.replace(1, QPointF(p.x()-waypointSize/2.0f, p.y()-waypointSize/2.0f));
        // Left point
        poly.replace(2, QPointF(p.x()+waypointSize/2.0f, p.y() + waypointSize/2.0f));

        //    // Select color based on if this is the current waypoint
        //    if (list.at(i)->getCurrent())
        //    {
        //        color = QGC::colorCyan;//uas->getColor();
        //        pen.setWidthF(refLineWidthToPen(0.8f));
        //    }
        //    else
        //    {
        //        color = uas->getColor();
        //        pen.setWidthF(refLineWidthToPen(0.4f));
        //    }

        //pen.setColor(color);
        if (pen)
        {
            pen->setWidthF(2);
            painter.setPen(*pen);
        }
        else
        {
            QPen pen2(Qt::yellow);
            pen2.setWidth(2);
            painter.setPen(pen2);
        }
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(poly);
    }
}
