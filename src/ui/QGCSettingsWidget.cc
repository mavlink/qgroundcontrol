#include <QSettings>
#include <QDesktopWidget>

#include "QGCSettingsWidget.h"
#include "MainWindow.h"
#include "ui_QGCSettingsWidget.h"

#include "JoystickWidget.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "GAudioOutput.h"
#include "QGCCore.h"

QGCSettingsWidget::QGCSettingsWidget(JoystickInput *joystick, QWidget *parent, Qt::WindowFlags flags) :
    QDialog(parent, flags),
    mainWindow((MainWindow*)parent),
    ui(new Ui::QGCSettingsWidget)
{
    ui->setupUi(this);

    // Center the window on the screen.
    QRect position = frameGeometry();
    position.moveCenter(QApplication::desktop()->availableGeometry().center());
    move(position.topLeft());

    // Add the joystick settings pane
    ui->tabWidget->addTab(new JoystickWidget(joystick, this), "Controllers");

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
    
    connect(ui->deleteSettings, &QAbstractButton::toggled, this, &QGCSettingsWidget::_deleteSettingsToggled);

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

    // And then connect all the signals for the UI for changing styles.
    connect(ui->styleChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));

    // Close / destroy
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(deleteLater()));
}

QGCSettingsWidget::~QGCSettingsWidget()
{
    delete ui;
}

void QGCSettingsWidget::styleChanged(int index)
{
    mainWindow->loadStyle((index == 1) ? MainWindow::QGC_MAINWINDOW_STYLE_LIGHT : MainWindow::QGC_MAINWINDOW_STYLE_DARK);
}

void QGCSettingsWidget::selectCustomMode(int mode)
{
    MainWindow::instance()->setCustomMode(static_cast<enum MainWindow::CUSTOM_MODE>(ui->customModeComboBox->itemData(mode).toInt()));
    MainWindow::instance()->showInfoMessage(tr("Please restart QGroundControl"), tr("The optimization selection was changed. The application needs to be closed and restarted to put all optimizations into effect."));
}

void QGCSettingsWidget::_deleteSettingsToggled(bool checked)
{
    if (checked){
        QMessageBox::StandardButton answer = QMessageBox::question(this,
                                                                   tr("Delete Settings"),
                                                                   tr("All saved settgings will be deleted the next time you start QGroundControl. Is this really what you want?"),
                                                                   QMessageBox::Yes | QMessageBox::No,
                                                                   QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            QSettings settings;
            settings.setValue(QGCCore::deleteAllSettingsKey, 1);
        } else {
            ui->deleteSettings->setChecked(false);
        }
    }
}
