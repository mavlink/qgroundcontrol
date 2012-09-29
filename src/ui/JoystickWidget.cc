#include "JoystickWidget.h"
#include "ui_JoystickWidget.h"
#include <QDebug>

JoystickWidget::JoystickWidget(JoystickInput* joystick, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::JoystickWidget)
{
    m_ui->setupUi(this);
    this->joystick = joystick;

    m_ui->rollMapSpinBox->setValue(joystick->getMappingXAxis());
    m_ui->pitchMapSpinBox->setValue(joystick->getMappingYAxis());
    m_ui->yawMapSpinBox->setValue(joystick->getMappingYawAxis());
    m_ui->throttleMapSpinBox->setValue(joystick->getMappingThrustAxis());
    m_ui->autoMapSpinBox->setValue(joystick->getMappingAutoButton());

    connect(this->joystick, SIGNAL(joystickChanged(double,double,double,double,int,int)), this, SLOT(updateJoystick(double,double,double,double,int,int)));
    connect(this->joystick, SIGNAL(buttonPressed(int)), this, SLOT(pressKey(int)));
    connect(m_ui->rollMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingXAxis(int)));
    connect(m_ui->pitchMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingYAxis(int)));
    connect(m_ui->yawMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingYawAxis(int)));
    connect(m_ui->throttleMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingThrustAxis(int)));
    connect(m_ui->autoMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingAutoButton(int)));

    // Display the widget
    this->window()->setWindowTitle(tr("Joystick Settings"));
    if (joystick) updateStatus(tr("Found joystick: %1").arg(joystick->getName()));

    this->show();
}

JoystickWidget::~JoystickWidget()
{
    delete m_ui;
}

void JoystickWidget::updateJoystick(double roll, double pitch, double yaw, double thrust, int xHat, int yHat)
{
    setX(roll);
    setY(pitch);
    setZ(yaw);
    setThrottle(thrust);
    setHat(xHat, yHat);
}

void JoystickWidget::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


void JoystickWidget::setThrottle(float thrust)
{
    m_ui->thrust->setValue(thrust*100);
}

void JoystickWidget::setX(float x)
{
    m_ui->xSlider->setValue(x*100);
    m_ui->xValue->display(x*100);
}

void JoystickWidget::setY(float y)
{
    m_ui->ySlider->setValue(y*100);
    m_ui->yValue->display(y*100);
}

void JoystickWidget::setZ(float z)
{
    m_ui->dial->setValue(z*100);
}

void JoystickWidget::setHat(float x, float y)
{
    updateStatus(tr("Hat position: x: %1, y: %2").arg(x).arg(y));
}

void JoystickWidget::clearKeys()
{
    QString colorstyle;
    QColor buttonStyleColor = QColor(200, 20, 20);
    colorstyle = QString("QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: %1;}").arg(buttonStyleColor.name());

    m_ui->buttonLabel0->setStyleSheet(colorstyle);
    m_ui->buttonLabel1->setStyleSheet(colorstyle);
    m_ui->buttonLabel2->setStyleSheet(colorstyle);
    m_ui->buttonLabel3->setStyleSheet(colorstyle);
    m_ui->buttonLabel4->setStyleSheet(colorstyle);
    m_ui->buttonLabel5->setStyleSheet(colorstyle);
    m_ui->buttonLabel6->setStyleSheet(colorstyle);
    m_ui->buttonLabel7->setStyleSheet(colorstyle);
    m_ui->buttonLabel8->setStyleSheet(colorstyle);
    m_ui->buttonLabel9->setStyleSheet(colorstyle);
}

void JoystickWidget::pressKey(int key)
{
    QString colorstyle;
    QColor buttonStyleColor = QColor(20, 200, 20);
    colorstyle = QString("QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: %1;}").arg(buttonStyleColor.name());
    switch(key) {
    case 0:
        m_ui->buttonLabel0->setStyleSheet(colorstyle);
        break;
    case 1:
        m_ui->buttonLabel1->setStyleSheet(colorstyle);
        break;
    case 2:
        m_ui->buttonLabel2->setStyleSheet(colorstyle);
        break;
    case 3:
        m_ui->buttonLabel3->setStyleSheet(colorstyle);
        break;
    case 4:
        m_ui->buttonLabel4->setStyleSheet(colorstyle);
        break;
    case 5:
        m_ui->buttonLabel5->setStyleSheet(colorstyle);
        break;
    case 6:
        m_ui->buttonLabel6->setStyleSheet(colorstyle);
        break;
    case 7:
        m_ui->buttonLabel7->setStyleSheet(colorstyle);
        break;
    case 8:
        m_ui->buttonLabel8->setStyleSheet(colorstyle);
        break;
    case 9:
        m_ui->buttonLabel9->setStyleSheet(colorstyle);
        break;
    }
    QTimer::singleShot(20, this, SLOT(clearKeys()));
    updateStatus(tr("Key %1 pressed").arg(key));
}

void JoystickWidget::updateStatus(const QString& status)
{
    m_ui->statusLabel->setText(status);
}
