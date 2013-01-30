#include <QtGui>

#include "HUD2HorizonCrosshair.h"
#include "HUD2Math.h"

HUD2HorizonCrosshair::HUD2HorizonCrosshair(const qreal *gap, QWidget *parent) :
    QWidget(parent),
    gap(gap)
{
    pen.setColor(Qt::green);
    pen.setWidth(2);
}

void HUD2HorizonCrosshair::updateGeometry(const QSize &size){

    int _gap = percent2pix_w(size, *gap);
    int minigap = _gap/5;

    pen.setWidth(percent2pix_h(size, 0.3));

    // left
    lines[0] = QLine(-_gap/2, 0, 0, 0);
    lines[0].translate(-minigap, 0);

    // right
    lines[1] = QLine(0, 0, _gap/2, 0);
    lines[1].translate(minigap, 0);

    // up
    lines[2] = QLine(0, -_gap/2, 0, 0);
    lines[2].translate(0, -minigap);
}

void HUD2HorizonCrosshair::paint(QPainter *painter){
    painter->save();
    painter->setPen(pen);
    painter->drawLines(lines, 3);
    painter->restore();
}

void HUD2HorizonCrosshair::setColor(QColor color){
    pen.setColor(color);
}
