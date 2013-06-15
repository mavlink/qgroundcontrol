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
    ui->limitRangeCheckBox->hide(); // Hide the range checkbox by default. It's only activated by switching to the Throttle axis option.
    ui->label->setText(QString::number(id));
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(mappingComboBoxChanged(int)));
    connect(ui->invertedCheckBox, SIGNAL(clicked(bool)), this, SLOT(inversionCheckBoxChanged(bool)));
    connect(ui->limitRangeCheckBox, SIGNAL(clicked(bool)), this, SLOT(limitRangeCheckBoxChanged(bool)));
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
        ui->limitRangeCheckBox->show();
    }
    else
    {
        ui->limitRangeCheckBox->hide();
    }
    emit mappingChanged(id, (JoystickInput::JOYSTICK_INPUT_MAPPING)newMapping);
}

void JoystickAxis::inversionCheckBoxChanged(bool inverted)
{
    emit inversionChanged(id, inverted);
}

void JoystickAxis::limitRangeCheckBoxChanged(bool limited)
{
    if (limited)
    {
        ui->progressBar->setRange(0, 100);
    }
    else
    {
        ui->progressBar->setRange(-100, 100);
    }
    emit rangeLimitChanged(id, limited);
}
