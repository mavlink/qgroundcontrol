#include <QColor>
#include <QPainter>

#include "HUD2Drawer.h"


HUD2Drawer::HUD2Drawer(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    horizon(huddata, this),
    roll(huddata, this),
    yaw(huddata, this),
    huddata(huddata)
{
    defaultColor = QColor(70, 255, 70);
    warningColor = Qt::yellow;
    criticalColor = Qt::red;
    infoColor = QColor(20, 200, 20);
    fuelColor = criticalColor;
}

void HUD2Drawer::paint(QPainter *painter)
{
    roll.paint(painter);
    horizon.paint(painter);
    yaw.paint(painter);
    fps.paint(painter);

    emit paintComplete();
}

void HUD2Drawer::updateGeometry(const QSize &size){
    roll.updateGeometry(size);
    horizon.updateGeometry(size);
    yaw.updateGeometry(size);
    fps.updateGeometry(size);
}
