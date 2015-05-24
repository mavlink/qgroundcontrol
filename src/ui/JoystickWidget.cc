#include "JoystickWidget.h"
#include "QGCApplication.h"
#include "ui_JoystickWidget.h"
#include "JoystickButton.h"
#include "JoystickAxis.h"

#include <QDesktopWidget>

JoystickWidget::JoystickWidget(JoystickInput* joystick, QWidget *parent) :
    QWidget(parent),
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

    // Watch for button, axis, and hat input events from the joystick.
    connect(this->joystick, SIGNAL(buttonPressed(int)), this, SLOT(joystickButtonPressed(int)));
    connect(this->joystick, SIGNAL(buttonReleased(int)), this, SLOT(joystickButtonReleased(int)));
    connect(this->joystick, SIGNAL(axisValueChanged(int,float)), this, SLOT(updateAxisValue(int,float)));
    connect(this->joystick, SIGNAL(hatDirectionChanged(qint8,qint8)), this, SLOT(setHat(qint8,qint8)));

    // Also watch for when new settings were loaded for the current joystick to do a mass UI refresh.
    connect(this->joystick, SIGNAL(joystickSettingsChanged()), this, SLOT(updateUI()));

    // If the selected joystick is changed, update the JoystickInput.
    connect(m_ui->joystickNameComboBox, SIGNAL(currentIndexChanged(int)), this->joystick, SLOT(setActiveJoystick(int)));
    // Also wait for the JoystickInput to switch, then update our UI.
    connect(this->joystick, SIGNAL(newJoystickSelected()), this, SLOT(createUIForJoystick()));

    // Initialize the UI to the current JoystickInput state. Also make sure to listen for future changes
    // so that the UI can be updated.
    connect(m_ui->enableCheckBox, SIGNAL(toggled(bool)), m_ui->joystickFrame, SLOT(setEnabled(bool)));
    m_ui->enableCheckBox->setChecked(this->joystick->enabled()); // Needs to be after connecting to the joystick frame and before watching for enabled events from JoystickInput.
    connect(m_ui->enableCheckBox, SIGNAL(toggled(bool)), this->joystick, SLOT(setEnabled(bool)));

    // Update the button label colors based on the current theme and watch for future theme changes.
    styleChanged(qgcApp()->styleIsDark());
    connect(qgcApp(), &QGCApplication::styleChanged, this, &JoystickWidget::styleChanged);

    // change mode when mode combobox is changed
    connect(m_ui->joystickModeComboBox, SIGNAL(currentIndexChanged(int)), this->joystick, SLOT(setMode(int)));

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
        // And if joystick support is enabled, show the UI.
        if (m_ui->enableCheckBox->isChecked())
        {
            m_ui->joystickFrame->setEnabled(true);
        }

        // mode combo box
        m_ui->joystickModeComboBox->addItem("Attitude");
        m_ui->joystickModeComboBox->addItem("Position");
        m_ui->joystickModeComboBox->addItem("Force");
        m_ui->joystickModeComboBox->addItem("Velocity");
        m_ui->joystickModeComboBox->addItem("Manual");
        m_ui->joystickModeComboBox->setCurrentIndex(joystick->getMode());

        // Create the initial UI.
        createUIForJoystick();
    }
    // But if there're no joysticks, disable everything and hide empty UI.
    else
    {
        m_ui->enableCheckBox->setEnabled(false);
        m_ui->joystickNameComboBox->addItem(tr("No joysticks found. Connect and restart QGC to add one."));
        m_ui->joystickNameComboBox->setEnabled(false);
        m_ui->joystickFrame->hide();
    }
}

void JoystickWidget::styleChanged(bool styleIsDark)
{
    if (styleIsDark)
    {
        buttonLabelColor = QColor(0x14, 0xC6, 0x14);
    }
    else
    {
        buttonLabelColor = QColor(0x73, 0xD9, 0x5D);
    }
}

JoystickWidget::~JoystickWidget()
{
    delete m_ui;
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

void JoystickWidget::updateUI()
{
    // Update the actions for all of the buttons
    for (int i = 0; i < buttons.size(); i++)
    {
        JoystickButton* button = buttons[i];
        int action = joystick->getActionForButton(i);
        button->setAction(action);
    }

    // Update the axis mappings
    int rollAxis = joystick->getMappingRollAxis();
    int pitchAxis = joystick->getMappingPitchAxis();
    int yawAxis = joystick->getMappingYawAxis();
    int throttleAxis = joystick->getMappingThrottleAxis();
    for (int i = 0; i < axes.size(); i++)
    {
        JoystickAxis* axis = axes[i];
        JoystickInput::JOYSTICK_INPUT_MAPPING mapping = JoystickInput::JOYSTICK_INPUT_MAPPING_NONE;
        if (i == rollAxis)
        {
            mapping = JoystickInput::JOYSTICK_INPUT_MAPPING_ROLL;
        }
        else if (i == pitchAxis)
        {
            mapping = JoystickInput::JOYSTICK_INPUT_MAPPING_PITCH;
        }
        else if (i == yawAxis)
        {
            mapping = JoystickInput::JOYSTICK_INPUT_MAPPING_YAW;
        }
        else if (i == throttleAxis)
        {
            mapping = JoystickInput::JOYSTICK_INPUT_MAPPING_THROTTLE;
        }
        axis->setMapping(mapping);
        bool inverted = joystick->getInvertedForAxis(i);
        axis->setInverted(inverted);
        bool limited = joystick->getRangeLimitForAxis(i);
        axis->setRangeLimit(limited);
    }
}

void JoystickWidget::createUIForJoystick()
{
    // Delete all the old UI elements
    foreach (JoystickButton* b, buttons)
    {
        delete b;
    }
    buttons.clear();
    foreach (JoystickAxis* a, axes)
    {
        delete a;
    }
    axes.clear();

    // And add the necessary button displays for this joystick.
    int newButtons = joystick->getJoystickNumButtons();
    if (newButtons)
    {
        m_ui->buttonBox->show();
        for (int i = 0; i < newButtons; i++)
        {
            JoystickButton* button = new JoystickButton(i, m_ui->buttonBox);
            button->setAction(joystick->getActionForButton(i));
            connect(button, SIGNAL(actionChanged(int,int)), this->joystick, SLOT(setButtonAction(int,int)));
            connect(this->joystick, SIGNAL(activeUASSet(UASInterface*)), button, SLOT(setActiveUAS(UASInterface*)));
            m_ui->buttonLayout->addWidget(button);
            buttons.append(button);
        }
    }
    else
    {
        m_ui->buttonBox->hide();
    }

    // Do the same for the axes supported by this joystick.
    int rollAxis = joystick->getMappingRollAxis();
    int pitchAxis = joystick->getMappingPitchAxis();
    int yawAxis = joystick->getMappingYawAxis();
    int throttleAxis = joystick->getMappingThrottleAxis();
    int newAxes = joystick->getJoystickNumAxes();
    if (newAxes)
    {
        for (int i = 0; i < newAxes; i++)
        {
            JoystickAxis* axis = new JoystickAxis(i, m_ui->axesBox);
            axis->setValue(joystick->getCurrentValueForAxis(i));
            if (i == rollAxis)
            {
                axis->setMapping(JoystickInput::JOYSTICK_INPUT_MAPPING_ROLL);
            }
            else if (i == pitchAxis)
            {
                axis->setMapping(JoystickInput::JOYSTICK_INPUT_MAPPING_PITCH);
            }
            else if (i == yawAxis)
            {
                axis->setMapping(JoystickInput::JOYSTICK_INPUT_MAPPING_YAW);
            }
            else if (i == throttleAxis)
            {
                axis->setMapping(JoystickInput::JOYSTICK_INPUT_MAPPING_THROTTLE);
            }
            axis->setInverted(joystick->getInvertedForAxis(i));
            axis->setRangeLimit(joystick->getRangeLimitForAxis(i));
            connect(axis, SIGNAL(mappingChanged(int,JoystickInput::JOYSTICK_INPUT_MAPPING)), this->joystick, SLOT(setAxisMapping(int,JoystickInput::JOYSTICK_INPUT_MAPPING)));
            connect(axis, SIGNAL(inversionChanged(int,bool)), this->joystick, SLOT(setAxisInversion(int,bool)));
            connect(axis, SIGNAL(rangeChanged(int,bool)), this->joystick, SLOT(setAxisRangeLimit(int,bool)));
            connect(this->joystick, SIGNAL(activeUASSet(UASInterface*)), axis, SLOT(setActiveUAS(UASInterface*)));
            m_ui->axesLayout->addWidget(axis);
            axes.append(axis);
        }
    }
    else
    {
        m_ui->axesBox->hide();
    }

    connect(m_ui->calibrationButton, SIGNAL(clicked()), this, SLOT(cycleCalibrationButton()));
}

void JoystickWidget::updateAxisValue(int axis, float value)
{
    if (axis < axes.size())
    {
        axes.at(axis)->setValue(value);
    }
}

void JoystickWidget::setHat(qint8 x, qint8 y)
{
    m_ui->statusLabel->setText(tr("Hat position: x: %1, y: %2").arg(x).arg(y));
}

void JoystickWidget::joystickButtonPressed(int key)
{
    QString colorStyle = QString("QLabel { background-color: %1;}").arg(buttonLabelColor.name());
    buttons.at(key)->setStyleSheet(colorStyle);
}

void JoystickWidget::joystickButtonReleased(int key)
{
    buttons.at(key)->setStyleSheet("");
}

void JoystickWidget::cycleCalibrationButton()
{
    if (this->joystick->calibrating()) {
        this->joystick->setCalibrating(false);
        m_ui->calibrationButton->setText("Calibrate range");
    } else {
        this->joystick->setCalibrating(true);
        m_ui->calibrationButton->setText("End calibration");
    }
}
