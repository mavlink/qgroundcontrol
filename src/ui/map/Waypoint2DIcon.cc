#include "Waypoint2DIcon.h"
#include <QPainter>

#include <QDebug>

Waypoint2DIcon::Waypoint2DIcon(QGraphicsItem* parent) :
        QGC2DIcon(parent)
{
}

/**
 * @return the bounding rectangle of the icon
 */
QRectF Waypoint2DIcon::boundingRect() const
{
    qreal penWidth = 1;
    return QRectF(-10 - penWidth / 2, -10 - penWidth / 2,
                  20 + penWidth, 20 + penWidth);
}

/**
 * @param painter QPainter to draw with
 * @param option Visual style
 * @param widget Parent widget
 */
void Waypoint2DIcon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    qDebug() << __FILE__ << __LINE__ << "DRAWING";
    painter->setPen(QPen(Qt::red));
    painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);
}
