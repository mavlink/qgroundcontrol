#include <QColor>

#include "hudyawindicator.h"

// YAW INDICATOR
//
//      .
//    .   .
//   .......
//

HudYawIndicator::HudYawIndicator()
{
    scale = 15;

    color = QColor(Qt::green); // default color
    pen = QPen(color);
    poly = QPolygon(4);
    poly.setPoint(0, 0, 0);
    poly.setPoint(1, 0, 0);
    poly.setPoint(2, 0, 0);
    poly.setPoint(3, 0, 0);
}

void HudYawIndicator::updatesize(QRect *rect){
    int h = rect->height() / scale;
    int clearance = h / 2;

    poly.setPoint(0, QPoint(0,   0));
    poly.setPoint(1, QPoint(h/2,  h));
    poly.setPoint(2, QPoint(-h/2, h));
    poly.setPoint(3, QPoint(0,    0));

    poly.translate(0, (rect->height() / 2) - h - clearance);
    pen.setWidth(2);
}

void HudYawIndicator::paint(QPainter *painter){
    painter->setPen(pen);
    painter->drawPolyline(poly);
}

void HudYawIndicator::setColor(QColor color){
    this->color = color;
}
