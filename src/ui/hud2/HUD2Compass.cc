#include "HUD2Compass.h"
#include "HUD2Data.h"
#include "HUD2Math.h"

HUD2Compass::HUD2Compass(const HUD2Data *huddata, QWidget *parent):
    HUD2Ribbon(POSITION_TOP, true, QString("COMPASS"), huddata, parent)
{
}

double HUD2Compass::processData(void)
{
    return rad2deg(huddata->yaw);
}
