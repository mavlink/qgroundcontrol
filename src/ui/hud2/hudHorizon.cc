#include <QtGui>
#include "hudHorizon.h"

HudHorizon::HudHorizon(HUD2data *data, QWidget *parent) :
    QWidget(parent),
    pitchlinepos(parent)
{
    this->data = data;
    this->updateGeometry(new QSize(640, 480));
}

void HudHorizon::updateGeometry(const QSize *size){
    int x1 = size->width();

    gap = x1 / 10;
    pen.setWidth(6);
    left.setLine(-x1, 0, -gap/2, 0);
    right.setLine(gap/2, 0, x1, 0);

    pitchlinepos.updateGeometry(size);
}

void HudHorizon::paint(QPainter *painter, QColor color){
    painter->save();

    painter->save();
    painter->translate(0, -50);
    pitchlinepos.paint(painter);
    painter->restore();

    painter->rotate(-rad2deg(data->roll));
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawLine(left);
    painter->drawLine(right);
    painter->restore();
}

void HudHorizon::updateColor(QColor color){
    pen.setColor(color);
}

qreal HudHorizon::rad2deg(float rad){
    return rad * (180.0 / M_PI);
}
