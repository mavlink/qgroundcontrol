#ifndef HUD2ALTIMETER_H
#define HUD2ALTIMETER_H
#include "HUD2Ribbon.h"

typedef enum{
    ALTIMETER_BARO = 0,
    ALTIMETER_GNSS = 1
} altimeter_source_t;

class HUD2IndicatorAltimeter : public HUD2Ribbon
{
    Q_OBJECT
public:
    HUD2IndicatorAltimeter(const HUD2Data *huddata, QWidget *parent);

public slots:
    void selectSource(int index);

private:
    double processData(void);
    const HUD2Data *huddata;
    altimeter_source_t src;
};

#endif // HUD2ALTIMETER_H
