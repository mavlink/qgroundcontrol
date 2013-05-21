#include "HUD2IndicatorAltimeter.h"
#include "HUD2Data.h"

HUD2IndicatorAltimeter::HUD2IndicatorAltimeter(const HUD2Data *huddata, QWidget *parent):
    HUD2Ribbon(POSITION_RIGHT, false, QString("ALTIMETER"), parent),
    huddata(huddata),
    src(ALTIMETER_BARO)
{
}

double HUD2IndicatorAltimeter::processData(void)
{
    switch(src){
    case ALTIMETER_BARO:
        return huddata->alt_baro;
        break;

    case ALTIMETER_GNSS:
        return huddata->alt_gnss;
        break;

    default:
        return huddata->alt_baro;
        break;
    }
}

void HUD2IndicatorAltimeter::selectSource(int index)
{
    this->src = (altimeter_source_t)index;
}
