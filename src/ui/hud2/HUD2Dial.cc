#include "HUD2Dial.h"

HUD2Dial::HUD2Dial(QWidget *parent, int rscale, int xscale, int yscale,
                   int marks, int markStep, int hands)
    : QWidget(parent)
{
    this->rscale    = rscale;
    this->xscale    = xscale;
    this->yscale    = yscale;
    this->marks     = marks;
    this->markStep  = markStep;
    this->hands     = hands;

    this->dialPen.setColor(Qt::green);
    this->dialPen.setWidth(1);
}

void HUD2Dial::updateGeometry(const QSize *size){
    r = 200;
    x = 10;
    y = 10;
}

void HUD2Dial::paint(QPainter *painter, qreal value){
    painter->drawEllipse(QPoint(x, y), r, r);
}

void HUD2Dial::setColor(QColor color){
}
