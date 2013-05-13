#include "HUD2FpsForm.h"
#include "ui_HUD2FpsForm.h"

HUD2FpsForm::HUD2FpsForm(HUD2IndicatorFps *fps, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2FpsForm),
    fps(fps)
{
    ui->setupUi(this);
    ui->checkBox->setChecked(fps->getEnabled());
}

HUD2FpsForm::~HUD2FpsForm()
{
    delete ui;
}

void HUD2FpsForm::on_checkBox_toggled(bool checked)
{
    fps->setEnabled(checked);
}
