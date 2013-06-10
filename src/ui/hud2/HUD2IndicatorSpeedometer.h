#ifndef HUD2SPEEDOMETER_H
#define HUD2SPEEDOMETER_H

#include "HUD2Ribbon.h"

typedef enum{
    SPEEDOMETER_AIR = 0,
    SPEEDOMETER_GROUND = 1
} speedometer_source_t;

typedef enum{
    SPEEDOMETER_UNITS_MS = 0,
    SPEEDOMETER_UNITS_KMH = 1
} speedometer_units_t;

class HUD2IndicatorSpeedometer : public HUD2Ribbon
{
    Q_OBJECT
public:
    HUD2IndicatorSpeedometer(const HUD2Data *huddata, QString name, QWidget *parent);
    speedometer_source_t getSource(void){return source;}
    speedometer_units_t getUnits(void){return units;}

public slots:
    void selectSource(int index){source = (speedometer_source_t)index;}
    void selectUnits(int index){units = (speedometer_units_t)index;}
    void syncSettings(void);

private:
    double processData(void);
    const QString &getLabelTop(void);
    const QString &getLabelBot(void);

private:
    const HUD2Data *huddata;
    speedometer_source_t source;
    speedometer_units_t units;
    QString label_top;
    QString label_bot;
};

#endif // HUD2SPEEDOMETER_H
