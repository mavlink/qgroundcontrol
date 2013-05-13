#include <QPainter>

#include "HUD2IndicatorClimb.h"
#include "HUD2Math.h"

HUD2IndicatorClimb::HUD2IndicatorClimb(const HUD2Data *data, QWidget *parent) :
    HUD2Ribbon(POSITION_RIGHT, false, QString("CLIMBMETER"), data, parent)
{
}
