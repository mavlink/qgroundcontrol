#include <QtGui>

#include "HUD2IndicatorHorizonCrosshair.h"
#include "HUD2Math.h"

HUD2IndicatorHorizonCrosshair::HUD2IndicatorHorizonCrosshair(const qreal *gap, QWidget *parent) :
    QWidget(parent),
    gap(gap)
{
    pen.setColor(Qt::green);
    pen.setWidth(2);
}

void HUD2IndicatorHorizonCrosshair::updateGeometry(const QSize &size){

    int a = percent2pix_w(size, *gap);
    int minigap = a/5;

    pen.setWidth(percent2pix_h(size, 0.3));

    // left
    lines[0] = QLine(-a/2, 0, 0, 0);
    lines[0].translate(-minigap, 0);

    // right
    lines[1] = QLine(0, 0, a/2, 0);
    lines[1].translate(minigap, 0);

    // up
    lines[2] = QLine(0, -a/2, 0, 0);
    lines[2].translate(0, -minigap);
}

void HUD2IndicatorHorizonCrosshair::paint(QPainter *painter){
    painter->save();
    painter->translate(painter->window().center());
    painter->setPen(pen);
    painter->drawLines(lines, 3);
    painter->restore();
}

void HUD2IndicatorHorizonCrosshair::setColor(QColor color){
    pen.setColor(color);
}
