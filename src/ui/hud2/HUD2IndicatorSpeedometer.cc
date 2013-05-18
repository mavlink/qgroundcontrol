#include "HUD2IndicatorSpeedometer.h"
#include "HUD2Data.h"

HUD2IndicatorSpeedometer::HUD2IndicatorSpeedometer(const HUD2Data *huddata, QWidget *parent):
    HUD2Ribbon(POSITION_LEFT, false, QString("SPEEDOMETER"), huddata, parent)
{
}

double HUD2IndicatorSpeedometer::processData(void)
{
    return huddata->airspeed;
}
