#include "JoystickAxis.h"
#include "ui_JoystickAxis.h"
#include <QString>

JoystickAxis::JoystickAxis(int id, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::JoystickAxis)
{
    ui->setupUi(this);
    ui->label->setText(QString::number(id));
}

JoystickAxis::~JoystickAxis()
{
    delete ui;
}

void JoystickAxis::setValue(int value)
{
    ui->progressBar->setValue(value);
}
