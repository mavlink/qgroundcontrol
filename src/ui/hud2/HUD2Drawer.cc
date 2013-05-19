#include <QColor>
#include <QPainter>
#include <QSettings>

#include "HUD2Drawer.h"
#include "HUD2DialogInstruments.h"
#include "HUD2DialogColor.h"


HUD2Drawer::HUD2Drawer(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    horizon(&huddata->pitch, &huddata->roll, this),
    roll(&huddata->roll, this),
    speedometer(huddata, this),
    altimeter(huddata, this),
    compass(huddata, this),
    fps(this),
    msg(this)
{
    defaultColor = QColor(70, 255, 70);
    warningColor = Qt::yellow;
    criticalColor = Qt::red;
    infoColor = QColor(20, 200, 20);
    fuelColor = criticalColor;

    QColor color;
    QSettings settings;
    settings.beginGroup("QGC_HUD2");
    color = settings.value("INSTRUMENTS_COLOR", INSTRUMENTS_COLOR_DEFAULT).value<QColor>();
    roll.setColor(color);
    speedometer.setColor(color);
    altimeter.setColor(color);
    compass.setColor(color);
    fps.setColor(color);

    settings.endGroup();
}

void HUD2Drawer::paint(QPainter *painter)
{
    horizon.paint(painter);
    roll.paint(painter);
    speedometer.paint(painter);
    altimeter.paint(painter);
    compass.paint(painter);
    fps.paint(painter);
    msg.paint(painter);

    emit paintComplete();
}

void HUD2Drawer::updateGeometry(const QSize &size){
    horizon.updateGeometry(size);
    roll.updateGeometry(size);
    speedometer.updateGeometry(size);
    altimeter.updateGeometry(size);
    compass.updateGeometry(size);
    fps.updateGeometry(size);
    msg.updateGeometry(size);
}

void HUD2Drawer::showDialog(void){
    HUD2FormHorizon *horizon_form = horizon.getForm();

    HUD2DialogInstruments *d = new HUD2DialogInstruments(
                horizon_form, &roll, &speedometer, &altimeter, &compass, &fps, this);
    d->exec();

    delete horizon_form;
    delete d;
}

void HUD2Drawer::showColorDialog(void){
    HUD2DialogColor *d = new HUD2DialogColor(
                &horizon, &roll, &speedometer, &altimeter, &compass, &fps, this);
    d->exec();
    delete d;
}

void HUD2Drawer::updateTextMessage(int uasid, int componentid, int severity, QString text){
    this->msg.updateTextMessage(uasid, componentid, severity, text);
}

