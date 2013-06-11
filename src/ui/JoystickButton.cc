#include "JoystickButton.h"
#include "ui_JoystickButton.h"

JoystickButton::JoystickButton(int id, QWidget *parent) :
    QWidget(parent),
    id(id),
    m_ui(new Ui::JoystickButton)
{
    m_ui->setupUi(this);
    m_ui->joystickButtonLabel->setText(QString::number(id));
}

JoystickButton::~JoystickButton()
{
    delete m_ui;
}
