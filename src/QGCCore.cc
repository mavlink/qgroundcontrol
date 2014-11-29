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

/**
 * @file
 *   @brief Implementation of class QGCApplication
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

#include <QDebug>

#include "configuration.h"
#include "QGC.h"
#include "QGCCore.h"
#include "MainWindow.h"
#include "GAudioOutput.h"
#include "CmdLineOptParser.h"

#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif
#include "UDPLink.h"
#include "MAVLinkSimulationLink.h"
#include "SerialLink.h"

const char* QGCApplication::_deleteAllSettingsKey = "DeleteAllSettingsNextBoot";
const char* QGCApplication::_settingsVersionKey = "SettingsVersion";
const char* QGCApplication::_savedFilesLocationKey = "SavedFilesLocation";
const char* QGCApplication::_promptFlightDataSave = "PromptFlightDataSave";

const char* QGCApplication::_defaultSavedFileDirectoryName = "QGroundControl";
const char* QGCApplication::_savedFileMavlinkLogDirectoryName = "FlightData";
const char* QGCApplication::_savedFileParameterDirectoryName = "SavedParameters";

/**
 * @brief Constructor for the main application.
 *
 * This constructor initializes and starts the whole application. It takes standard
 * command-line parameters
 *
 * @param argc The number of command-line parameters
 * @param argv The string array of parameters
 **/


QGCApplication::QGCApplication(int &argc, char* argv[]) :
QApplication(argc, argv),
_mainWindow(NULL)
{
    // Set application information
    this->setApplicationName(QGC_APPLICATION_NAME);
    this->setOrganizationName(QGC_ORG_NAME);
    this->setOrganizationDomain(QGC_ORG_DOMAIN);
    
    // Version string is build from component parts. Format is:
    //  vMajor.Minor.BuildNumber BuildType
    QString versionString("v%1.%2.%3 %4");
    versionString = versionString.arg(QGC_APPLICATION_VERSION_MAJOR).arg(QGC_APPLICATION_VERSION_MINOR).arg(QGC_APPLICATION_VERSION_BUILDNUMBER).arg(QGC_APPLICATION_VERSION_BUILDTYPE);
    this->setApplicationVersion(versionString);
    
    // Set settings format
    QSettings::setDefaultFormat(QSettings::IniFormat);
    
    // Parse command line options
    
    bool fClearSettingsOptions = false; // Clear stored settings
    
    CmdLineOpt_t rgCmdLineOptions[] = {
        { "--clear-settings",   &fClearSettingsOptions },
        // Add additional command line option flags here
    };
    
    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, sizeof(rgCmdLineOptions)/sizeof(rgCmdLineOptions[0]), false);
    
    QSettings settings;
    
    // The setting will delete all settings on this boot
    fClearSettingsOptions |= settings.contains(_deleteAllSettingsKey);
    
    if (fClearSettingsOptions) {
        // User requested settings to be cleared on command line
        settings.clear();
        settings.setValue(_settingsVersionKey, QGC_SETTINGS_VERSION);
        settings.sync();
    }
    
}

bool QGCApplication::init(void)
{
    QSettings settings;
    
    // Exit main application when last window is closed
    connect(this, SIGNAL(lastWindowClosed()), this, SLOT(quit()));
    
    // Show user an upgrade message if the settings version has been bumped up
    bool settingsUpgraded = false;
    enum MainWindow::CUSTOM_MODE mode = MainWindow::CUSTOM_MODE_PX4;
    if (settings.contains(_settingsVersionKey)) {
        if (settings.value(_settingsVersionKey).toInt() != QGC_SETTINGS_VERSION) {
            settingsUpgraded = true;
        }
    } else if (settings.allKeys().count()) {
        // Settings version key is missing and there are settings. This is an upgrade scenario.
        settingsUpgraded = true;
    }
    
    if (settingsUpgraded) {
        settings.clear();
        settings.setValue(_settingsVersionKey, QGC_SETTINGS_VERSION);
    }
    
    // Load saved files location and validate
    
    QString savedFilesLocation;
    if (settings.contains(_savedFilesLocationKey)) {
        savedFilesLocation = settings.value(_savedFilesLocationKey).toString();
    } else {
        // No location set. Create a default one in Documents standard location.
        
        QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        
        QDir documentsDir(documentsLocation);
        Q_ASSERT(documentsDir.exists());
        
        if (documentsDir.mkpath(_defaultSavedFileDirectoryName)) {
            savedFilesLocation = documentsDir.filePath(_defaultSavedFileDirectoryName);
        }

        setSavedFilesLocation(savedFilesLocation);
    }
    
    if (!savedFilesLocation.isEmpty()) {
        if (!validatePossibleSavedFilesLocation(savedFilesLocation)) {
            savedFilesLocation.clear();
        }
    }
    
    // If we made it this far and we still don't have a location. Either the specfied location was invalid
    // or we coudn't create a default location. Either way, we need to let the user know and prompt for a new
    /// settings.
    if (savedFilesLocation.isEmpty()) {
        QMessageBox::warning(MainWindow::instance(),
                             tr("Bad save location"),
                             tr("The location to save files to is invalid, or cannot be written to. Please provide a new one."));
        MainWindow::instance()->showSettings();
    }

    
    mode = (enum MainWindow::CUSTOM_MODE) settings.value("QGC_CUSTOM_MODE", (int)MainWindow::CUSTOM_MODE_PX4).toInt();
    
    settings.sync();
    
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
    _mainWindow = MainWindow::_create(splashScreen, mode);
    
    UDPLink* udpLink = NULL;
    
    if (_mainWindow->getCustomMode() == MainWindow::CUSTOM_MODE_WIFI)
    {
        // Connect links
        // to make sure that all components are initialized when the
        // first messages arrive
        udpLink = new UDPLink(QHostAddress::Any, 14550);
        LinkManager::instance()->add(udpLink);
    } else {
        // We want to have a default serial link available for "quick" connecting.
        SerialLink *slink = new SerialLink();
        LinkManager::instance()->add(slink);
    }
    
#ifdef QGC_RTLAB_ENABLED
    // Add OpalRT Link, but do not connect
    OpalLink* opalLink = new OpalLink();
    MainWindow::instance()->addLink(opalLink);
#endif
    
    // Remove splash screen
    splashScreen->finish(_mainWindow);
    _mainWindow->splashScreenFinished();
    
    if (settingsUpgraded) {
        _mainWindow->showInfoMessage(tr("Settings Cleared"),
                                     tr("The format for QGroundControl saved settings has been modified. "
                                        "Your saved settings have been reset to defaults."));
    }
    
    // Check if link could be connected
    if (udpLink && !LinkManager::instance()->connectLink(udpLink))
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
            QTimer::singleShot(200, _mainWindow, SLOT(close()));
        }
    }
    
    return true;
}

/**
 * @brief Destructor for the groundstation. It destroys all loaded instances.
 *
 **/
QGCApplication::~QGCApplication()
{
    delete UASManager::instance();
    delete LinkManager::instance();
}

/**
 * @brief Start the link managing component.
 *
 * The link manager keeps track of all communication links and provides the global
 * packet queue. It is the main communication hub
 **/
void QGCApplication::startLinkManager()
{
    LinkManager::instance();
}

/**
 * @brief Start the Unmanned Air System Manager
 *
 **/
void QGCApplication::startUASManager()
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
    
    QStringList pluginFileNames;
    
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        if (plugin) {
            //populateMenus(plugin);
            pluginFileNames += fileName;
            //printf(QString("Loaded plugin from " + fileName + "\n").toStdString().c_str());
        }
    }
}

void QGCApplication::deleteAllSettingsNextBoot(void)
{
    QSettings settings;
    settings.setValue(_deleteAllSettingsKey, true);
}

void QGCApplication::clearDeleteAllSettingsNextBoot(void)
{
    QSettings settings;
    settings.remove(_deleteAllSettingsKey);
}

void QGCApplication::setSavedFilesLocation(QString& location)
{
    QSettings settings;
    settings.setValue(_savedFilesLocationKey, location);
}

bool QGCApplication::validatePossibleSavedFilesLocation(QString& location)
{
    // Make sure we can write to the directory
    QString filename = QDir(location).filePath("QGCTempXXXXXXXX.tmp");
    QTemporaryFile tempFile(filename);
    if (!tempFile.open()) {
        return false;
    }
    
    return true;
}

QString QGCApplication::savedFilesLocation(void)
{
    QSettings settings;
    
    Q_ASSERT(settings.contains(_savedFilesLocationKey));
    return settings.value(_savedFilesLocationKey).toString();
}

QString QGCApplication::savedParameterFilesLocation(void)
{
    QString location;
    QDir    parentDir(savedFilesLocation());
    
    location = parentDir.filePath(_savedFileParameterDirectoryName);
    
    if (!QDir(location).exists()) {
        // If directory doesn't exist, try to create it
        if (!parentDir.mkpath(_savedFileParameterDirectoryName)) {
            // Return an error
            location.clear();
        }
    }
    
    return location;
}

QString QGCApplication::mavlinkLogFilesLocation(void)
{
    QString location;
    QDir    parentDir(savedFilesLocation());
    
    location = parentDir.filePath(_savedFileMavlinkLogDirectoryName);
    
    if (!QDir(location).exists()) {
        // If directory doesn't exist, try to create it
        if (!parentDir.mkpath(_savedFileMavlinkLogDirectoryName)) {
            // Return an error
            location.clear();
        }
    }
    
    return location;
}

bool QGCApplication::promptFlightDataSave(void)
{
    QSettings settings;
    
    return settings.value(_promptFlightDataSave, true).toBool();
}

void QGCApplication::setPromptFlightDataSave(bool promptForSave)
{
    QSettings settings;
    settings.setValue(_promptFlightDataSave, promptForSave);
}

/// @brief Returns the QGCApplication object singleton.
QGCApplication* qgcApp(void)
{
    QGCApplication* app = dynamic_cast<QGCApplication*>(qApp);
    Q_ASSERT(app);
    return app;
}
