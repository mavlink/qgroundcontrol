#include <QSettings>

#include "HUD2RibbonForm.h"
#include "ui_HUD2RibbonForm.h"

HUD2RibbonForm::HUD2RibbonForm(HUD2Ribbon *ribbon, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2RibbonForm),
    ribbon(ribbon)
{
    ui->setupUi(this);
    ui->checkBoxNeedle->setChecked(ribbon->getOpacityNeedle());
    ui->checkBoxRibbon->setChecked(ribbon->getOpacityRibbon());
    ui->checkBoxEnable->setChecked(ribbon->getEnabled());

    ui->comboBox->addItem("airspeed");
    ui->comboBox->addItem("alt");
    ui->comboBox->addItem("groundspeed");
    ui->comboBox->addItem("pitch");
    ui->comboBox->addItem("roll");
    ui->comboBox->addItem("yaw");

    ui->comboBox->setCurrentIndex(ribbon->getValueIdx());
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

void HUD2RibbonForm::on_checkBoxEnable_toggled(bool checked){
    ribbon->setEnabled(checked);
}

void HUD2RibbonForm::on_comboBox_activated(int index){
    ribbon->setValuePtr(index);
}
