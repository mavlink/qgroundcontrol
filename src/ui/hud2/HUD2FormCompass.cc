#include "HUD2FormCompass.h"

HUD2FormCompass::HUD2FormCompass(HUD2IndicatorCompass *ribbon, QWidget *parent):
    HUD2FormRibbon(ribbon, parent),
    spacer(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding))
{
    ui->verticalLayout->addSpacerItem(spacer);
    connect(this, SIGNAL(destroyed()), ribbon, SLOT(syncSettings()));
}
