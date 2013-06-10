#ifndef HUD2ALTIMETER_H
#define HUD2ALTIMETER_H
#include "HUD2Ribbon.h"

typedef enum{
    ALTIMETER_BARO = 0,
    ALTIMETER_GNSS = 1
} altimeter_source_t;

typedef enum{
    ALTIMETER_UNITS_M = 0,
    ALTIMETER_UNITS_F = 1
} altimeter_units_t;

class HUD2IndicatorAltimeter : public HUD2Ribbon
{
    Q_OBJECT
public:
    HUD2IndicatorAltimeter(const HUD2Data *huddata, QString name, QWidget *parent);
    altimeter_source_t getSource(void){return source;}
    altimeter_units_t getUnits(void){return units;}

public slots:
    void selectSource(int index){source = (altimeter_source_t)index;}
    void selectUnits(int index){units = (altimeter_units_t)index;}
    void syncSettings(void);

private:
    double processData(void);
    const QString &getLabelTop(void);
    const QString &getLabelBot(void);

private:
    const HUD2Data *huddata;
    altimeter_source_t source;
    altimeter_units_t units;
    QString label_top;
    QString label_bot;
};

#endif // HUD2ALTIMETER_H

