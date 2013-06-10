#include <QtGui>

#include "HUD2IndicatorSpeedometer.h"
#include "HUD2Data.h"

HUD2IndicatorSpeedometer::HUD2IndicatorSpeedometer(
        const HUD2Data *huddata, QString name, QWidget *parent):
    HUD2Ribbon(POSITION_LEFT, false, name, parent),
    huddata(huddata),
    source(SPEEDOMETER_AIR),
    units(SPEEDOMETER_UNITS_MS),
    label_top("AIR"),
    label_bot("")
{
    QSettings settings;
    settings.beginGroup("QGC_HUD2");
    source = (speedometer_source_t)settings.value(name + QString("_DATA_SOURCE"), 0).toInt();
    units = (speedometer_units_t)settings.value(name + QString("_MEASUREMENT_UNITS"), 0).toInt();
    settings.endGroup();
}

void HUD2IndicatorSpeedometer::syncSettings(void)
{
    HUD2Ribbon::syncSettings();

    QSettings settings;
    settings.setValue(("QGC_HUD2/" + name + "_DATA_SOURCE"), source);
    settings.setValue(("QGC_HUD2/" + name + "_MEASUREMENT_UNITS"), units);
}

double HUD2IndicatorSpeedometer::processData(void)
{
    double tmp;

    switch(source){
    case SPEEDOMETER_AIR:
        tmp = huddata->airspeed;
        break;
    case SPEEDOMETER_GROUND:
        tmp = huddata->groundspeed;
        break;
    default:
        tmp = huddata->airspeed;
        break;
    }

    switch(units){
    case SPEEDOMETER_UNITS_MS:
        break;
    case SPEEDOMETER_UNITS_KMH:
        tmp *= 3.6;
        break;
    default:
        break;
    }

    return tmp;
}

const QString &HUD2IndicatorSpeedometer::getLabelTop(void)
{
    switch(source){
    case SPEEDOMETER_AIR:
        label_top = "AIR";
        break;
    case SPEEDOMETER_GROUND:
        label_top = "GND";
        break;
    default:
        label_top = "";
        break;
    }

    switch(units){
    case SPEEDOMETER_UNITS_MS:
        label_top += ", m/s";
        break;
    case SPEEDOMETER_UNITS_KMH:
        label_top += ", km/h";
        break;
    default:
        label_top += "";
        break;
    }

    return label_top;
}

const QString &HUD2IndicatorSpeedometer::getLabelBot(void)
{
    return label_bot;
}
