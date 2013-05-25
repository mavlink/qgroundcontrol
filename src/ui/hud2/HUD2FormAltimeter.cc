#include "HUD2FormAltimeter.h"

HUD2FormAltimeter::HUD2FormAltimeter(HUD2IndicatorAltimeter *ribbon, QWidget *parent):
    HUD2FormRibbon(ribbon, parent),
    sourceLabel(new QLabel("What altitude to show")),
    unitsLabel(new QLabel("Units")),
    sourceComboBox(new QComboBox()),
    unitsComboBox(new QComboBox()),
    spacer(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding))
{
    int rows;

    sourceComboBox->addItem("barometer");
    sourceComboBox->addItem("GNSS");
    sourceComboBox->setCurrentIndex(ribbon->getSource());
    connect(sourceComboBox, SIGNAL(currentIndexChanged(int)), ribbon, SLOT(selectSource(int)));
    rows = ui->gridLayout->rowCount();
    ui->gridLayout->addWidget(sourceComboBox, rows, 0);
    ui->gridLayout->addWidget(sourceLabel, rows, 1);

    unitsComboBox->addItem("meters");
    unitsComboBox->addItem("feet");
    unitsComboBox->setCurrentIndex(ribbon->getUnits());
    connect(unitsComboBox, SIGNAL(currentIndexChanged(int)), ribbon, SLOT(selectUnits(int)));
    rows = ui->gridLayout->rowCount();
    ui->gridLayout->addWidget(unitsComboBox, rows, 0);
    ui->gridLayout->addWidget(unitsLabel, rows, 1);

    ui->verticalLayout->addSpacerItem(spacer);

    connect(this, SIGNAL(destroyed()), ribbon, SLOT(syncSettings()));
}
