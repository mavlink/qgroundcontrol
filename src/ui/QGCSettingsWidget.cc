#include <QSettings>
#include <QDesktopWidget>

#include "QGCSettingsWidget.h"
#include "MainWindow.h"
#include "ui_QGCSettingsWidget.h"

#include "JoystickAxis.h"
#include "JoystickButton.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "GAudioOutput.h"

//, Qt::WindowFlags flags

QGCSettingsWidget::QGCSettingsWidget(JoystickInput *joystick, QWidget *parent, Qt::WindowFlags flags) :
    QDialog(parent, flags),
    joystick(joystick),
    mainWindow((MainWindow*)parent),
    ui(new Ui::QGCSettingsWidget)
{
    ui->setupUi(this);

    // Center the window on the screen.
    QRect position = frameGeometry();
    position.moveCenter(QApplication::desktop()->availableGeometry().center());
    move(position.topLeft());

    // Add all protocols
    QList<ProtocolInterface*> protocols = LinkManager::instance()->getProtocols();
    foreach (ProtocolInterface* protocol, protocols) {
        MAVLinkProtocol* mavlink = dynamic_cast<MAVLinkProtocol*>(protocol);
        if (mavlink) {
            MAVLinkSettingsWidget* msettings = new MAVLinkSettingsWidget(mavlink, this);
            ui->tabWidget->addTab(msettings, "MAVLink");
        }
    }

    this->window()->setWindowTitle(tr("QGroundControl Settings"));

    // Initialize the joystick UI
    initJoystickUI();

    // Watch for button, axis, and hat input events from the joystick.
    connect(this->joystick, SIGNAL(buttonPressed(int)), this, SLOT(joystickButtonPressed(int)));
    connect(this->joystick, SIGNAL(buttonReleased(int)), this, SLOT(joystickButtonReleased(int)));
    connect(this->joystick, SIGNAL(axisValueChanged(int,float)), this, SLOT(updateAxisValue(int,float)));
    connect(this->joystick, SIGNAL(hatDirectionChanged(qint8,qint8)), this, SLOT(setHat(qint8,qint8)));

    // Also watch for when new settings were loaded for the current joystick to do a mass UI refresh.
    connect(this->joystick, SIGNAL(joystickSettingsChanged()), this, SLOT(updateJoystickUI()));

    // If the selected joystick is changed, update the JoystickInput.
    connect(ui->joystickNameComboBox, SIGNAL(currentIndexChanged(int)), this->joystick, SLOT(setActiveJoystick(int)));
    // Also wait for the JoystickInput to switch, then update our UI.
    connect(this->joystick, SIGNAL(newJoystickSelected()), this, SLOT(createUIForJoystick()));

    // Initialize the UI to the current JoystickInput state. Also make sure to listen for future changes
    // so that the UI can be updated.
    connect(ui->enableCheckBox, SIGNAL(toggled(bool)), ui->joystickFrame, SLOT(setEnabled(bool)));
    ui->enableCheckBox->setChecked(this->joystick->enabled()); // Needs to be after connecting to the joystick frame and before watching for enabled events from JoystickInput.
    connect(ui->enableCheckBox, SIGNAL(toggled(bool)), this->joystick, SLOT(setEnabled(bool)));

    // Settings reset
    connect(ui->resetSettingsButton, SIGNAL(clicked()), this, SLOT(resetSettings()));

    // Audio preferences
    ui->audioMuteCheckBox->setChecked(GAudioOutput::instance()->isMuted());
    connect(ui->audioMuteCheckBox, SIGNAL(toggled(bool)), GAudioOutput::instance(), SLOT(mute(bool)));
    connect(GAudioOutput::instance(), SIGNAL(mutedChanged(bool)), ui->audioMuteCheckBox, SLOT(setChecked(bool)));

    // Reconnect
    ui->reconnectCheckBox->setChecked(mainWindow->autoReconnectEnabled());
    connect(ui->reconnectCheckBox, SIGNAL(clicked(bool)), mainWindow, SLOT(enableAutoReconnect(bool)));

    // Low power mode
    ui->lowPowerCheckBox->setChecked(mainWindow->lowPowerModeEnabled());
    connect(ui->lowPowerCheckBox, SIGNAL(clicked(bool)), mainWindow, SLOT(enableLowPowerMode(bool)));

    // Dock widget title bars
    ui->titleBarCheckBox->setChecked(mainWindow->dockWidgetTitleBarsEnabled());
    connect(ui->titleBarCheckBox,SIGNAL(clicked(bool)),mainWindow,SLOT(enableDockWidgetTitleBars(bool)));

    // Custom mode

    ui->customModeComboBox->addItem(tr("Default: Generic MAVLink and serial links"), MainWindow::CUSTOM_MODE_NONE);
    ui->customModeComboBox->addItem(tr("Wifi: Generic MAVLink, wifi or serial links"), MainWindow::CUSTOM_MODE_WIFI);
    ui->customModeComboBox->addItem(tr("PX4: Optimized for PX4 Autopilot Users"), MainWindow::CUSTOM_MODE_PX4);
    // XXX we need to polish the APM view mode before re-enabling this
    //ui->customModeComboBox->addItem(tr("APM: Optimized for ArduPilot Users"), MainWindow::CUSTOM_MODE_APM);

    ui->customModeComboBox->setCurrentIndex(ui->customModeComboBox->findData(mainWindow->getCustomMode()));
    connect(ui->customModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectCustomMode(int)));

    // Intialize the style UI to the proper values obtained from the MainWindow.
    MainWindow::QGC_MAINWINDOW_STYLE style = mainWindow->getStyle();
    ui->styleChooser->setCurrentIndex(style);
    if (style == MainWindow::QGC_MAINWINDOW_STYLE_DARK)
    {
        ui->styleSheetFile->setText(mainWindow->getDarkStyleSheet());
    }
    else
    {
        ui->styleSheetFile->setText(mainWindow->getLightStyleSheet());
    }

    // And then connect all the signals for the UI for changing styles.
    connect(ui->styleChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));
    connect(ui->styleCustomButton, SIGNAL(clicked()), this, SLOT(selectStylesheet()));
    connect(ui->styleDefaultButton, SIGNAL(clicked()), this, SLOT(setDefaultStyle()));
    connect(ui->styleSheetFile, SIGNAL(editingFinished()), this, SLOT(lineEditFinished()));

    // Close / destroy
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(deleteLater()));
}

QGCSettingsWidget::~QGCSettingsWidget()
{
    delete ui;
}

void QGCSettingsWidget::initJoystickUI()
{
    // Add the joysticks to the top combobox. They're indexed by their item number.
    // And set the currently-selected combobox item to the current joystick.
    int joysticks = joystick->getNumJoysticks();
    if (joysticks)
    {
        for (int i = 0; i < joysticks; i++)
        {
            ui->joystickNameComboBox->addItem(joystick->getJoystickNameById(i));
        }
        ui->joystickNameComboBox->setCurrentIndex(joystick->getJoystickID());
        // And if joystick support is enabled, show the UI.
        if (ui->enableCheckBox->isChecked())
        {
            ui->joystickFrame->setEnabled(true);
        }

        // Create the initial UI.
        createUIForJoystick();
    }
    // But if there're no joysticks, disable everything and hide empty UI.
    else
    {
        ui->enableCheckBox->setEnabled(false);
        ui->joystickNameComboBox->addItem(tr("No joysticks found. Connect and restart QGC to add one."));
        ui->joystickNameComboBox->setEnabled(false);
        ui->joystickFrame->hide();
    }
}

void QGCSettingsWidget::updateJoystickUI()
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

void QGCSettingsWidget::createUIForJoystick()
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
        ui->buttonBox->show();
        for (int i = 0; i < newButtons; i++)
        {
            JoystickButton* button = new JoystickButton(i, ui->buttonBox);
            button->setAction(joystick->getActionForButton(i));
            connect(button, SIGNAL(actionChanged(int,int)), this->joystick, SLOT(setButtonAction(int,int)));
            connect(this->joystick, SIGNAL(activeUASSet(UASInterface*)), button, SLOT(setActiveUAS(UASInterface*)));
            ui->buttonLayout->addWidget(button);
            buttons.append(button);
        }
    }
    else
    {
        ui->buttonBox->hide();
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
            JoystickAxis* axis = new JoystickAxis(i, ui->axesBox);
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
            ui->axesLayout->addWidget(axis);
            axes.append(axis);
        }
    }
    else
    {
        ui->axesBox->hide();
    }
}

void QGCSettingsWidget::updateAxisValue(int axis, float value)
{
    if (axis < axes.size())
    {
        axes.at(axis)->setValue(value);
    }
}

void QGCSettingsWidget::setHat(qint8 x, qint8 y)
{
    ui->statusLabel->setText(tr("Hat position: x: %1, y: %2").arg(x).arg(y));
}

void QGCSettingsWidget::joystickButtonPressed(int key)
{
    QString colorStyle = QString("QLabel { background-color: %1;}").arg(buttonLabelColor.name());
    buttons.at(key)->setStyleSheet(colorStyle);
}

void QGCSettingsWidget::joystickButtonReleased(int key)
{
    buttons.at(key)->setStyleSheet("");
}

void QGCSettingsWidget::selectStylesheet()
{
    // Let user select style sheet. The root directory for the file picker is the user's home directory if they haven't loaded a custom style.
    // Otherwise it defaults to the directory of that custom file.
    QString findDir;
    QString oldStylesheet(ui->styleSheetFile->text());
    QFile styleSheet(oldStylesheet);
    if (styleSheet.exists() && oldStylesheet[0] != ':')
    {
        findDir = styleSheet.fileName();
    }
    else
    {
        findDir = QDir::homePath();
    }

    // Prompt the user to select a new style sheet. Do nothing if they cancel.
    QString newStyleFileName = QFileDialog::getOpenFileName(this, tr("Specify stylesheet"), findDir, tr("CSS Stylesheet (*.css);;"));
    if (newStyleFileName.isNull()) {
        return;
    }

    // Load the new style sheet if a valid one was selected, notifying the user
    // of an error if necessary.
    QFile newStyleFile(newStyleFileName);
    if (!newStyleFile.exists() || !updateStyle(newStyleFileName))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(tr("QGroundControl did not load a new style"));
        msgBox.setInformativeText(tr("Stylesheet file %1 was not readable").arg(newStyleFileName));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
    // And update the UI as needed.
    else
    {
        ui->styleSheetFile->setText(newStyleFileName);
    }
}

bool QGCSettingsWidget::updateStyle(QString style)
{
    switch (ui->styleChooser->currentIndex())
    {
    case 0:
        return mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, style);
    case 1:
        return mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, style);
    default:
        return false;
    }
}

void QGCSettingsWidget::lineEditFinished()
{
    QString newStyleFileName(ui->styleSheetFile->text());
    QFile newStyleFile(newStyleFileName);
    if (!newStyleFile.exists() || !updateStyle(newStyleFileName))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(tr("QGroundControl did not load a new style"));
        msgBox.setInformativeText(tr("Stylesheet file %1 was not readable").arg(newStyleFileName));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void QGCSettingsWidget::styleChanged(int index)
{
    if (index == 1)
    {
        ui->styleSheetFile->setText(mainWindow->getLightStyleSheet());
        mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, mainWindow->getLightStyleSheet());
    }
    else
    {
        ui->styleSheetFile->setText(mainWindow->getDarkStyleSheet());
        mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, mainWindow->getDarkStyleSheet());
    }
}

void QGCSettingsWidget::setDefaultStyle()
{
    if (ui->styleChooser->currentIndex() == 1)
    {
        ui->styleSheetFile->setText(MainWindow::defaultLightStyle);
        mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, MainWindow::defaultLightStyle);
    }
    else
    {
        ui->styleSheetFile->setText(MainWindow::defaultDarkStyle);
        mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, MainWindow::defaultDarkStyle);
    }
}

void QGCSettingsWidget::selectCustomMode(int mode)
{
    MainWindow::instance()->setCustomMode(static_cast<enum MainWindow::CUSTOM_MODE>(ui->customModeComboBox->itemData(mode).toInt()));
    MainWindow::instance()->showInfoMessage(tr("Please restart QGroundControl"), tr("The optimization selection was changed. The application needs to be closed and restarted to put all optimizations into effect."));
}

void QGCSettingsWidget::resetSettings()
{
    QSettings settings;
    settings.sync();
    settings.clear();
    // Write current application version
    settings.setValue("QGC_APPLICATION_VERSION", QGC_APPLICATION_VERSION);
    settings.sync();
}
