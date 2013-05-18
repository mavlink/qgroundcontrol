#include "HUD2IndicatorAltimeter.h"
#include "HUD2Data.h"

HUD2IndicatorAltimeter::HUD2IndicatorAltimeter(const HUD2Data *huddata, QWidget *parent):
    HUD2Ribbon(POSITION_RIGHT, false, QString("ALTIMETER"), huddata, parent)
{
}

double HUD2IndicatorAltimeter::processData(void)
{
    return huddata->alt;
}
