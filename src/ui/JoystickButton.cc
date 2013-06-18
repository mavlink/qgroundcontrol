#include "JoystickButton.h"
#include "ui_JoystickButton.h"

JoystickButton::JoystickButton(int id, QWidget *parent) :
    QWidget(parent),
    id(id),
    m_ui(new Ui::JoystickButton)
{
    m_ui->setupUi(this);
    m_ui->joystickButtonLabel->setText(QString::number(id));
    connect(m_ui->joystickAction, SIGNAL(currentIndexChanged(int)), this, SLOT(actionComboBoxChanged(int)));
}

JoystickButton::~JoystickButton()
{
    delete m_ui;
}

void JoystickButton::setActiveUAS(UASInterface* uas)
{
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
    } else {
        m_ui->joystickAction->setEnabled(false);
        m_ui->joystickAction->clear();
        m_ui->joystickAction->addItem("--");
    }
}

void JoystickButton::actionComboBoxChanged(int index)
{
    emit actionChanged(id, index);
}
