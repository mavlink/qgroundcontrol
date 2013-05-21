#ifndef HUD2FORMCOMPASS_H
#define HUD2FORMCOMPASS_H

#include "HUD2FormRibbon.h"
#include "HUD2IndicatorCompass.h"

class HUD2FormCompass : public HUD2FormRibbon
{
public:
    HUD2FormCompass(HUD2IndicatorCompass *ribbon, QWidget *parent);
};

#endif // HUD2FORMCOMPASS_H
