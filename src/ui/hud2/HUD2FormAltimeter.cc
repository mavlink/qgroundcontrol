#include "HUD2FormAltimeter.h"
#include "ui_HUD2FormRibbon.h"

HUD2FormAltimeter::HUD2FormAltimeter(HUD2Ribbon *ribbon, QWidget *parent):
    HUD2FormRibbon(ribbon, parent)
{

    QLabel *sourceLabel = new QLabel("What altitude to show");
    QComboBox *sourceComboBox = new QComboBox();
    QSpacerItem *spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    sourceComboBox->addItem("barometer");
    sourceComboBox->addItem("GNSS");

    connect(sourceComboBox, SIGNAL(currentIndexChanged(int)), ribbon, SLOT(selectSource(int)));
    int rows = ui->gridLayout->rowCount();

    ui->gridLayout->addWidget(sourceComboBox, rows, 0);
    ui->gridLayout->addWidget(sourceLabel, rows, 1);

    ui->verticalLayout->addSpacerItem(spacer);
}
