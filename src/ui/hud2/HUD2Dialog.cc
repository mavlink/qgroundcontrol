#include "HUD2Dialog.h"
#include "ui_HUD2Dialog.h"

HUD2Dialog::HUD2Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HUD2Dialog)
{
    ui->setupUi(this);
}

HUD2Dialog::~HUD2Dialog()
{
    delete ui;
}
