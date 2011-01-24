#include "MAV2DIcon.h"
#include <QPainter>

#include <qmath.h>

MAV2DIcon::MAV2DIcon(UASInterface* uas, int radius, int type, const QColor& color, QString name, Alignment alignment, QPen* pen)
    : Point(uas->getLatitude(), uas->getLongitude(), name, alignment),
    yaw(0.0f),
    radius(radius),
    type(type),
    iconColor(color),
    selected(uas->getSelected()),
    uasid(uas->getUASID())
{
    //connect
    size = QSize(radius, radius);
    drawIcon(pen);
}

MAV2DIcon::MAV2DIcon(qreal x, qreal y, QString name, Alignment alignment, QPen* pen)
    : Point(x, y, name, alignment),
    radius(20),
    type(0),
    iconColor(Qt::yellow),
    selected(false),
    uasid(0)
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

void MAV2DIcon::setSelectedUAS(bool selected)
{
    this->selected = selected;
    drawIcon(mypen);
}

/**
 * Yaw changes of less than ±15 degrees are ignored.
 *
 * @param yaw in radians, 0 = north, positive = clockwise
 */
void MAV2DIcon::setYaw(float yaw)
{
    //qDebug() << "MAV2Icon" << yaw;
    float diff = fabs(yaw - this->yaw);
    while (diff > M_PI)
    {
        diff -= M_PI;
    }

    if (diff > 0.1)
    {
        this->yaw = yaw;
        drawIcon(mypen);
    }
}

void MAV2DIcon::drawIcon(QPen* pen)
{
    Q_UNUSED(pen);
    if (!mypixmap) mypixmap = new QPixmap(radius+1, radius+1);
    mypixmap->fill(Qt::transparent);
    QPainter painter(mypixmap);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    // Rotate by yaw
    painter.translate(radius/2, radius/2);

    // Draw selected indicator
    if (selected)
    {
//        qDebug() << "SYSTEM IS NOW SELECTED";
//        QColor color(Qt::yellow);
//        color.setAlpha(0.3f);
        painter.setBrush(Qt::NoBrush);
//        QPen selPen(color);
//        int width = 5;
//        selPen.setWidth(width);
        QPen pen(Qt::yellow);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawEllipse(QPoint(0, 0), radius/2-1, radius/2-1);
        //qDebug() << "Painting ellipse" << radius/2-width << width;
        //selPen->deleteLater();
    }

    switch (type)
    {
    case MAV_ICON_AIRPLANE:
        {
            // DRAW AIRPLANE

            // Rotate 180 degs more since the icon is oriented downwards
            float yawRotate = (yaw/(float)M_PI)*180.0f + 180.0f+180.0f;

            painter.rotate(yawRotate);

            //qDebug() << "ICON SIZE:" << radius;

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
        }
        break;
    case MAV_ICON_QUADROTOR:
        {
            // QUADROTOR
            float iconSize = radius*0.9f;
            float yawRotate = (yaw/(float)M_PI)*180.0f + 180.0f;

            painter.rotate(yawRotate);

            //qDebug() << "ICON SIZE:" << radius;

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
        }
        break;
    case MAV_ICON_ROTARY_WING:
        case MAV_ICON_GENERIC:
    default:
        {
            // GENERIC

            float yawRotate = (yaw/(float)M_PI)*180.0f + 180.0f;

            painter.rotate(yawRotate);

            //qDebug() << "ICON SIZE:" << radius;

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
        }
        break;
    }
}
