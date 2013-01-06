#include <QColor>

#include "HUD2HorizonYawIndicator.h"

// YAW INDICATOR
//
//      .
//    .   .
//   .......
//

HUD2YawIndicator::HUD2YawIndicator()
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

void HUD2YawIndicator::updatesize(const QSize *size){
    int h = size->height() / scale;
    int clearance = h / 2;

    poly.setPoint(0, QPoint(0,   0));
    poly.setPoint(1, QPoint(h/2,  h));
    poly.setPoint(2, QPoint(-h/2, h));
    poly.setPoint(3, QPoint(0,    0));

    poly.translate(0, (size->height() / 2) - h - clearance);
    pen.setWidth(2);
}

void HUD2YawIndicator::paint(QPainter *painter, QColor color){
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawPolyline(poly);
}

void HUD2YawIndicator::setColor(QColor color){
    this->color = color;
}
