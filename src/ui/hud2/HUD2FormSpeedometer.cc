#include "HUD2FormSpeedometer.h"

HUD2FormSpeedometer::HUD2FormSpeedometer(HUD2IndicatorSpeedometer *ribbon, QWidget *parent):
    HUD2FormRibbon(ribbon, parent),
    sourceLabel(new QLabel("What speed to show")),
    unitsLabel(new QLabel("Units")),
    sourceComboBox(new QComboBox()),
    unitsComboBox(new QComboBox()),
    spacer(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding))
{
    int rows;

    sourceComboBox->addItem("air");
    sourceComboBox->addItem("ground");
    sourceComboBox->setCurrentIndex(ribbon->getSource());
    connect(sourceComboBox, SIGNAL(currentIndexChanged(int)), ribbon, SLOT(selectSource(int)));
    rows = ui->gridLayout->rowCount();
    ui->gridLayout->addWidget(sourceComboBox, rows, 0);
    ui->gridLayout->addWidget(sourceLabel, rows, 1);

    unitsComboBox->addItem("m/s");
    unitsComboBox->addItem("km/h");
    unitsComboBox->setCurrentIndex(ribbon->getUnits());
    connect(unitsComboBox, SIGNAL(currentIndexChanged(int)), ribbon, SLOT(selectUnits(int)));
    rows = ui->gridLayout->rowCount();
    ui->gridLayout->addWidget(unitsComboBox, rows, 0);
    ui->gridLayout->addWidget(unitsLabel, rows, 1);

    ui->verticalLayout->addSpacerItem(spacer);

    connect(this, SIGNAL(destroyed()), ribbon, SLOT(syncSettings()));
}

