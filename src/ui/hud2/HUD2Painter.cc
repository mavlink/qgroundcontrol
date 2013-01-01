#include <QColor>
#include "HUD2Painter.h"

HUD2Painter::HUD2Painter(HUD2data *huddata, QWidget *parent) :
    QWidget(parent),
    horizon(huddata)
{
    this->huddata = huddata;

    defaultColor = QColor(70, 255, 70);
    warningColor = Qt::yellow;
    criticalColor = Qt::red;
    infoColor = QColor(20, 200, 20);
    fuelColor = criticalColor;
}

void HUD2Painter::paint(QPainter *painter, QPaintEvent *event)
{
    Q_UNUSED(event);
    QRect hudrect = painter->viewport();
    painter->translate(hudrect.center());

    yaw.paint(painter, defaultColor);
    horizon.paint(painter, defaultColor);
}

void HUD2Painter::updateGeometry(const QSize *size){
    yaw.updatesize(size);
    horizon.updateGeometry(size);
}
