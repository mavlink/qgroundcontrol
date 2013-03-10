#include <QColor>
#include <QPainter>

#include "HUD2Drawer.h"
#include "HUD2Dialog.h"


HUD2Drawer::HUD2Drawer(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    horizon(huddata, this),
    roll(huddata, this),
    speed(huddata, this),
    climb(huddata, this),
    compass(huddata, this)
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
    speed.paint(painter);
    climb.paint(painter);
    compass.paint(painter);
    fps.paint(painter);

    emit paintComplete();
}

void HUD2Drawer::updateGeometry(const QSize &size){
    horizon.updateGeometry(size);
    roll.updateGeometry(size);
    speed.updateGeometry(size);
    climb.updateGeometry(size);
    compass.updateGeometry(size);
    fps.updateGeometry(size);
}

void HUD2Drawer::showDialog(void){
    HUD2Dialog *dialog = new HUD2Dialog(this);
    dialog->exec();
    delete dialog;
}
