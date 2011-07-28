#include "MAV2DIcon.h"
#include <QPainter>

#include <qmath.h>

#include "QGC.h"

MAV2DIcon::MAV2DIcon(mapcontrol::MapGraphicItem* map,mapcontrol::OPMapWidget* parent, UASInterface* uas, int radius, int type)
    : UAVItem(map,parent),
    yaw(0.0f),
    radius(radius),
    type(type),
    airframe(uas->getAirframe()),
    iconColor(uas->getColor()),
    selected(uas->getSelected()),
    uasid(uas->getUASID())
{
    size = QSize(radius, radius);
    pic = QPixmap(size);
    drawIcon();
}

MAV2DIcon::MAV2DIcon(mapcontrol::MapGraphicItem* map, mapcontrol::OPMapWidget* parent, qreal lat, qreal lon, qreal alt, QColor color)
    : UAVItem(map,parent),
    radius(20),
    type(0),
    airframe(0),
    iconColor(color),
    selected(false),
    uasid(0)
{
    size = QSize(radius, radius);
    pic = QPixmap(size);
    drawIcon();
    SetUAVPos(internals::PointLatLng(lat, lon), alt, color);
}

MAV2DIcon::~MAV2DIcon()
{
    //delete pic;
}

void MAV2DIcon::setSelectedUAS(bool selected)
{
    this->selected = selected;
    drawIcon();
}

/**
 * Yaw changes of less than ±15 degrees are ignored.
 *
 * @param yaw in radians, 0 = north, positive = clockwise
 */
void MAV2DIcon::setYaw(float yaw)
{
    //// qDebug() << "MAV2Icon" << yaw;
    float diff = fabs(yaw - this->yaw);
    while (diff > (float)M_PI) {
        diff -= (float)M_PI;
    }

    if (diff > 0.1f) {
        this->yaw = yaw;
        drawIcon();
        // FIXME
    }
}

void MAV2DIcon::drawIcon()
{
    pic.fill(Qt::transparent);
    QPainter painter(&pic);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    radius = qMin(pic.width(), pic.height());

    // Rotate by yaw
    painter.translate(radius/2, radius/2);

    // Draw selected indicator
    painter.setBrush(Qt::NoBrush);
    if (selected) {
        QPen pen(Qt::yellow);
        pen.setWidth(2);
        painter.setPen(pen);
    }
    else
    {
        QPen pen(Qt::white);
        pen.setWidth(1);
        painter.setPen(pen);
    }
    painter.drawEllipse(QPoint(0, 0), radius/2-1, radius/2-1);
    drawAirframePolygon(airframe, painter, radius, iconColor, yaw);
}

void MAV2DIcon::drawAirframePolygon(int airframe, QPainter& painter, int radius, QColor& iconColor, float yaw)
{
    switch (airframe) {
    case UASInterface::QGC_AIRFRAME_PREDATOR:
    case UASInterface::QGC_AIRFRAME_REAPER: {
            // DRAW AIRPLANE

            // Rotate 180 degs more since the icon is oriented downwards
            //float yawRotate = (yaw/(float)M_PI)*180.0f + 180.0f+180.0f;

            const float yawDeg = ((yaw/M_PI)*180.0f)+180.0f+180.0f;
            int yawRotate = static_cast<int>(yawDeg) % 360;

            painter.rotate(yawRotate);

            //// qDebug() << "ICON SIZE:" << radius;

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

            painter.setBrush(QBrush(iconColor));
            QPen iconPen(Qt::black);
            iconPen.setWidthF(1.0f);
            painter.setPen(iconPen);

            painter.drawPolygon(poly);
            painter.rotate(-yawRotate);
        }
        break;
    case UASInterface::QGC_AIRFRAME_MIKROKOPTER:
    case UASInterface::QGC_AIRFRAME_CHEETAH: {
            // QUADROTOR
            float iconSize = radius*0.9f;
            const float yawDeg = ((yaw/M_PI)*180.0f)+180.0f;
            int yawRotate = static_cast<int>(yawDeg) % 360;

            painter.rotate(yawRotate);

            //// qDebug() << "ICON SIZE:" << radius;

            QPointF front(0, 0.2);
            front = front *iconSize;
            QPointF left(-0.2, 0);
            left = left * iconSize;
            QPointF right(0.2, 0.0);
            right *= iconSize;
            QPointF back(0, -0.2);
            back *= iconSize;

            QPolygonF poly(0);



            painter.setBrush(QBrush(iconColor));
            QPen iconPen(Qt::black);
            iconPen.setWidthF(1.0f);
            painter.setPen(iconPen);

            painter.drawPolygon(poly);

            painter.drawEllipse(left, radius/4/2, radius/4/2);
            painter.drawEllipse(right, radius/4/2, radius/4/2);
            painter.drawEllipse(back, radius/4/2, radius/4/2);

            painter.setBrush(Qt::red);
            painter.drawEllipse(front, radius/4/2, radius/4/2);
            painter.rotate(-yawRotate);
        }
        break;
    case UASInterface::QGC_AIRFRAME_EASYSTAR:
    case UASInterface::QGC_AIRFRAME_MERLIN:
    case UASInterface::QGC_AIRFRAME_TWINSTAR:
    case UASInterface::QGC_AIRFRAME_PTERYX: {
            // DRAW AIRPLANE

            const float yawDeg = ((yaw/M_PI)*180.0f)+180.0f;
            int yawRotate = static_cast<int>(yawDeg) % 360;
            painter.rotate(yawRotate);

            //// qDebug() << "ICON SIZE:" << radius;

            float iconSize = radius*0.7f;

            QPolygonF poly(32);
            poly.replace(0, QPointF(0.000000f*iconSize, 0.362319f*iconSize));
            poly.replace(1, QPointF(0.018116f*iconSize, 0.340580f*iconSize));
            poly.replace(2, QPointF(0.028986f*iconSize, 0.318841f*iconSize));
            poly.replace(3, QPointF(0.036232f*iconSize, 0.166667f*iconSize));
            poly.replace(4, QPointF(0.326087f*iconSize, 0.170290f*iconSize));
            poly.replace(5, QPointF(0.420290f*iconSize, 0.130435f*iconSize));
            poly.replace(6, QPointF(0.456522f*iconSize, 0.108696f*iconSize));
            poly.replace(7, QPointF(0.500000f*iconSize, 0.000000f*iconSize));
            poly.replace(8, QPointF(0.456522f*iconSize, 0.021739f*iconSize));
            poly.replace(9, QPointF(0.391304f*iconSize, 0.021739f*iconSize));
            poly.replace(10, QPointF(0.028986f*iconSize, 0.021739f*iconSize));
            poly.replace(11, QPointF(0.021739f*iconSize, -0.239130f*iconSize));
            poly.replace(12, QPointF(0.094203f*iconSize, -0.246377f*iconSize));
            poly.replace(13, QPointF(0.123188f*iconSize, -0.268116f*iconSize));
            poly.replace(14, QPointF(0.144928f*iconSize, -0.304348f*iconSize));
            poly.replace(15, QPointF(0.000000f*iconSize, -0.307971f*iconSize));
            poly.replace(16, QPointF(0.000000f*iconSize, -0.307971f*iconSize));
            poly.replace(17, QPointF(-0.144928f*iconSize, -0.304348f*iconSize));
            poly.replace(18, QPointF(-0.123188f*iconSize, -0.268116f*iconSize));
            poly.replace(19, QPointF(-0.094203f*iconSize, -0.246377f*iconSize));
            poly.replace(20, QPointF(-0.021739f*iconSize, -0.239130f*iconSize));
            poly.replace(21, QPointF(-0.028986f*iconSize, 0.021739f*iconSize));
            poly.replace(22, QPointF(-0.391304f*iconSize, 0.021739f*iconSize));
            poly.replace(23, QPointF(-0.456522f*iconSize, 0.021739f*iconSize));
            poly.replace(24, QPointF(-0.500000f*iconSize, 0.000000f*iconSize));
            poly.replace(25, QPointF(-0.456522f*iconSize, 0.108696f*iconSize));
            poly.replace(26, QPointF(-0.420290f*iconSize, 0.130435f*iconSize));
            poly.replace(27, QPointF(-0.326087f*iconSize, 0.170290f*iconSize));
            poly.replace(28, QPointF(-0.036232f*iconSize, 0.166667f*iconSize));
            poly.replace(29, QPointF(-0.028986f*iconSize, 0.318841f*iconSize));
            poly.replace(30, QPointF(-0.018116f*iconSize, 0.340580f*iconSize));
            poly.replace(31, QPointF(-0.000000f*iconSize, 0.362319f*iconSize));

            painter.setBrush(QBrush(iconColor));
            QPen iconPen(Qt::black);
            iconPen.setWidthF(1.0f);
            painter.setPen(iconPen);

            painter.drawPolygon(poly);
            painter.rotate(-yawRotate);
        }
        break;
    case UASInterface::QGC_AIRFRAME_GENERIC:
    default: {
            // GENERIC

            float yawRotate = (yaw/(float)M_PI)*180.0f + 180.0f;

            painter.rotate(yawRotate);

            //// qDebug() << "ICON SIZE:" << radius;

            float iconSize = radius*0.9f;
            QPolygonF poly(3);
            poly.replace(0, QPointF(0.0f*iconSize, 0.3f*iconSize));
            poly.replace(1, QPointF(-0.2f*iconSize, -0.2f*iconSize));
            poly.replace(2, QPointF(0.2f*iconSize, -0.2f*iconSize));

            painter.setBrush(QBrush(iconColor));
            QPen iconPen(Qt::black);
            iconPen.setWidthF(1.0f);
            painter.setPen(iconPen);

            painter.drawPolygon(poly);
            painter.rotate(-yawRotate);
        }
        break;
    }
}
