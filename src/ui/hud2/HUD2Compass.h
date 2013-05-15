#ifndef HUD2COMPASS_H
#define HUD2COMPASS_H

#include "HUD2Ribbon.h"

class HUD2Compass : public HUD2Ribbon
{
public:
    HUD2Compass(const HUD2Data *huddata, QWidget *parent);
private:
    double processData(void);
};

#endif // HUD2COMPASS_H
