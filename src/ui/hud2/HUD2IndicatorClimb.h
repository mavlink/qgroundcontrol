#ifndef HUD2INDICATORCLIMB_H
#define HUD2INDICATORCLIMB_H

#include <QWidget>

#include "HUD2Data.h"
#include "HUD2Ribbon.h"

class HUD2IndicatorClimb : public HUD2Ribbon
{
    Q_OBJECT
public:
    explicit HUD2IndicatorClimb(const float *value, QWidget *parent);
};

#endif // HUD2INDICATORCLIMB_H
