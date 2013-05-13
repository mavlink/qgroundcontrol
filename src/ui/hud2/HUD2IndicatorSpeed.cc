#include <QPainter>

#include "HUD2IndicatorSpeed.h"

HUD2IndicatorSpeed::HUD2IndicatorSpeed(const float *value, QWidget *parent) :
    HUD2Ribbon(POSITION_LEFT, false, value, parent)
{

}
