#include <QColor>

#include "HUD2Painter.h"


HUD2Painter::HUD2Painter(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    horizon(huddata, this),
    altimeter(huddata, this),
    huddata(huddata)
{
    defaultColor = QColor(70, 255, 70);
    warningColor = Qt::yellow;
    criticalColor = Qt::red;
    infoColor = QColor(20, 200, 20);
    fuelColor = criticalColor;
}

void HUD2Painter::paint(QPainter *painter)
{
    painter->translate(painter->window().center());

    horizon.paint(painter, defaultColor);
    altimeter.paint(painter, defaultColor);

    emit paintComplete();
}

void HUD2Painter::updateGeometry(const QSize *size){
    horizon.updateGeometry(size);
    altimeter.updateGeometry(size);
}
