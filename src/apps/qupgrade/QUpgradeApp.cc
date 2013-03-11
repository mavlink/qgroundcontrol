/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Implementation of main application class
 *
 *   @author Lorenz Meier <lm@inf.ethz.ch>
 *
 */


#include <QFile>
#include <QFlags>
#include <QThread>
#include <QSplashScreen>
#include <QPixmap>
#include <QDesktopWidget>
#include <QPainter>
#include <QStyleFactory>
#include <QAction>
#include <QSettings>
#include <QFontDatabase>
#include <QMainWindow>

#include "QUpgradeApp.h"
#include "QUpgradeMainWindow.h"
#include "PX4FirmwareUpgradeWorker.h"
#include "UDPLink.h"


/**
 * @brief Constructor for the main application.
 *
 * This constructor initializes and starts the whole application. It takes standard
 * command-line parameters
 *
 * @param argc The number of command-line parameters
 * @param argv The string array of parameters
 **/

QUpgradeApp::QUpgradeApp(int &argc, char* argv[]) : QApplication(argc, argv)
{
    this->setApplicationName("Q PX4 Firmware Upgrade");
    this->setApplicationVersion("v. 1.0.0 (Beta)");
    this->setOrganizationName(QLatin1String("PX4"));
    this->setOrganizationDomain("http://pixhawk.ethz.ch/px4/");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    // Exit main application when last window is closed
    connect(this, SIGNAL(lastWindowClosed()), this, SLOT(quit()));

    // Create main window
    QUpgradeMainWindow* window = new QUpgradeMainWindow();
    PX4FirmwareUpgrader *upgrader = new PX4FirmwareUpgrader(window);
    window->setCentralWidget(upgrader);

    // Get PX4 upgrade widget and instantiate worker thread
    PX4FirmwareUpgradeWorker* worker = PX4FirmwareUpgradeWorker::putWorkerInThread(this);

//    connect(worker, SIGNAL(detectionStatusChanged(QString)), upgrader, SLOT(setDetectionStatusText(QString)), Qt::QueuedConnection);
//    connect(worker, SIGNAL(upgradeStatusChanged(QString)), upgrader, SLOT(setFlashStatusText(QString)), Qt::QueuedConnection);
//    connect(worker, SIGNAL(upgradeProgressChanged(int)), upgrader, SLOT(setFlashProgress(int)), Qt::QueuedConnection);
//    connect(worker, SIGNAL(validPortFound(QString)), upgrader, SLOT(setPortName(QString)));
//    connect(upgrader, SIGNAL(firmwareFileNameSet(QString)), worker, SLOT(loadFirmware(QString)), Qt::QueuedConnection);
//    connect(upgrader, SIGNAL(upgrade()), worker, SLOT(upgrade()), Qt::QueuedConnection);
//    connect(this, SIGNAL(lastWindowClosed()), worker, SLOT(abort()), Qt::QueuedConnection);

    worker->loadFirmware("/Users/lomeier/src/Firmware/Images/px4fmu.bin");

    window->setWindowTitle(applicationName() + " " + applicationVersion());
    window->show();
}

/**
 * @brief Destructor for the groundstation. It destroys all loaded instances.
 *
 **/
QUpgradeApp::~QUpgradeApp()
{
}

