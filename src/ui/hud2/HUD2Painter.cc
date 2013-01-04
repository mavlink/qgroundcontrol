#include <QColor>

#include "HUD2Painter.h"

HUD2Painter::HUD2Painter(HUD2data *huddata, QWidget *parent) :
    QWidget(parent),
    horizon(huddata, this),
    altimeter(this, 20, 25, 25)
{
    this->huddata = huddata;

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

    yaw.paint(painter, defaultColor);
    horizon.paint(painter, defaultColor);
    altimeter.paint(painter, huddata->alt);

    emit paintComplete();
}

void HUD2Painter::updateGeometry(const QSize *size){
    yaw.updatesize(size);
    horizon.updateGeometry(size);
    altimeter.updateGeometry(size);
}
