#include "JoystickAxis.h"
#include "JoystickInput.h"
#include "ui_JoystickAxis.h"
#include "MultiVehicleManager.h"
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

void JoystickAxis::setMapping(JoystickInput::JOYSTICK_INPUT_MAPPING newMapping)
{
    ui->comboBox->setCurrentIndex(newMapping);
    if (newMapping == JoystickInput::JOYSTICK_INPUT_MAPPING_THROTTLE)
    {
        ui->rangeCheckBox->show();
    }
    else
    {
        ui->rangeCheckBox->hide();
    }
    this->activeVehicleChanged(MultiVehicleManager::instance()->activeVehicle());
}

void JoystickAxis::setInverted(bool newValue)
{
    ui->invertedCheckBox->setChecked(newValue);
}

void JoystickAxis::setRangeLimit(bool newValue)
{
    ui->rangeCheckBox->setChecked(newValue);
}

void JoystickAxis::mappingComboBoxChanged(int newMapping)
{
    JoystickInput::JOYSTICK_INPUT_MAPPING mapping = (JoystickInput::JOYSTICK_INPUT_MAPPING)newMapping;
    emit mappingChanged(id, mapping);
    updateUIBasedOnUAS(MultiVehicleManager::instance()->activeVehicle(), mapping);
}

void JoystickAxis::inversionCheckBoxChanged(bool inverted)
{
    emit inversionChanged(id, inverted);
}

void JoystickAxis::rangeCheckBoxChanged(bool limited)
{
    emit rangeChanged(id, limited);
}

void JoystickAxis::activeVehicleChanged(Vehicle* vehicle)
{
    updateUIBasedOnUAS(vehicle, (JoystickInput::JOYSTICK_INPUT_MAPPING)ui->comboBox->currentIndex());
}

void JoystickAxis::updateUIBasedOnUAS(Vehicle* vehicle, JoystickInput::JOYSTICK_INPUT_MAPPING axisMapping)
{
    UAS* uas = NULL;
    
    if (vehicle) {
        uas = vehicle->uas();
    }
    
    // Set the throttle display to only positive if:
    // * This is the throttle axis AND
    // * The current UAS can't reverse OR there is no current UAS
    // This causes us to default to systems with no negative throttle.
    if (((uas && !uas->systemCanReverse()) || !uas) && axisMapping == JoystickInput::JOYSTICK_INPUT_MAPPING_THROTTLE)
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
