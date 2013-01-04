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
    handPens = new QPen[hands];
    handLines = new QLine[hands];
    handScales = new qreal[hands];

    handPens[0].setColor(Qt::white);
    handPens[0].setWidth(6);
    handScales[0] = 1000;

    handPens[1].setColor(Qt::green);
    handPens[1].setWidth(3);
    handScales[1] = 100;

    handPens[2].setColor(Qt::red);
    handPens[2].setWidth(1);
    handScales[2] = 10;

    this->dialPen.setColor(Qt::green);
    this->dialPen.setWidth(0);
}

void HUD2Dial::updateGeometry(const QSize *size){
    // main sizes
    _r = percent2pix_h(size, r);
    _y = percent2pix_h(size, y);
    _x = percent2pix_w(size, x);

    handLines[0] = QLine(0, 0, 0, -_r/2);
    handLines[1] = QLine(0, 0, 0, -(3*_r)/4 );
    handLines[2] = QLine(0, 0, 0, -_r);
}

void HUD2Dial::paint(QPainter *painter, qreal value){
    painter->save();
    painter->setPen(dialPen);
    painter->drawEllipse(QPoint(_x, _y), _r, _r);

    // hands
    painter->translate(_x, _y);

    painter->save();
    painter->rotate(360*value / handScales[0]);
    painter->setPen(handPens[0]);
    painter->drawLine(handLines[0]);
    painter->restore();

    painter->save();
    painter->rotate(360*value / handScales[1]);
    painter->setPen(handPens[1]);
    painter->drawLine(handLines[1]);
    painter->restore();

    painter->save();
    painter->rotate(360*value / handScales[2]);
    painter->setPen(handPens[2]);
    painter->drawLine(handLines[2]);
    painter->restore();

    painter->restore();
}

void HUD2Dial::setColor(QColor color){
}
