#include "JoystickAxis.h"
#include "JoystickInput.h"
#include "ui_JoystickAxis.h"
<<<<<<< HEAD
#include "UASManager.h"
=======
>>>>>>> 64d0741ee82db3d5dac2108f3f6d03c6fa66e5c6
#include <QString>

JoystickAxis::JoystickAxis(int id, QWidget *parent) :
    QWidget(parent),
    id(id),
    ui(new Ui::JoystickAxis)
{
    ui->setupUi(this);
    ui->label->setText(QString::number(id));
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(mappingComboBoxChanged(int)));
    connect(ui->invertedCheckBox, SIGNAL(clicked(bool)), this, SLOT(inversionCheckBoxChanged(bool)));
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
    this->setActiveUAS(UASManager::instance()->getActiveUAS());
}

void JoystickAxis::inversionCheckBoxChanged(bool inverted)
{
    emit inversionChanged(id, inverted);
}

void JoystickAxis::setActiveUAS(UASInterface* uas)
{
    if (uas && !uas->systemCanReverse() && ui->comboBox->currentIndex() == JoystickInput::JOYSTICK_INPUT_MAPPING_THROTTLE)
    {
        ui->progressBar->setRange(0, 100);
    }
    else
    {
        ui->progressBar->setRange(-100, 100);
    }
}
