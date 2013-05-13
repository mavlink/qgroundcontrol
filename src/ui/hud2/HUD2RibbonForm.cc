#include "HUD2RibbonForm.h"
#include "ui_HUD2RibbonForm.h"

HUD2RibbonForm::HUD2RibbonForm(HUD2Ribbon *ribbon, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2RibbonForm),
    ribbon(ribbon)
{
    ui->setupUi(this);
}

HUD2RibbonForm::~HUD2RibbonForm()
{
    delete ui;
}

void HUD2RibbonForm::on_checkBoxNeedle_toggled(bool checked){
    ribbon->setOpacityNeedle(checked);
}

void HUD2RibbonForm::on_checkBoxRibbon_toggled(bool checked){
    ribbon->setOpacityRibbon(checked);
}
