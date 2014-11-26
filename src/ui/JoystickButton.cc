#include "JoystickButton.h"
#include "ui_JoystickButton.h"
#include "UASManager.h"

JoystickButton::JoystickButton(int id, QWidget *parent) :
    QWidget(parent),
    id(id),
    m_ui(new Ui::JoystickButton)
{
    m_ui->setupUi(this);
    m_ui->joystickButtonLabel->setText(QString::number(id));
    this->setActiveUAS(qgcApp()->singletonUASManager()->getActiveUAS());
    connect(m_ui->joystickAction, SIGNAL(currentIndexChanged(int)), this, SLOT(actionComboBoxChanged(int)));
}

JoystickButton::~JoystickButton()
{
    delete m_ui;
}

void JoystickButton::setActiveUAS(UASInterface* uas)
{
    // Disable signals so that changes to joystickAction don't trigger JoystickInput updates.
    disconnect(m_ui->joystickAction, SIGNAL(currentIndexChanged(int)), this, SLOT(actionComboBoxChanged(int)));
    if (uas)
    {
        m_ui->joystickAction->setEnabled(true);
        m_ui->joystickAction->clear();
        m_ui->joystickAction->addItem("--");
        QList<QAction*> actions = uas->getActions();
        foreach (QAction* a, actions)
        {
            m_ui->joystickAction->addItem(a->text());
        }
        m_ui->joystickAction->setCurrentIndex(0);
    } else {
        m_ui->joystickAction->setEnabled(false);
        m_ui->joystickAction->clear();
        m_ui->joystickAction->addItem("--");
        m_ui->joystickAction->setCurrentIndex(0);
    }
    connect(m_ui->joystickAction, SIGNAL(currentIndexChanged(int)), this, SLOT(actionComboBoxChanged(int)));
}

void JoystickButton::setAction(int index)
{
    // Disable signals so that changes to joystickAction don't trigger JoystickInput updates.
    disconnect(m_ui->joystickAction, SIGNAL(currentIndexChanged(int)), this, SLOT(actionComboBoxChanged(int)));
    // Add one because the default no-action takes the 0-spot.
    m_ui->joystickAction->setCurrentIndex(index + 1);
    connect(m_ui->joystickAction, SIGNAL(currentIndexChanged(int)), this, SLOT(actionComboBoxChanged(int)));
}

void JoystickButton::actionComboBoxChanged(int index)
{
    // Subtract one because the default no-action takes the 0-spot.
    emit actionChanged(id, index - 1);
}
