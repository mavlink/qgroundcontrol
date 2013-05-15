#include "HUD2Altimeter.h"
#include "HUD2Data.h"

HUD2Altimeter::HUD2Altimeter(const HUD2Data *huddata, QWidget *parent):
    HUD2Ribbon(POSITION_RIGHT, false, QString("ALTIMETER"), huddata, parent)
{
}

double HUD2Altimeter::processData(void)
{
    return huddata->alt;
}
