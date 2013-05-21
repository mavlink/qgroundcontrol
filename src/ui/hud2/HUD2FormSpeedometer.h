#ifndef HUD2FORMSPEEDOMETER_H
#define HUD2FORMSPEEDOMETER_H

#include "HUD2FormRibbon.h"
#include "HUD2IndicatorSpeedometer.h"

class HUD2FormSpeedometer : public HUD2FormRibbon
{
public:
    HUD2FormSpeedometer(HUD2IndicatorSpeedometer *ribbon, QWidget *parent);
};

#endif // HUD2FORMSPEEDOMETER_H
