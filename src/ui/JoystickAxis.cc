#include "JoystickAxis.h"
#include "ui_JoystickAxis.h"
#include <QString>

JoystickAxis::JoystickAxis(int id, QWidget *parent) :
    QWidget(parent),
    id(id),
    ui(new Ui::JoystickAxis)
{
    ui->setupUi(this);
    ui->label->setText(QString::number(id));
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(mappingComboBoxChanged(int)));
}

JoystickAxis::~JoystickAxis()
{
    delete ui;
}

void JoystickAxis::setValue(float value)
{
    ui->progressBar->setValue(100.0f * value);
}

void JoystickAxis::mappingComboBoxChanged(int newMapping)
{
    emit mappingChanged(id, newMapping);
}
