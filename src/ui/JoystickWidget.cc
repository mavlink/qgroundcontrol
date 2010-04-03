#include "JoystickWidget.h"
#include "ui_JoystickWidget.h"
#include <QDebug>

JoystickWidget::JoystickWidget(JoystickInput* joystick, QWidget *parent) :
        QDialog(parent),
        m_ui(new Ui::JoystickWidget)
{
    m_ui->setupUi(this);
    this->joystick = joystick;


    connect(this->joystick, SIGNAL(joystickChanged(double,double,double,double,int,int)), this, SLOT(updateJoystick(double,double,double,double,int,int)));
    connect(this->joystick, SIGNAL(buttonPressed(int)), this, SLOT(pressKey(int)));

    // Display the widget
    this->window()->setWindowTitle(tr("Joystick Settings"));
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
    setThrottle(thrust);
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
void JoystickWidget::pressKey(int key)
{
    QString colorstyle;
    QColor heartbeatColor = QColor(20, 200, 20);
    colorstyle = colorstyle.sprintf("QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X;}",
                                    heartbeatColor.red(), heartbeatColor.green(), heartbeatColor.blue());
    m_ui->button0Label->setStyleSheet(colorstyle);
    m_ui->button0Label->setAutoFillBackground(true);
    qDebug() << "KEY" << key << " pressed on joystick";
}
