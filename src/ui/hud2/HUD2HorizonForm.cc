#include <QSettings>

#include "HUD2HorizonForm.h"
#include "ui_HUD2HorizonForm.h"

HUD2HorizonForm::HUD2HorizonForm(HUD2IndicatorHorizon *horizon, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2HorizonForm),
    horizon(horizon)
{
    ui->setupUi(this);
    ui->checkBox->setChecked(horizon->getColoredBg());
}

HUD2HorizonForm::~HUD2HorizonForm()
{
    delete ui;
}

void HUD2HorizonForm::on_checkBox_toggled(bool checked)
{
    QSettings settings;
    horizon->setColoredBg(checked);

    settings.beginGroup("QGC_HUD2");
    settings.setValue("HORIZON_COLORED_BG", checked);
    settings.endGroup();
}
