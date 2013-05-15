#ifndef HUD2ALTIMETER_H
#define HUD2ALTIMETER_H
#include "HUD2Ribbon.h"

class HUD2Altimeter : public HUD2Ribbon
{
public:
    HUD2Altimeter(const HUD2Data *huddata, QWidget *parent);
private:
    double processData(void);
};

#endif // HUD2ALTIMETER_H
