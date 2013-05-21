#include "HUD2FormCompass.h"
#include "ui_HUD2FormRibbon.h"

HUD2FormCompass::HUD2FormCompass(HUD2IndicatorCompass *ribbon, QWidget *parent):
    HUD2FormRibbon(ribbon, parent)
{
    QSpacerItem *spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    ui->verticalLayout->addSpacerItem(spacer);
}
