#include "MAV2DIcon.h"
#include <QPainter>

#include <qmath.h>

MAV2DIcon::MAV2DIcon(qreal x, qreal y, int radius, int type, const QColor& color, QString name, Alignment alignment, QPen* pen)
    : Point(x, y, name, alignment),
    yaw(0.0f),
    radius(radius),
    type(type),
    iconColor(color)
{
    size = QSize(radius, radius);
    drawIcon(pen);
}

MAV2DIcon::MAV2DIcon(qreal x, qreal y, QString name, Alignment alignment, QPen* pen)
    : Point(x, y, name, alignment),
    radius(20),
    type(0),
    iconColor(Qt::yellow)
{
    size = QSize(radius, radius);
    drawIcon(pen);
}

MAV2DIcon::~MAV2DIcon()
{
    delete mypixmap;
}

void MAV2DIcon::setPen(QPen* pen)
{
    mypen = pen;
    drawIcon(pen);
}

/**
 * Yaw changes of less than ±15 degrees are ignored.
 *
 * @param yaw in radians, 0 = north, positive = clockwise
 */
void MAV2DIcon::setYaw(float yaw)
{
    float diff = fabs(yaw - this->yaw);
    while (diff > M_PI)
    {
        diff -= M_PI;
    }

    if (diff > 0.25)
    {
        this->yaw = yaw;
        drawIcon(mypen);
    }
}

void MAV2DIcon::drawIcon(QPen* pen)
{
    Q_UNUSED(pen);
    switch (type)
    {
    case MAV_ICON_GENERIC:
    case MAV_ICON_AIRPLANE:
    case MAV_ICON_QUADROTOR:
    case MAV_ICON_ROTARY_WING:
    default:
        {
            mypixmap = new QPixmap(radius+1, radius+1);
            mypixmap->fill(Qt::transparent);
            QPainter painter(mypixmap);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::HighQualityAntialiasing);

            // Rotate by yaw
            painter.translate(radius/2, radius/2);
            painter.rotate(((yaw/(float)M_PI)+1.0f)*360.0f);

            // DRAW AIRPLANE

            qDebug() << "ICON SIZE:" << radius;

            float iconSize = radius*0.9f;
            QPolygonF poly(24);
            poly.replace(0, QPointF(0.000000f*iconSize, -0.312500f*iconSize));
            poly.replace(1, QPointF(0.025000f*iconSize, -0.287500f*iconSize));
            poly.replace(2, QPointF(0.037500f*iconSize, -0.237500f*iconSize));
            poly.replace(3, QPointF(0.031250f*iconSize, -0.187500f*iconSize));
            poly.replace(4, QPointF(0.025000f*iconSize, -0.043750f*iconSize));
            poly.replace(5, QPointF(0.500000f*iconSize, -0.025000f*iconSize));
            poly.replace(6, QPointF(0.500000f*iconSize, 0.025000f*iconSize));
            poly.replace(7, QPointF(0.025000f*iconSize, 0.043750f*iconSize));
            poly.replace(8, QPointF(0.025000f*iconSize, 0.162500f*iconSize));
            poly.replace(9, QPointF(0.137500f*iconSize, 0.181250f*iconSize));
            poly.replace(10, QPointF(0.137500f*iconSize, 0.218750f*iconSize));
            poly.replace(11, QPointF(0.025000f*iconSize, 0.206250f*iconSize));
            poly.replace(12, QPointF(-0.025000f*iconSize, 0.206250f*iconSize));
            poly.replace(13, QPointF(-0.137500f*iconSize, 0.218750f*iconSize));
            poly.replace(14, QPointF(-0.137500f*iconSize, 0.181250f*iconSize));
            poly.replace(15, QPointF(-0.025000f*iconSize, 0.162500f*iconSize));
            poly.replace(16, QPointF(-0.025000f*iconSize, 0.043750f*iconSize));
            poly.replace(17, QPointF(-0.500000f*iconSize, 0.025000f*iconSize));
            poly.replace(18, QPointF(-0.500000f*iconSize, -0.025000f*iconSize));
            poly.replace(19, QPointF(-0.025000f*iconSize, -0.043750f*iconSize));
            poly.replace(20, QPointF(-0.031250f*iconSize, -0.187500f*iconSize));
            poly.replace(21, QPointF(-0.037500f*iconSize, -0.237500f*iconSize));
            poly.replace(22, QPointF(-0.031250f*iconSize, -0.262500f*iconSize));
            poly.replace(23, QPointF(0.000000f*iconSize, -0.312500f*iconSize));

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
//            if (pen)
//            {
//                pen->setWidthF(2);
//                painter.setPen(*pen);
//            }
//            else
//            {
//                QPen pen2(Qt::red);
//                pen2.setWidth(0);
//                painter.setPen(pen2);
//            }
            painter.setBrush(QBrush(iconColor));

            painter.drawPolygon(poly);
        }
        break;
    }
}
