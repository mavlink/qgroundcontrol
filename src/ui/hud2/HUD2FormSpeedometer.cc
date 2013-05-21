#include "HUD2FormSpeedometer.h"
#include "ui_HUD2FormRibbon.h"

HUD2FormSpeedometer::HUD2FormSpeedometer(HUD2IndicatorSpeedometer *ribbon, QWidget *parent):
    HUD2FormRibbon(ribbon, parent)
{
    QLabel *sourceLabel = new QLabel("What speed to show");
    QComboBox *sourceComboBox = new QComboBox();
    QSpacerItem *spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    sourceComboBox->addItem("ground");
    sourceComboBox->addItem("air");

    connect(sourceComboBox, SIGNAL(currentIndexChanged(int)), ribbon, SLOT(selectSource(int)));
    int rows = ui->gridLayout->rowCount();

    ui->gridLayout->addWidget(sourceComboBox, rows, 0);
    ui->gridLayout->addWidget(sourceLabel, rows, 1);

    ui->verticalLayout->addSpacerItem(spacer);
}
