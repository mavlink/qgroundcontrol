#include <QtGui>

#include "HUD2Altimeter.h"

HUD2Altimeter::HUD2Altimeter(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    // create some stuff for altimeter
    int hands = 3;
    QPen *handPens = new QPen[hands];
    qreal *handScales = new qreal[hands];

    handPens[0].setColor(Qt::white);
    handPens[0].setWidth(6);
    handScales[0] = 1000;

    handPens[1].setColor(Qt::green);
    handPens[1].setWidth(3);
    handScales[1] = 100;

    handPens[2].setColor(Qt::red);
    handPens[2].setWidth(1);
    handScales[2] = 10;

    this->dial = new HUD2Dial(15, 25, -25,
                              10, 1, 3,
                              handPens, handScales,
                              this);
    delete[] handPens;
    delete[] handScales;
}

void HUD2Altimeter::updateGeometry(const QSize *size){
    dial->updateGeometry(size);
}

void HUD2Altimeter::paint(QPainter *painter, QColor color){
    Q_UNUSED(color);
    painter->save();
    dial->paint(painter, huddata->alt);
    painter->restore();
}

void HUD2Altimeter::setColor(QColor color){
    Q_UNUSED(color);
}
