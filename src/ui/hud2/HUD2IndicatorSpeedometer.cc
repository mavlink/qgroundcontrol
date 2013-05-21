#include <QtGui>

#include "HUD2IndicatorSpeedometer.h"
#include "HUD2Data.h"

HUD2IndicatorSpeedometer::HUD2IndicatorSpeedometer(const HUD2Data *huddata, QWidget *parent):
    HUD2Ribbon(POSITION_LEFT, false, QString("SPEEDOMETER"), parent),
    huddata(huddata),
    src(SPEEDOMETER_AIR)
{
}

double HUD2IndicatorSpeedometer::processData(void)
{
    switch(src){
    case SPEEDOMETER_AIR:
        return huddata->airspeed;
        break;

    case SPEEDOMETER_GROUND:
        return huddata->groundspeed;
        break;

    default:
        return huddata->airspeed;
        break;
    }
}

void HUD2IndicatorSpeedometer::selectSource(int index)
{
    this->src = (speedometer_source_t)index;
}
