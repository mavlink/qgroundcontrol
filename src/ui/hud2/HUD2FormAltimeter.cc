#include "HUD2FormAltimeter.h"
#include "ui_HUD2FormRibbon.h"

HUD2FormAltimeter::HUD2FormAltimeter(HUD2Ribbon *ribbon, QWidget *parent):
    HUD2FormRibbon(ribbon, parent)
{
    QCheckBox *climbCheckBox = new QCheckBox();
    climbCheckBox->setText("Climb meter");

    ui->verticalLayout->addWidget(climbCheckBox);

    ui->verticalLayout->addSpacerItem(new
            QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}
