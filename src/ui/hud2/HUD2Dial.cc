#include "HUD2Dial.h"
#include "HUD2Math.h"

HUD2Dial::HUD2Dial(QWidget *parent, qreal r, qreal x, qreal y,
                   int marks, int markStep, int hands)
    : QWidget(parent),
      r(r),
      x(x),
      y(y),
      marks(marks),
      markStep(markStep),
      hands(hands)
{
    this->dialPen.setColor(Qt::green);
    this->dialPen.setWidth(0);
}

void HUD2Dial::updateGeometry(const QSize *size){
    _r = percent2pix_h(size, r);
    _y = percent2pix_h(size, y);
    _x = _y;
}

void HUD2Dial::paint(QPainter *painter, qreal value){
    painter->save();
    painter->setPen(dialPen);
    painter->drawEllipse(QPoint(_x, _y), _r, _r);
    painter->restore();
}

void HUD2Dial::setColor(QColor color){
}
