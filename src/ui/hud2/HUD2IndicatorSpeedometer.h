#ifndef HUD2SPEEDOMETER_H
#define HUD2SPEEDOMETER_H

#include "HUD2Ribbon.h"

typedef enum{
    SPEEDOMETER_AIR = 0,
    SPEEDOMETER_GROUND = 1
} speedometer_source_t;

class HUD2IndicatorSpeedometer : public HUD2Ribbon
{
    Q_OBJECT
public:
    HUD2IndicatorSpeedometer(const HUD2Data *huddata, QWidget *parent);

public slots:
    void selectSource(int index);

private:
    double processData(void);

private:
    speedometer_source_t src;
    const HUD2Data *huddata;
};

#endif // HUD2SPEEDOMETER_H
