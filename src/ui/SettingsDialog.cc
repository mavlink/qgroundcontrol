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

SettingsDialog::SettingsDialog(QWidget *parent, Qt::WindowFlags flags)
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

    _ui->tabWidget->setCurrentWidget(pMavsettings);

    connect(_ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
}

SettingsDialog::~SettingsDialog()
{
    delete _ui;
}
