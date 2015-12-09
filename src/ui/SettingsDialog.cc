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

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "GAudioOutput.h"
#include "QGCApplication.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"
#include "MainToolBarController.h"
#include "FlightMapSettings.h"

SettingsDialog::SettingsDialog(QWidget *parent, int showTab, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , _ui(new Ui::SettingsDialog)
{
    _ui->setupUi(this);

    // Center the window on the screen.
    QDesktopWidget *desktop = QApplication::desktop();
    int screen = desktop->screenNumber(parent);

    QRect position = frameGeometry();
    position.moveCenter(QApplication::desktop()->availableGeometry(screen).center());
    move(position.topLeft());

    MAVLinkSettingsWidget* pMavsettings  = new MAVLinkSettingsWidget(qgcApp()->toolbox()->mavlinkProtocol(), this);

    // Add the MAVLink settings pane
    _ui->tabWidget->addTab(pMavsettings,  "MAVLink");

    this->window()->setWindowTitle(tr("QGroundControl Settings"));

    _ui->savedFilesLocation->setText(qgcApp()->savedFilesLocation());

    // Connect signals
    connect(_ui->browseSavedFilesLocation, &QPushButton::clicked, this, &SettingsDialog::_selectSavedFilesDirectory);
    connect(_ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::_validateBeforeClose);

    if (showTab == ShowMavlink) {
        _ui->tabWidget->setCurrentWidget(pMavsettings);
    }
}

SettingsDialog::~SettingsDialog()
{
    delete _ui;
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
}
