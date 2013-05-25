#ifndef HUD2FORMALTIMETER_H
#define HUD2FORMALTIMETER_H

#include "HUD2FormRibbon.h"
#include "HUD2IndicatorAltimeter.h"
#include "ui_HUD2FormRibbon.h"

class HUD2FormAltimeter : public HUD2FormRibbon
{
public:
    HUD2FormAltimeter(HUD2IndicatorAltimeter *ribbon, QWidget *parent);
private:
    QLabel *sourceLabel;
    QLabel *unitsLabel;
    QComboBox *sourceComboBox;
    QComboBox *unitsComboBox;
    QSpacerItem *spacer;
};

#endif // HUD2FORMALTIMETER_H
