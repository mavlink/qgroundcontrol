#ifndef HUD2SPEEDOMETER_H
#define HUD2SPEEDOMETER_H

#include "HUD2Ribbon.h"

class HUD2Speedometer : public HUD2Ribbon
{
public:
    HUD2Speedometer(const HUD2Data *huddata, QWidget *parent);
private:
    double processData(void);
};

#endif // HUD2SPEEDOMETER_H
