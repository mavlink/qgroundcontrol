#include "JoystickWidget.h"
#include "ui_JoystickWidget.h"
#include <QDebug>
#include <QDesktopWidget>

JoystickWidget::JoystickWidget(JoystickInput* joystick, QWidget *parent) :
    QDialog(parent),
    joystick(joystick),
    m_ui(new Ui::JoystickWidget)
{
    m_ui->setupUi(this);

    // Center the window on the screen.
    QRect position = frameGeometry();
    position.moveCenter(QDesktopWidget().availableGeometry().center());
    move(position.topLeft());

    // Initialize the UI based on the current joystick
    initUI();

    // Watch for input events from the joystick
    connect(this->joystick, SIGNAL(joystickChanged(double,double,double,double,int,int,int)), this, SLOT(updateJoystick(double,double,double,double,int,int)));
    connect(this->joystick, SIGNAL(buttonPressed(int)), this, SLOT(pressKey(int)));

    // Watch for changes to the button/axis mappings
    connect(m_ui->rollMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingXAxis(int)));
    connect(m_ui->pitchMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingYAxis(int)));
    connect(m_ui->yawMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingYawAxis(int)));
    connect(m_ui->throttleMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingThrustAxis(int)));
    connect(m_ui->autoMapSpinBox, SIGNAL(valueChanged(int)), this->joystick, SLOT(setMappingAutoButton(int)));

    // Update the UI if the joystick changes.
    connect(m_ui->joystickNameComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUIForJoystick(int)));

    // Display the widget above all other windows.
    this->raise();
    this->show();
}

void JoystickWidget::initUI()
{
    // Add the joysticks to the top combobox. They're indexed by their item number.
    // And set the currently-selected combobox item to the current joystick.
    int joysticks = joystick->getNumJoysticks();
    if (joysticks)
    {
        for (int i = 0; i < joysticks; i++)
        {
            m_ui->joystickNameComboBox->addItem(joystick->getJoystickNameById(i));
        }
        m_ui->joystickNameComboBox->setCurrentIndex(joystick->getJoystickID());
    }
    else
    {
        m_ui->joystickNameComboBox->addItem(tr("No joysticks found. Connect and restart QGC to add one."));
    }

    // Add any missing buttons
    updateUIForJoystick(joystick->getJoystickID());
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

void JoystickWidget::updateUIForJoystick(int id)
{
    // Delete all the old buttonlabels
    foreach (QLabel* l, m_ui->buttonLabelBox->findChildren<QLabel*>())
    {
        delete l;
    }

    // Set the JoystickInput to listen to the new joystick instead.
    joystick->setActiveJoystick(id);

    // And add new ones for every new button found.
    for (int i = 0; i < joystick->getJoystickNumButtons(); i++)
    {
        QLabel* buttonLabel = new QLabel(m_ui->buttonLabelBox);
        buttonLabel->setText(QString::number(i));
        buttonLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        buttonLabel->setAlignment(Qt::AlignCenter);
        // And make sure we insert BEFORE the vertical spacer.
        m_ui->buttonLabelLayout->insertWidget(i, buttonLabel);
    }

    // Update the mapping UI
    m_ui->rollMapSpinBox->setValue(joystick->getMappingXAxis());
    m_ui->pitchMapSpinBox->setValue(joystick->getMappingYAxis());
    m_ui->yawMapSpinBox->setValue(joystick->getMappingYawAxis());
    m_ui->throttleMapSpinBox->setValue(joystick->getMappingThrustAxis());
    m_ui->autoMapSpinBox->setValue(joystick->getMappingAutoButton());
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

void JoystickWidget::pressKey(int key)
{
//    QString colorstyle;
//    QColor buttonStyleColor = QColor(20, 200, 20);
//    colorstyle = QString("QLabel { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: %1;}").arg(buttonStyleColor.name());
//    switch(key) {
//    case 0:
//        m_ui->button0->setStyleSheet(colorstyle);
//        break;
//    case 1:
//        m_ui->button1->setStyleSheet(colorstyle);
//        break;
//    case 2:
//        m_ui->button2->setStyleSheet(colorstyle);
//        break;
//    case 3:
//        m_ui->button3->setStyleSheet(colorstyle);
//        break;
//    case 4:
//        m_ui->button4->setStyleSheet(colorstyle);
//        break;
//    case 5:
//        m_ui->button5->setStyleSheet(colorstyle);
//        break;
//    case 6:
//        m_ui->button6->setStyleSheet(colorstyle);
//        break;
//    case 7:
//        m_ui->button7->setStyleSheet(colorstyle);
//        break;
//    case 8:
//        m_ui->button8->setStyleSheet(colorstyle);
//        break;
//    case 9:
//        m_ui->button9->setStyleSheet(colorstyle);
//        break;
//    case 10:
//        m_ui->button10->setStyleSheet(colorstyle);
//        break;
//    }
    updateStatus(tr("Key %1 pressed").arg(key));
}

void JoystickWidget::updateStatus(const QString& status)
{
    m_ui->statusLabel->setText(status);
}
