#include <QPainter>

#include "HUD2IndicatorCompass.h"
#include "HUD2Math.h"

HUD2IndicatorCompass::HUD2IndicatorCompass(const float *value, QWidget *parent) :
    HUD2Ribbon(POSITION_TOP, true, value, parent)
{

}
