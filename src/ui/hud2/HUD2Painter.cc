#include <QColor>

#include "HUD2Painter.h"


HUD2Painter::HUD2Painter(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    //this->huddata = huddata;

    this->horizon = new HUD2Horizon(huddata, this);
    this->yaw = new HUD2HorizonYaw(huddata, this);

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

    this->altimeter = new HUD2Dial(15, 25, -25,
                                   10, 1, 3,
                                   handPens, handScales,
                                   this);
    delete[] handPens;
    delete[] handScales;

    defaultColor = QColor(70, 255, 70);
    warningColor = Qt::yellow;
    criticalColor = Qt::red;
    infoColor = QColor(20, 200, 20);
    fuelColor = criticalColor;
}

void HUD2Painter::paint(QPainter *painter)
{
    QRect hudrect = painter->window();
    painter->translate(hudrect.center());

    yaw->paint(painter, defaultColor);
    horizon->paint(painter, defaultColor);
    altimeter->paint(painter, huddata->alt);

    emit paintComplete();
}

void HUD2Painter::updateGeometry(const QSize *size){
    yaw->updateGeometry(size);
    horizon->updateGeometry(size);
    altimeter->updateGeometry(size);
}
