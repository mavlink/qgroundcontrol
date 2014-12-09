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
#include "MainWindow.h"
#include "UDPLink.h"
#include "MAVLinkSimulationLink.h"
#include "SerialLink.h"
#include "QGCSingleton.h"
#include "LinkManager.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include "QGCTemporaryFile.h"

#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif


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


QGCApplication::QGCApplication(int &argc, char* argv[], bool unitTesting) :
    QApplication(argc, argv),
    _runningUnitTests(unitTesting)
{
    Q_ASSERT(_app == NULL);
    _app = this;
    
    
#ifdef QT_DEBUG
    // First thing we want to do is set up the qtlogging.ini file. If it doesn't already exist we copy
    // it to the correct location. This way default debug builds will have logging turned off.
    
    bool loggingDirectoryOk = false;
    QDir iniFileLocation(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    if (!iniFileLocation.cd("QtProjects")) {
        if (!iniFileLocation.mkdir("QtProjects")) {
            qDebug() << "Unable to create qtlogging.ini directory" << iniFileLocation.filePath("QtProjects");
        } else {
            if (!iniFileLocation.cd("QtProjects")) {
                qDebug() << "Unable to access qtlogging.ini directory" << iniFileLocation.filePath("QtProjects");;
            }
            loggingDirectoryOk = true;
        }
    } else {
        loggingDirectoryOk = true;
    }
    
    if (loggingDirectoryOk) {
        qDebug () << iniFileLocation;
        if (!iniFileLocation.exists("qtlogging.ini")) {
            if (!QFile::copy(":QLoggingCategory/qtlogging.ini", iniFileLocation.filePath("qtlogging.ini"))) {
                qDebug() << "Unable to copy qtlogging.ini to" << iniFileLocation;
            }
        }
    }
#endif
    
    // Set application information
    if (_runningUnitTests) {
        // We don't want unit tests to use the same QSettings space as the normal app. So we tweak the app
        // name. Also we want to run unit tests with clean settings every time.
        setApplicationName(QString("%1_unittest").arg(QGC_APPLICATION_NAME));
    } else {
        setApplicationName(QGC_APPLICATION_NAME);
    }
    setOrganizationName(QGC_ORG_NAME);
    setOrganizationDomain(QGC_ORG_DOMAIN);
    
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
        { "--clear-settings",   &fClearSettingsOptions, QString() },
        // Add additional command line option flags here
    };
    
    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, sizeof(rgCmdLineOptions)/sizeof(rgCmdLineOptions[0]), false);
    
    QSettings settings;
    
    // The setting will delete all settings on this boot
    fClearSettingsOptions |= settings.contains(_deleteAllSettingsKey);
    
    if (_runningUnitTests) {
        // Unit tests run with clean settings
        fClearSettingsOptions = true;
    }
    
    if (fClearSettingsOptions) {
        // User requested settings to be cleared on command line
        settings.clear();
        settings.setValue(_settingsVersionKey, QGC_SETTINGS_VERSION);
    }
}

QGCApplication::~QGCApplication()
{
    destroySingletonsForUnitTest();
}

void QGCApplication::_initCommon(void)
{
    QSettings settings;
    
    _createSingletons();
    
    // Show user an upgrade message if the settings version has been bumped up
    bool settingsUpgraded = false;
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
        QGCMessageBox::information(tr("Settings Cleared"),
                                   tr("The format for QGroundControl saved settings has been modified. "
                                      "Your saved settings have been reset to defaults."));
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
    }
    
    if (!savedFilesLocation.isEmpty()) {
        if (!validatePossibleSavedFilesLocation(savedFilesLocation)) {
            savedFilesLocation.clear();
        }
    }
    settings.setValue(_savedFilesLocationKey, savedFilesLocation);

    // Load application font
    QFontDatabase fontDatabase = QFontDatabase();
    const QString fontFileName = ":/general/vera.ttf"; ///< Font file is part of the QRC file and compiled into the app
    //const QString fontFamilyName = "Bitstream Vera Sans";
    if(!QFile::exists(fontFileName)) printf("ERROR! font file: %s DOES NOT EXIST!\n", fontFileName.toStdString().c_str());
    fontDatabase.addApplicationFont(fontFileName);
    // Avoid Using setFont(). In the Qt docu you can read the following:
    //     "Warning: Do not use this function in conjunction with Qt Style Sheets."
    // setFont(fontDatabase.font(fontFamilyName, "Roman", 12));
}

bool QGCApplication::_initForNormalAppBoot(void)
{
    QSettings settings;
    
    enum MainWindow::CUSTOM_MODE mode = (enum MainWindow::CUSTOM_MODE) settings.value("QGC_CUSTOM_MODE", (int)MainWindow::CUSTOM_MODE_PX4).toInt();
    
    // Show splash screen
    QPixmap splashImage(":/files/images/splash.png");
    QSplashScreen* splashScreen = new QSplashScreen(splashImage);
    // Delete splash screen after mainWindow was displayed
    splashScreen->setAttribute(Qt::WA_DeleteOnClose);
    splashScreen->show();
    processEvents();
    splashScreen->showMessage(tr("Loading application fonts"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    
    // Exit main application when last window is closed
    connect(this, SIGNAL(lastWindowClosed()), this, SLOT(quit()));

    // Start the user interface
    splashScreen->showMessage(tr("Starting user interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    MainWindow* mainWindow = MainWindow::_create(splashScreen, mode);
    Q_CHECK_PTR(mainWindow);
    
    // If we made it this far and we still don't have a location. Either the specfied location was invalid
    // or we coudn't create a default location. Either way, we need to let the user know and prompt for a new
    /// settings.
    QString savedFilesLocation = settings.value(_savedFilesLocationKey).toString();
    if (savedFilesLocation.isEmpty()) {
        QGCMessageBox::warning(tr("Bad save location"),
                               tr("The location to save files to is invalid, or cannot be written to. Please provide a new one."));
        mainWindow->showSettings();
    }
    
    UDPLink* udpLink = NULL;
    
    if (mainWindow->getCustomMode() == MainWindow::CUSTOM_MODE_WIFI)
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
    _mainWindow->addLink(opalLink);
#endif
    
    // Remove splash screen
    splashScreen->finish(mainWindow);
    mainWindow->splashScreenFinished();
    
    
    // Check if link could be connected
    if (udpLink && LinkManager::instance()->connectLink(udpLink))
    {
        QMessageBox::StandardButton button = QGCMessageBox::critical(tr("Could not connect UDP port. Is an instance of %1 already running?").arg(qAppName()),
                                                                     tr("It is recommended to close the application and stop all instances. Click Yes to close."),
                                                                     QMessageBox::Yes | QMessageBox::No,
                                                                     QMessageBox::No);
        // Exit application
        if (button == QMessageBox::Yes)
        {
            //mainWindow->close();
            QTimer::singleShot(200, mainWindow, SLOT(close()));
        }
    }
    
    return true;
}

bool QGCApplication::_initForUnitTests(void)
{
    return true;
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
    QGCTemporaryFile tempFile(filename);
    if (!tempFile.open()) {
        return false;
    }
    
    tempFile.remove();
    
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

/// @brief We create all the non-ui based singletons here instead of allowing them to be created randomly
///         by calls to instance. The reason being that depending on boot sequence the singleton may end
///         up being creating on something other than the main thread.
void QGCApplication::_createSingletons(void)
{
    
    LinkManager* linkManager = LinkManager::instance();
    Q_UNUSED(linkManager);
    Q_ASSERT(linkManager);

    UASManagerInterface* uasManager = UASManager::instance();
    Q_UNUSED(uasManager);
    Q_ASSERT(uasManager);

    AutoPilotPluginManager* pluginManager = AutoPilotPluginManager::instance();
    Q_UNUSED(pluginManager);
    Q_ASSERT(pluginManager);

    // Must be after UASManager since FactSystem connects to UASManager
    FactSystem* factSystem = FactSystem::instance();
    Q_UNUSED(factSystem);
    Q_ASSERT(factSystem);
}

void QGCApplication::destroySingletonsForUnitTest(void)
{
    foreach(QGCSingleton* singleton, _singletons) {
        Q_ASSERT(singleton);
        singleton->deleteInstance();
    }
    
    if (MainWindow::instance()) {
        delete MainWindow::instance();
    }
    
    _singletons.clear();
}

void QGCApplication::registerSingleton(QGCSingleton* singleton)
{
    Q_ASSERT(singleton);
    Q_ASSERT(!_singletons.contains(singleton));
    
    _singletons.append(singleton);
}
