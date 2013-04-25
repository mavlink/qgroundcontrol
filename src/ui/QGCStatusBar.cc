/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2013 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include <QToolButton>
#include <QLabel>
#include <QSpacerItem>
#include "QGCStatusBar.h"
#include "UASManager.h"
#include "MainWindow.h"

QGCStatusBar::QGCStatusBar(QWidget *parent) :
    QStatusBar(parent),
    toggleLoggingButton(NULL),
    player(NULL),
    changed(true),
    lastLogDirectory(QDesktopServices::storageLocation(QDesktopServices::DesktopLocation))
{
    setObjectName("QGC_STATUSBAR");

    toggleLoggingButton = new QPushButton("Logging", this);
    toggleLoggingButton->setCheckable(true);

    addWidget(toggleLoggingButton);

    loadSettings();
}

void QGCStatusBar::setLogPlayer(QGCMAVLinkLogPlayer* player)
{
    this->player = player;
    addPermanentWidget(player);
    connect(toggleLoggingButton, SIGNAL(clicked(bool)), this, SLOT(logging(bool)));

    // XXX Mutex issue if called like this
//    toggleLoggingButton->blockSignals(true);
//    toggleLoggingButton->setChecked(MainWindow::instance()->getMAVLink()->loggingEnabled());
//    toggleLoggingButton->blockSignals(false);
}

void QGCStatusBar::logging(bool checked)
{
    // Stop logging in any case
    MainWindow::instance()->getMAVLink()->enableLogging(false);

    if (!checked && player)
    {
        player->setLastLogFile(lastLogDirectory);
    }

	// If the user is enabling logging
    if (checked)
    {
		// Prompt the user for a filename/location to save to
        QString fileName = QFileDialog::getSaveFileName(this, tr("Specify MAVLink log file to save to"), lastLogDirectory, tr("MAVLink Logfile (*.mavlink *.log *.bin);;"));

		// Check that they didn't cancel out
		if (fileName.isNull())
		{
            toggleLoggingButton->setChecked(false);
			return;
		}

		// Make sure the file's named properly
        if (!fileName.endsWith(".mavlink"))
        {
            fileName.append(".mavlink");
        }

		// Check that we can save the logfile
        QFileInfo file(fileName);
        if ((file.exists() && !file.isWritable()))
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("The selected logfile is not writable"));
            msgBox.setInformativeText(tr("Please make sure that the file %1 is writable or select a different file").arg(fileName));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        }
		// Otherwise we're off and logging
        else
        {
            MainWindow::instance()->getMAVLink()->setLogfileName(fileName);
            MainWindow::instance()->getMAVLink()->enableLogging(true);
            lastLogDirectory = file.absoluteDir().absolutePath(); //save last log directory
        }
    }
}

void QGCStatusBar::loadSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAVLINKLOGPLAYER");
    lastLogDirectory = settings.value("LAST_LOG_DIRECTORY", lastLogDirectory).toString();
    settings.endGroup();
}

void QGCStatusBar::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAVLINKLOGPLAYER");
    settings.setValue("LAST_LOG_DIRECTORY", lastLogDirectory);
    settings.endGroup();
    settings.sync();
}

QGCStatusBar::~QGCStatusBar()
{
    storeSettings();
    if (toggleLoggingButton) toggleLoggingButton->deleteLater();
}
