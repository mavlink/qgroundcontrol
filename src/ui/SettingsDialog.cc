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

#include "JoystickWidget.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "GAudioOutput.h"
#include "QGCApplication.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"

SettingsDialog::SettingsDialog(JoystickInput *joystick, QWidget *parent, Qt::WindowFlags flags) :
QDialog(parent, flags),
_mainWindow(MainWindow::instance()),
_ui(new Ui::SettingsDialog)
{
    _ui->setupUi(this);
    
    // Center the window on the screen.
    QRect position = frameGeometry();
    position.moveCenter(QApplication::desktop()->availableGeometry().center());
    move(position.topLeft());
    
    // Add the joystick settings pane
    _ui->tabWidget->addTab(new JoystickWidget(joystick, this), "Controllers");
    
    MAVLinkSettingsWidget* msettings = new MAVLinkSettingsWidget(LinkManager::instance()->mavlink(), this);
    _ui->tabWidget->addTab(msettings, "MAVLink");
    
    this->window()->setWindowTitle(tr("QGroundControl Settings"));
    
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
    
    // Dock widget title bars
    _ui->titleBarCheckBox->setChecked(_mainWindow->dockWidgetTitleBarsEnabled());
    connect(_ui->titleBarCheckBox,SIGNAL(clicked(bool)),_mainWindow,SLOT(enableDockWidgetTitleBars(bool)));
    
    connect(_ui->deleteSettings, &QAbstractButton::toggled, this, &SettingsDialog::_deleteSettingsToggled);
    
    // Custom mode
    
    _ui->customModeComboBox->addItem(tr("Default: Generic MAVLink and serial links"), MainWindow::CUSTOM_MODE_NONE);
    _ui->customModeComboBox->addItem(tr("Wifi: Generic MAVLink, wifi or serial links"), MainWindow::CUSTOM_MODE_WIFI);
    _ui->customModeComboBox->addItem(tr("PX4: Optimized for PX4 Autopilot Users"), MainWindow::CUSTOM_MODE_PX4);
    
    _ui->customModeComboBox->setCurrentIndex(_ui->customModeComboBox->findData(_mainWindow->getCustomMode()));
    connect(_ui->customModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectCustomMode(int)));
    
    // Application color style
    MainWindow::QGC_MAINWINDOW_STYLE style = _mainWindow->getStyle();
    _ui->styleChooser->setCurrentIndex(style);
    
    _ui->savedFilesLocation->setText(qgcApp()->savedFilesLocation());
    _ui->promptFlightDataSave->setChecked(qgcApp()->promptFlightDataSave());
    
    // Connect signals
    connect(_ui->styleChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));
    connect(_ui->browseSavedFilesLocation, &QPushButton::clicked, this, &SettingsDialog::_selectSavedFilesDirectory);
    connect(_ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::_validateBeforeClose);
}

SettingsDialog::~SettingsDialog()
{
    delete _ui;
}

void SettingsDialog::styleChanged(int index)
{
    _mainWindow->loadStyle((index == 1) ? MainWindow::QGC_MAINWINDOW_STYLE_LIGHT : MainWindow::QGC_MAINWINDOW_STYLE_DARK);
}

void SettingsDialog::selectCustomMode(int mode)
{
    _mainWindow->setCustomMode(static_cast<enum MainWindow::CUSTOM_MODE>(_ui->customModeComboBox->itemData(mode).toInt()));
}

void SettingsDialog::_deleteSettingsToggled(bool checked)
{
    if (checked){
        QGCMessageBox::StandardButton answer = QGCMessageBox::question(tr("Delete Settings"),
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
        QGCMessageBox::warning(tr("Bad save location"),
                               tr("The location to save files to is invalid, or cannot be written to. Please provide a valid directory."));
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
    QString newLocation = QGCFileDialog::getExistingDirectory(this,
                                                            tr("Select the directory where you want to save files to."),
                                                            _ui->savedFilesLocation->text());
    if (!newLocation.isEmpty()) {
        _ui->savedFilesLocation->setText(newLocation);
    }
}
