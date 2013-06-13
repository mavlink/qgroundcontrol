#include "JoystickAxis.h"
#include "JoystickInput.h"
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
    connect(ui->checkBox, SIGNAL(clicked(bool)), this, SLOT(inversionCheckBoxChanged(bool)));
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
    emit mappingChanged(id, (JoystickInput::JOYSTICK_INPUT_MAPPING)newMapping);
}

void JoystickAxis::inversionCheckBoxChanged(bool inverted)
{
    emit inversionChanged(id, inverted);
}
