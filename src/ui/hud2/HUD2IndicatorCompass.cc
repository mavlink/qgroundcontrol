#include <QPainter>

#include "HUD2IndicatorCompass.h"
#include "HUD2Math.h"

HUD2IndicatorCompass::HUD2IndicatorCompass(const HUD2Data *huddata, QWidget *parent) :
    HUD2Ribbon(POSITION_TOP, true, QString("COMPASS"), huddata, parent)
{

}
