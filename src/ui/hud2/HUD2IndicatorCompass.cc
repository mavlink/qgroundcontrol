#include "HUD2IndicatorCompass.h"
#include "HUD2Data.h"
#include "HUD2Math.h"

HUD2IndicatorCompass::HUD2IndicatorCompass(const HUD2Data *huddata, QWidget *parent):
    HUD2Ribbon(POSITION_TOP, true, QString("COMPASS"), huddata, parent)
{
}

double HUD2IndicatorCompass::processData(void)
{
    return rad2deg(huddata->yaw);
}
