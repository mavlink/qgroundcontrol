#include <QSettings>

#include "HUD2IndicatorAltimeter.h"
#include "HUD2Data.h"

HUD2IndicatorAltimeter::HUD2IndicatorAltimeter(
        const HUD2Data *huddata, QString name, QWidget *parent):
    HUD2Ribbon(POSITION_RIGHT, false, name, parent),
    huddata(huddata),
    source(ALTIMETER_BARO),
    label_top("GNSS"),
    label_bot("ALT")
{
    QSettings settings;
    settings.beginGroup("QGC_HUD2");
    source = (altimeter_source_t)settings.value(name + QString("_DATA_SOURCE"), 0).toInt();
    units = (altimeter_units_t)settings.value(name + QString("_MEASUREMENT_UNITS"), 0).toInt();
    settings.endGroup();
}

double HUD2IndicatorAltimeter::processData(void)
{
    double tmp;

    switch(source){
    case ALTIMETER_BARO:
        tmp = huddata->alt_baro;
        break;
    case ALTIMETER_GNSS:
        tmp = huddata->alt_gnss;
        break;
    default:
        tmp = huddata->alt_baro;
        break;
    }

    switch(units){
    case ALTIMETER_UNITS_M:
        return tmp;
    case ALTIMETER_UNITS_F:
        return tmp / 0.305;
    default:
        return tmp;
    }
}

const QString &HUD2IndicatorAltimeter::getLabelTop(void)
{
    switch(source){
    case ALTIMETER_BARO:
        return label_top = "BARO";
        break;
    case ALTIMETER_GNSS:
        return label_top = "GNSS";
        break;
    default:
        return label_top = "";
        break;
    }
}

const QString &HUD2IndicatorAltimeter::getLabelBot(void)
{
    double tmp = huddata->climb;
    switch(units){
    case ALTIMETER_UNITS_M:
        label_bot = QString::number(tmp, 'f', 1);
        label_bot += " m/s";
        break;
    case ALTIMETER_UNITS_F:
        label_bot = QString::number(tmp / 0.305, 'f', 1);
        label_bot += " f/s";
    default:
        break;
    }

    return label_bot;
}

void HUD2IndicatorAltimeter::syncSettings()
{
    HUD2Ribbon::syncSettings();

    QSettings settings;
    settings.setValue(("QGC_HUD2/" + name + "_MEASUREMENT_UNITS"), units);
    settings.setValue(("QGC_HUD2/" + name + "_DATA_SOURCE"), source);
}
