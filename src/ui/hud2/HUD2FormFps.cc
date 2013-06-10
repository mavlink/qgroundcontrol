#include "HUD2FormFps.h"
#include "ui_HUD2FormFps.h"

HUD2FormFps::HUD2FormFps(HUD2IndicatorFps *fps, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2FormFps),
    fps(fps)
{
    ui->setupUi(this);
    ui->checkBox->setChecked(fps->getEnabled());
}

HUD2FormFps::~HUD2FormFps()
{
    delete ui;
}

void HUD2FormFps::on_checkBox_toggled(bool checked)
{
    fps->setEnabled(checked);
}
