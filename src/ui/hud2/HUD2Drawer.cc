#include <QColor>
#include <QPainter>
#include <QSettings>

#include "HUD2Drawer.h"
#include "HUD2InstrumentsDialog.h"
#include "HUD2ColorDialog.h"


HUD2Drawer::HUD2Drawer(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    horizon(&huddata->pitch, &huddata->roll, this),
    roll(&huddata->roll, this),
    speed(POSITION_LEFT, false, QString("SPEEDOMETER"), &huddata->airspeed, this),
    climb(POSITION_RIGHT, false, QString("CLIMBMETER"), &huddata->alt, this),
    compass(POSITION_TOP, true, QString("COMPASS"), &huddata->yaw, this),
    fps(this)
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
    speed.setColor(color);
    climb.setColor(color);
    compass.setColor(color);
    fps.setColor(color);

    // set more or less suitable defaults
    if (!settings.contains("SPEEDOMETER_VALUE_INDEX"))
        settings.setValue("SPEEDOMETER_VALUE_INDEX", VALUE_IDX_AIRSPEED);
    if (!settings.contains("CLIMBMETER_VALUE_INDEX"))
        settings.setValue("CLIMBMETER_VALUE_INDEX", VALUE_IDX_ALT);
    if (!settings.contains("COMPASS_VALUE_INDEX"))
        settings.setValue("COMPASS_VALUE_INDEX", VALUE_IDX_YAW);

    settings.endGroup();
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
    HUD2InstrumentsDialog *d = new HUD2InstrumentsDialog(
                &horizon, &roll, &speed, &climb, &compass, &fps, this);
    d->exec();
    delete d;
}

void HUD2Drawer::showColorDialog(void){
    HUD2ColorDialog *d = new HUD2ColorDialog(
                &horizon, &roll, &speed, &climb, &compass, &fps, this);
    d->exec();
    delete d;
}

