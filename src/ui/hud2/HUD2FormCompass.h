#ifndef HUD2FORMCOMPASS_H
#define HUD2FORMCOMPASS_H

#include "HUD2FormRibbon.h"
#include "HUD2IndicatorCompass.h"
#include "ui_HUD2FormRibbon.h"

class HUD2FormCompass : public HUD2FormRibbon
{
public:
    HUD2FormCompass(HUD2IndicatorCompass *ribbon, QWidget *parent);
private:
    QSpacerItem *spacer;
};

#endif // HUD2FORMCOMPASS_H
