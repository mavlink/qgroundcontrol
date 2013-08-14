/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Implementation of class QGCCore
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
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
#include <QFontDatabase>
#include <QMessageBox>
#include <QDir>

#include <QDebug>

#include "configuration.h"
#include "QGC.h"
#include "QGCCore.h"
#include "MainWindow.h"
#include "QGCWelcomeMainWindow.h"
#include "GAudioOutput.h"

#ifdef OPAL_RT
#include "OpalLink.h"
#endif
#include "UDPLink.h"
#include "MAVLinkSimulationLink.h"


/**
 * @brief Constructor for the main application.
 *
 * This constructor initializes and starts the whole application. It takes standard
 * command-line parameters
 *
 * @param argc The number of command-line parameters
 * @param argv The string array of parameters
 **/


QGCCore::QGCCore(bool firstStart, int &argc, char* argv[]) : QApplication(argc, argv),
    restartRequested(false),
    welcome(NULL)
{
    // Exit main application when last window is closed
    connect(this, SIGNAL(lastWindowClosed()), this, SLOT(quit()));

    // Set application name
    this->setApplicationName(QGC_APPLICATION_NAME);
    this->setApplicationVersion(QGC_APPLICATION_VERSION);
    this->setOrganizationName(QLatin1String("QGroundControl"));
    this->setOrganizationDomain("org.qgroundcontrol");

    // Set settings format
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Check application settings
    // clear them if they mismatch
    // QGC then falls back to default
    QSettings settings;

    // Show user an upgrade message if QGC got upgraded (see code below, after splash screen)
    bool upgraded = false;
    enum MainWindow::CUSTOM_MODE mode = MainWindow::CUSTOM_MODE_NONE;
    QString lastApplicationVersion("");
    if (settings.contains("QGC_APPLICATION_VERSION"))
    {
        QString qgcVersion = settings.value("QGC_APPLICATION_VERSION").toString();
        if (qgcVersion != QGC_APPLICATION_VERSION)
        {
            lastApplicationVersion = qgcVersion;
            settings.clear();
            // Write current application version
            settings.setValue("QGC_APPLICATION_VERSION", QGC_APPLICATION_VERSION);
            upgraded = true;
        }
        else
        {
            mode = (enum MainWindow::CUSTOM_MODE) settings.value("QGC_CUSTOM_MODE", (int)MainWindow::CUSTOM_MODE_NONE).toInt();
        }
    }
    else
    {
        // If application version is not set, clear settings anyway
        settings.clear();
        // Write current application version
        settings.setValue("QGC_APPLICATION_VERSION", QGC_APPLICATION_VERSION);
    }

    settings.sync();

    // "Bootload" the application
    if ((!settings.contains("QGC_CUSTOM_MODE_STORED") || settings.value("QGC_CUSTOM_MODE_STORED") == false) && firstStart)
    {
        welcome = new QGCWelcomeMainWindow();
        connect(welcome, SIGNAL(customViewModeSelected(MainWindow::CUSTOM_MODE)), this, SLOT(customViewModeSelected(MainWindow::CUSTOM_MODE)));
        restartRequested = true;
        return;
    }

    // Show splash screen
    QPixmap splashImage(":/files/images/splash.png");
    QSplashScreen* splashScreen = new QSplashScreen(splashImage);
    // Delete splash screen after mainWindow was displayed
    splashScreen->setAttribute(Qt::WA_DeleteOnClose);
    splashScreen->show();
    processEvents();
    splashScreen->showMessage(tr("Loading application fonts"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // Load application font
    QFontDatabase fontDatabase = QFontDatabase();
    const QString fontFileName = ":/general/vera.ttf"; ///< Font file is part of the QRC file and compiled into the app
    //const QString fontFamilyName = "Bitstream Vera Sans";
    if(!QFile::exists(fontFileName)) printf("ERROR! font file: %s DOES NOT EXIST!\n", fontFileName.toStdString().c_str());
    fontDatabase.addApplicationFont(fontFileName);
    // Avoid Using setFont(). In the Qt docu you can read the following:
    //     "Warning: Do not use this function in conjunction with Qt Style Sheets."
    // setFont(fontDatabase.font(fontFamilyName, "Roman", 12));

    // Start the comm link manager
    splashScreen->showMessage(tr("Starting communication links"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    startLinkManager();

    // Start the UAS Manager
    splashScreen->showMessage(tr("Starting UAS manager"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    startUASManager();

    // Start the user interface
    splashScreen->showMessage(tr("Starting user interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // The first call to instance() creates the MainWindow, so make sure it's passed the splashScreen.
    mainWindow = MainWindow::instance_mode(splashScreen, mode);

    UDPLink* udpLink = NULL;

    if (mainWindow->getCustomMode() == MainWindow::CUSTOM_MODE_WIFI)
    {
        // Connect links
        // to make sure that all components are initialized when the
        // first messages arrive
        udpLink = new UDPLink(QHostAddress::Any, 14550);
        MainWindow::instance()->addLink(udpLink);
    }

#ifdef OPAL_RT
    // Add OpalRT Link, but do not connect
    OpalLink* opalLink = new OpalLink();
    MainWindow::instance()->addLink(opalLink);
#endif
    MAVLinkSimulationLink* simulationLink = new MAVLinkSimulationLink(":/demo-log.txt");
    MainWindow::instance()->addLink(simulationLink);
    simulationLink->disconnect();

    // Remove splash screen
    splashScreen->finish(mainWindow);

    if (upgraded) mainWindow->showInfoMessage(tr("Default Settings Loaded"), tr("QGroundControl has been upgraded from version %1 to version %2. Some of your user preferences have been reset to defaults for safety reasons. Please adjust them where needed.").arg(lastApplicationVersion).arg(QGC_APPLICATION_VERSION));

    // Check if link could be connected
    if (udpLink && !udpLink->connect())
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("Could not connect UDP port. Is an instance of " + qAppName() + "already running?");
        msgBox.setInformativeText("It is recommended to close the application and stop all instances. Click Yes to close.");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();

        // Close the message box shortly after the click to prevent accidental clicks
        QTimer::singleShot(15000, &msgBox, SLOT(reject()));

        // Exit application
        if (ret == QMessageBox::Yes)
        {
            //mainWindow->close();
            QTimer::singleShot(200, mainWindow, SLOT(close()));
        }
    }
}

/**
 * @brief Destructor for the groundstation. It destroys all loaded instances.
 *
 **/
QGCCore::~QGCCore()
{

    if (welcome)
    {
        delete welcome;
    } else {
        // unload every modules and delete their handler (loader)
        foreach (QPluginLoader * loader, pluginLoaders) 
        {
            if(loader) 
            {
                loader->unload();
                delete loader;
            }
        }
        pluginLoaders.clear();

        //mainWindow->storeSettings();
        //mainWindow->close();
        //mainWindow->deleteLater();
        // Delete singletons
        // First systems
        delete UASManager::instance();
        // then links
        delete LinkManager::instance();
        // Finally the main window
        //delete MainWindow::instance();
        //The main window now autodeletes on close.
    }
}

/**
 * @brief Start the link managing component.
 *
 * The link manager keeps track of all communication links and provides the global
 * packet queue. It is the main communication hub
 **/
void QGCCore::startLinkManager()
{
    LinkManager::instance();
}

/**
 * @brief Start the Unmanned Air System Manager
 *
 **/
void QGCCore::startUASManager()
{
    // Load UAS plugins
    QDir pluginsDir = QDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_LINUX)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    pluginsDir.cd("plugins");

    UASManager::instance();

    // Load plugins

    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        QPluginLoader *loader = new QPluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader->instance();
        if (plugin) {
            //populateMenus(plugin);
            pluginLoaders += loader;
            qDebug() << "Loaded plugin from " << fileName;
        }
        else {
            qWarning() << "Failed to load plugin from " << fileName;
            qWarning() << loader->errorString();
            delete loader;
        }
    }
}

void QGCCore::customViewModeSelected(enum MainWindow::CUSTOM_MODE mode)
{
    QSettings settings;
    settings.setValue("QGC_CUSTOM_MODE", (unsigned int)mode);
    // Store settings only if requested by user
    settings.setValue("QGC_CUSTOM_MODE_STORED", welcome->getStoreSettings());
    settings.sync();
    welcome->close();

}
