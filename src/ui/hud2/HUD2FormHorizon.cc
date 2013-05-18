#include <QSettings>

#include "HUD2FormHorizon.h"
#include "ui_HUD2FormHorizon.h"

HUD2FormHorizon::HUD2FormHorizon(HUD2IndicatorHorizon *horizon, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2FormHorizon),
    horizon(horizon)
{
    ui->setupUi(this);
    ui->checkBox->setChecked(horizon->getColoredBg());
}

HUD2FormHorizon::~HUD2FormHorizon()
{
    delete ui;
}

void HUD2FormHorizon::on_checkBox_toggled(bool checked)
{
    QSettings settings;
    horizon->setColoredBg(checked);

    settings.beginGroup("QGC_HUD2");
    settings.setValue("HORIZON_COLORED_BG", checked);
    settings.endGroup();
}
