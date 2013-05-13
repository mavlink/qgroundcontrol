#ifndef HUD2INDICATORCOMPASS_H
#define HUD2INDICATORCOMPASS_H

#include <QWidget>

#include "HUD2Data.h"
#include "HUD2Ribbon.h"


class HUD2IndicatorCompass : public HUD2Ribbon
{
    Q_OBJECT
public:
    explicit HUD2IndicatorCompass(const HUD2Data *huddata, QWidget *parent);
};



#endif // HUD2INDICATORCOMPASS_H
