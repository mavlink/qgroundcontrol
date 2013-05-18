#ifndef HUD2ALTIMETER_H
#define HUD2ALTIMETER_H
#include "HUD2Ribbon.h"

class HUD2IndicatorAltimeter : public HUD2Ribbon
{
public:
    HUD2IndicatorAltimeter(const HUD2Data *huddata, QWidget *parent);
private:
    double processData(void);
};

#endif // HUD2ALTIMETER_H
