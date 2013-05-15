#include "HUD2Speedometer.h"
#include "HUD2Data.h"

HUD2Speedometer::HUD2Speedometer(const HUD2Data *huddata, QWidget *parent):
    HUD2Ribbon(POSITION_LEFT, false, QString("SPEEDOMETER"), huddata, parent)
{
}

double HUD2Speedometer::processData(void)
{
    return huddata->airspeed;
}
