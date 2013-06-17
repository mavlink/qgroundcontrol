#include "JoystickAxis.h"
#include "JoystickInput.h"
#include "ui_JoystickAxis.h"
#include "UASManager.h"
#include <QString>

JoystickAxis::JoystickAxis(int id, QWidget *parent) :
    QWidget(parent),
    id(id),
    ui(new Ui::JoystickAxis)
{
    ui->setupUi(this);
    mappingComboBoxChanged(JoystickInput::JOYSTICK_INPUT_MAPPING_NONE);
    ui->label->setText(QString::number(id));
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(mappingComboBoxChanged(int)));
    connect(ui->invertedCheckBox, SIGNAL(clicked(bool)), this, SLOT(inversionCheckBoxChanged(bool)));
    connect(ui->rangeCheckBox, SIGNAL(clicked(bool)), this, SLOT(rangeCheckBoxChanged(bool)));
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
    if (newMapping == JoystickInput::JOYSTICK_INPUT_MAPPING_THROTTLE)
    {
        ui->rangeCheckBox->show();
    }
    else
    {
        ui->rangeCheckBox->hide();
    }
    emit mappingChanged(id, (JoystickInput::JOYSTICK_INPUT_MAPPING)newMapping);
    this->setActiveUAS(UASManager::instance()->getActiveUAS());
}

void JoystickAxis::inversionCheckBoxChanged(bool inverted)
{
    emit inversionChanged(id, inverted);
}

void JoystickAxis::rangeCheckBoxChanged(bool limited)
{
    emit rangeChanged(id, limited);
}

void JoystickAxis::setActiveUAS(UASInterface* uas)
{
    // Set the throttle display to only positive if:
    // * This is the throttle axis AND
    // * The current UAS can't reverse OR there is no current UAS
    if (((uas && !uas->systemCanReverse()) || !uas) && ui->comboBox->currentIndex() == JoystickInput::JOYSTICK_INPUT_MAPPING_THROTTLE)
    {
        ui->progressBar->setRange(0, 100);
        ui->rangeCheckBox->show();
    }
    else
    {
        ui->progressBar->setRange(-100, 100);
        ui->rangeCheckBox->hide();
    }
}
