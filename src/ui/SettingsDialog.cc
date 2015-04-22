/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

#include <QSettings>
#include <QDesktopWidget>

#include "SettingsDialog.h"
#include "MainWindow.h"
#include "ui_SettingsDialog.h"

#ifndef __mobile__
#include "JoystickWidget.h"
#endif
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "QGCLinkConfiguration.h"
#include "GAudioOutput.h"
#include "QGCApplication.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"
#include "MainToolBar.h"

#ifndef __mobile__
SettingsDialog::SettingsDialog(JoystickInput *joystick, QWidget *parent, int showTab, Qt::WindowFlags flags) :
#else
SettingsDialog::SettingsDialog(QWidget *parent, int showTab, Qt::WindowFlags flags) :
#endif
QDialog(parent, flags),
_mainWindow(MainWindow::instance()),
_ui(new Ui::SettingsDialog)
{
    _ui->setupUi(this);

    // Center the window on the screen.
    QRect position = frameGeometry();
    position.moveCenter(QApplication::desktop()->availableGeometry().center());
    move(position.topLeft());

    QGCLinkConfiguration*  pLinkConf     = new QGCLinkConfiguration(this);
#ifndef __mobile__
    JoystickWidget*        pJoystickConf = new JoystickWidget(joystick, this);
#endif
    MAVLinkSettingsWidget* pMavsettings  = new MAVLinkSettingsWidget(MAVLinkProtocol::instance(), this);

    // Add the link settings pane
    _ui->tabWidget->addTab(pLinkConf,     "Comm Links");
#ifndef __mobile__
    // Add the joystick settings pane
    _ui->tabWidget->addTab(pJoystickConf, "Controllers");
#endif
    // Add the MAVLink settings pane
    _ui->tabWidget->addTab(pMavsettings,  "MAVLink");

    this->window()->setWindowTitle(tr("QGroundControl Settings"));

    // Tool Bar Preferences
    QSettings settings;
    settings.beginGroup(TOOL_BAR_SETTINGS_GROUP);
    _ui->showBattery->setChecked(settings.value( TOOL_BAR_SHOW_BATTERY,  true).toBool());
    _ui->showGPS->setChecked(settings.value(     TOOL_BAR_SHOW_GPS,      true).toBool());
    _ui->showMav->setChecked(settings.value(     TOOL_BAR_SHOW_MAV,      true).toBool());
    _ui->showMessages->setChecked(settings.value(TOOL_BAR_SHOW_MESSAGES, true).toBool());
    _ui->showRSSI->setChecked(settings.value(   TOOL_BAR_SHOW_RSSI,      true).toBool());
    settings.endGroup();
    // Audio preferences
    _ui->audioMuteCheckBox->setChecked(GAudioOutput::instance()->isMuted());
    connect(_ui->audioMuteCheckBox, SIGNAL(toggled(bool)), GAudioOutput::instance(), SLOT(mute(bool)));
    connect(GAudioOutput::instance(), SIGNAL(mutedChanged(bool)), _ui->audioMuteCheckBox, SLOT(setChecked(bool)));

    // Reconnect
    _ui->reconnectCheckBox->setChecked(_mainWindow->autoReconnectEnabled());
    connect(_ui->reconnectCheckBox, SIGNAL(clicked(bool)), _mainWindow, SLOT(enableAutoReconnect(bool)));

    // Low power mode
    _ui->lowPowerCheckBox->setChecked(_mainWindow->lowPowerModeEnabled());
    connect(_ui->lowPowerCheckBox, SIGNAL(clicked(bool)), _mainWindow, SLOT(enableLowPowerMode(bool)));

    connect(_ui->deleteSettings, &QAbstractButton::toggled, this, &SettingsDialog::_deleteSettingsToggled);

    // Application color style
    _ui->styleChooser->setCurrentIndex(qgcApp()->styleIsDark() ? 0 : 1);

    _ui->savedFilesLocation->setText(qgcApp()->savedFilesLocation());
    _ui->promptFlightDataSave->setChecked(qgcApp()->promptFlightDataSave());

    // Connect signals
    connect(_ui->styleChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));
    connect(_ui->browseSavedFilesLocation, &QPushButton::clicked, this, &SettingsDialog::_selectSavedFilesDirectory);
    connect(_ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::_validateBeforeClose);

    switch (showTab) {
        case ShowCommLinks:
            _ui->tabWidget->setCurrentWidget(pLinkConf);
            break;
#ifndef __mobile__
        case ShowControllers:
            _ui->tabWidget->setCurrentWidget(pJoystickConf);
            break;
#endif
        case ShowMavlink:
            _ui->tabWidget->setCurrentWidget(pMavsettings);
            break;
    }
}

SettingsDialog::~SettingsDialog()
{
    delete _ui;
}

void SettingsDialog::styleChanged(int index)
{
    qgcApp()->setStyle(index == 0);
}

void SettingsDialog::_deleteSettingsToggled(bool checked)
{
    if (checked){
        QGCMessageBox::StandardButton answer =
            QGCMessageBox::question(tr("Delete Settings"),
                tr("All saved settings will be deleted the next time you start QGroundControl. Is this really what you want?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            qgcApp()->deleteAllSettingsNextBoot();
        } else {
            _ui->deleteSettings->setChecked(false);
        }
    } else {
        qgcApp()->clearDeleteAllSettingsNextBoot();
    }
}

/// @brief Validates the settings before closing
void SettingsDialog::_validateBeforeClose(void)
{
    QGCApplication* app = qgcApp();
    // Validate the saved file location
    QString saveLocation = _ui->savedFilesLocation->text();
    if (!app->validatePossibleSavedFilesLocation(saveLocation)) {
        QGCMessageBox::warning(
            tr("Invalid Save Location"),
            tr("The location to save files is invalid, or cannot be written to. Please provide a valid directory."));
        return;
    }
    // Locations is valid, save
    app->setSavedFilesLocation(saveLocation);
    qgcApp()->setPromptFlightDataSave(_ui->promptFlightDataSave->checkState() == Qt::Checked);
    // Close dialog
    accept();
}

/// @brief Displays a directory picker dialog to allow the user to select a saved file location
void SettingsDialog::_selectSavedFilesDirectory(void)
{
    QString newLocation = QGCFileDialog::getExistingDirectory(
        this,
        tr("Select the directory where you want to save files to."),
        _ui->savedFilesLocation->text());
    if (!newLocation.isEmpty()) {
        _ui->savedFilesLocation->setText(newLocation);
    }

    // TODO:
    // Once a directory is selected, we need to display the various subdirectories used underneath:
    // * Flight data logs
    // * Parameters
}

void SettingsDialog::on_showGPS_clicked(bool checked)
{
    _mainWindow->getMainToolBar()->viewStateChanged(TOOL_BAR_SHOW_GPS, checked);
}

void SettingsDialog::on_showBattery_clicked(bool checked)
{
    _mainWindow->getMainToolBar()->viewStateChanged(TOOL_BAR_SHOW_BATTERY, checked);
}

void SettingsDialog::on_showMessages_clicked(bool checked)
{
    _mainWindow->getMainToolBar()->viewStateChanged(TOOL_BAR_SHOW_MESSAGES, checked);
}

void SettingsDialog::on_showMav_clicked(bool checked)
{
    _mainWindow->getMainToolBar()->viewStateChanged(TOOL_BAR_SHOW_MAV, checked);
}

void SettingsDialog::on_showRSSI_clicked(bool checked)
{
    _mainWindow->getMainToolBar()->viewStateChanged(TOOL_BAR_SHOW_RSSI, checked);
}
