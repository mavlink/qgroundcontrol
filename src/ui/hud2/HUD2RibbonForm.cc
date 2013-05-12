#include "HUD2RibbonForm.h"
#include "ui_HUD2RibbonForm.h"

HUD2RibbonForm::HUD2RibbonForm(HUD2Ribbon *ribbon, QWidget *parent) :
    QWidget(parent),
    ribbon(ribbon),
    ui(new Ui::HUD2RibbonForm)
{
    ui->setupUi(this);
}

HUD2RibbonForm::~HUD2RibbonForm()
{
    delete ui;
}

void HUD2RibbonForm::on_checkBoxNeedle_toggled(bool checked){
    this->ribbon->setOpacityNeedle(checked);
}
