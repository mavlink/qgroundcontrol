#include <QPainter>

#include "HUD2IndicatorClimb.h"
#include "HUD2Math.h"

HUD2IndicatorClimb::HUD2IndicatorClimb(HUD2Data *huddata, QWidget *parent) :
    HUD2Ribbon(POSITION_RIGHT, false, QString("CLIMBMETER"), huddata, parent)
{
}
