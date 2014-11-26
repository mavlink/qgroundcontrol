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
#include "QGCApplication.h"
#include "MainWindow.h"
#include "GAudioOutput.h"
#include "CmdLineOptParser.h"
#include "QGCMessageBox.h"
#include "LinkManager.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include "MainWindow.h"

#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif
#include "UDPLink.h"
#include "MAVLinkSimulationLink.h"
#include "SerialLink.h"

QGCApplication* QGCApplication::_app = NULL;

const char* QGCApplication::_deleteAllSettingsKey = "DeleteAllSettingsNextBoot";
const char* QGCApplication::_settingsVersionKey = "SettingsVersion";
const char* QGCApplication::_savedFilesLocationKey = "SavedFilesLocation";
const char* QGCApplication::_promptFlightDataSave = "PromptFLightDataSave";

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
    _singletonAutoPilotPluginManager(NULL),
    _singletonLinkManager(NULL),
    _singletonUASManager(NULL),
    _singletonMainWindow(NULL),
    _singletonUASManagerSaveForMock(NULL)
{
    Q_ASSERT(_app == NULL);
    _app = this;
    
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

void QGCApplication::_initCommon(void)
{
    _createManagerSingletons();
}

bool QGCApplication::_initForNormalAppBoot(void)
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
        
        bool pathCreated = documentsDir.mkpath(_defaultSavedFileDirectoryName);
        Q_UNUSED(pathCreated);
        Q_ASSERT(pathCreated);
        savedFilesLocation = documentsDir.filePath(_defaultSavedFileDirectoryName);
        settings.setValue(_savedFilesLocationKey, savedFilesLocation);
    }
    
    if (!savedFilesLocation.isEmpty()) {
        if (!validatePossibleSavedFilesLocation(savedFilesLocation)) {
            savedFilesLocation.clear();
        }
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
    _singletonMainWindow = new MainWindow(splashScreen, mode);
    Q_CHECK_PTR(_singletonMainWindow);
    _singletonMainWindow->_init();
    
    UDPLink* udpLink = NULL;
    
    if (_singletonMainWindow->getCustomMode() == MainWindow::CUSTOM_MODE_WIFI)
    {
        // Connect links
        // to make sure that all components are initialized when the
        // first messages arrive
        udpLink = new UDPLink(QHostAddress::Any, 14550);
        qgcApp()->singletonLinkManager()->add(udpLink);
    } else {
        // We want to have a default serial link available for "quick" connecting.
        SerialLink *slink = new SerialLink();
        qgcApp()->singletonLinkManager()->add(slink);
    }
    
#ifdef QGC_RTLAB_ENABLED
    // Add OpalRT Link, but do not connect
    OpalLink* opalLink = new OpalLink();
    qgcApp()->singletonMainWindow()->addLink(opalLink);
#endif
    
    // Remove splash screen
    splashScreen->finish(_singletonMainWindow);
    _singletonMainWindow->splashScreenFinished();
    
    // If we made it this far and we still don't have a location. Either the specfied location was invalid
    // or we coudn't create a default location. Either way, we need to let the user know and prompt for a new
    /// settings.
    if (savedFilesLocation.isEmpty()) {
        QGCMessageBox::warning(tr("Bad save location"),
                               tr("The location to save files to is invalid, or cannot be written to. Please provide a new one."));
        qgcApp()->singletonMainWindow()->showSettings();
    }
    
    if (settingsUpgraded) {
        qgcApp()->singletonMainWindow()->showInfoMessage(tr("Settings Cleared"),
                                              tr("The format for QGroundControl saved settings has been modified. "
                                                 "Your saved settings have been reset to defaults."));
    }
    
    // Check if link could be connected
    if (udpLink && !qgcApp()->singletonLinkManager()->connectLink(udpLink))
    {
        QMessageBox::StandardButton button = QGCMessageBox::critical(tr("Could not connect UDP port. Is an instance of %1 already running?").arg(qAppName()),
                                                                     tr("It is recommended to close the application and stop all instances. Click Yes to close."),
                                                                     QMessageBox::Yes | QMessageBox::No,
                                                                     QMessageBox::No);
        // Exit application
        if (button == QMessageBox::Yes)
        {
            //mainWindow->close();
            QTimer::singleShot(200, _singletonMainWindow, SLOT(close()));
        }
    }
    
    return true;
}

bool QGCApplication::_initForUnitTests(void)
{
    return true;
}

/**
 * @brief Destructor for the groundstation. It destroys all loaded instances.
 *
 **/
QGCApplication::~QGCApplication()
{
    delete qgcApp()->singletonUASManager();
    delete qgcApp()->singletonLinkManager();
}

/**
 * @brief Start the link managing component.
 *
 * The link manager keeps track of all communication links and provides the global
 * packet queue. It is the main communication hub
 **/
void QGCApplication::startLinkManager()
{
    qgcApp()->singletonLinkManager();
}

/**
 * @brief Start the Unmanned Air System Manager
 *
 **/
void QGCApplication::startUASManager()
{
    // Load UAS plugins
    QDir pluginsDir = QDir(qgcApp()->applicationDirPath());
    
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
    
    qgcApp()->singletonUASManager();
    
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
    Q_ASSERT(QGCApplication::_app);
    return QGCApplication::_app;
}

AutoPilotPluginManager* QGCApplication::singletonAutoPilotPluginManager(void)
{
    Q_ASSERT(_singletonAutoPilotPluginManager);
    return _singletonAutoPilotPluginManager;
}

LinkManager* QGCApplication::singletonLinkManager(void)
{
    Q_ASSERT(_singletonLinkManager);
    return _singletonLinkManager;
}

UASManagerInterface* QGCApplication::singletonUASManager(void)
{
    Q_ASSERT(_singletonUASManager);
    return _singletonUASManager;
}

MainWindow* QGCApplication::singletonMainWindow(void)
{
    // MainWindow is only created by QGCApplication::_initForNormalAppBoot
    Q_ASSERT(_singletonMainWindow);
    return _singletonMainWindow;
}

#ifdef BUILD_UNITTEST
void QGCApplication::setMockSingletonUASManager(UASManagerInterface* mockUASManager)
{
    Q_ASSERT(mockUASManager != NULL);
    Q_ASSERT(_singletonUASManager != NULL);
    _singletonUASManagerSaveForMock = _singletonUASManager;
    _singletonUASManager = mockUASManager;
}

void QGCApplication::clearMockSingletonUASManager(void)
{
    Q_ASSERT(_singletonUASManagerSaveForMock != NULL);
    Q_ASSERT(_singletonUASManager != NULL);
    _singletonUASManager = _singletonUASManagerSaveForMock;
}

void QGCApplication::destroySingletonsForUnitTest(void)
{
    delete _singletonUASManager;
    delete _singletonLinkManager;
    delete _singletonAutoPilotPluginManager;
    delete _singletonMainWindow;
    
    _singletonUASManager = NULL;
    _singletonLinkManager = NULL;
    _singletonAutoPilotPluginManager = NULL;
    _singletonMainWindow = NULL;
}
#endif

/// @brief Creates all singletons except for MainWindow. This way they are all created on the correct
///         thread.
void QGCApplication::_createManagerSingletons(void)
{
    _singletonAutoPilotPluginManager = new AutoPilotPluginManager(this);
    _singletonUASManager = new UASManager(this);
    _singletonLinkManager = new LinkManager(this);
    
    Q_CHECK_PTR(_singletonAutoPilotPluginManager);
    Q_CHECK_PTR(_singletonUASManager);
    Q_CHECK_PTR(_singletonLinkManager);
}
