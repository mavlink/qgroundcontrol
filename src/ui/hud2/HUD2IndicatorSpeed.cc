#include <QPainter>
#include <QSettings>

#include "HUD2IndicatorSpeed.h"

HUD2IndicatorSpeed::HUD2IndicatorSpeed(const HUD2Data *huddata, QWidget *parent) :
    HUD2Ribbon(POSITION_LEFT, false, QString("SPEEDOMETER"), huddata, parent)
{

}

