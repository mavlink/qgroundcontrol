#include <QPainter>

#include "HUD2IndicatorClimb.h"
#include "HUD2Math.h"

HUD2IndicatorClimb::HUD2IndicatorClimb(const float *value, QWidget *parent) :
    HUD2Ribbon(POSITION_RIGHT, false, value, parent)
{

}
