#include <QColor>
#include <QPainter>

#include "HUD2Drawer.h"


HUD2Drawer::HUD2Drawer(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    horizon(huddata, this),
    roll(huddata, this),
    yaw(huddata, this),
    speed(huddata, this),
    climb(huddata, this)
{
    defaultColor = QColor(70, 255, 70);
    warningColor = Qt::yellow;
    criticalColor = Qt::red;
    infoColor = QColor(20, 200, 20);
    fuelColor = criticalColor;
}

void HUD2Drawer::paint(QPainter *painter)
{
    horizon.paint(painter);
    roll.paint(painter);
    yaw.paint(painter);
    speed.paint(painter);
    climb.paint(painter);
    fps.paint(painter);

    emit paintComplete();
}

void HUD2Drawer::updateGeometry(const QSize &size){
    horizon.updateGeometry(size);
    roll.updateGeometry(size);
    yaw.updateGeometry(size);
    speed.updateGeometry(size);
    climb.updateGeometry(size);
    fps.updateGeometry(size);
}
