#include "MAV2DIcon.h"
#include <QPainter>

MAV2DIcon::MAV2DIcon(QGraphicsItem* parent) :
    QGC2DIcon(parent)
{
}

/**
 * @return the bounding rectangle of the icon
 */
QRectF MAV2DIcon::boundingRect() const
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
void MAV2DIcon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);
}
