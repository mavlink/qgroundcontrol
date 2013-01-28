#include <QColor>

#include "HUD2Drawer.h"


HUD2Drawer::HUD2Drawer(HUD2Data &huddata, QWidget *parent) :
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

void HUD2Drawer::paint_static(QPainter *painter)
{
    painter->save();
    painter->translate(painter->window().center());
    horizon.paint_static(painter);
    fps.paint_static(painter);
    painter->restore();
}

void HUD2Drawer::paint_dynamic(QPainter *painter)
{
    painter->save();
    painter->translate(painter->window().center());

    horizon.paint_dynamic(painter);
    altimeter.paint(painter);
    fps.paint_dynamic(painter);

    painter->restore();

    emit paintComplete();
}

void HUD2Drawer::updateGeometry(const QSize &size){
    horizon.updateGeometry(size);
    altimeter.updateGeometry(size);
    fps.updateGeometry(size);
}
