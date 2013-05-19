#ifndef HUD2SPEEDOMETER_H
#define HUD2SPEEDOMETER_H

#include "HUD2Ribbon.h"

class HUD2IndicatorSpeedometer : public HUD2Ribbon
{
public:
    HUD2IndicatorSpeedometer(const HUD2Data *huddata, QWidget *parent);

private:
    double processData(void);

private:
};

#endif // HUD2SPEEDOMETER_H
