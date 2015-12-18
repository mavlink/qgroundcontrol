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
#include <QPixmap>
#include <QDesktopWidget>
#include <QPainter>
#include <QStyleFactory>
#include <QAction>

#ifdef QGC_ENABLE_BLUETOOTH
#include <QBluetoothLocalDevice>
#endif

#include <QDebug>

#include "VideoStreaming.h"

#include "QGC.h"
#include "QGCApplication.h"
#include "GAudioOutput.h"
#include "CmdLineOptParser.h"
#include "UDPLink.h"
#include "LinkManager.h"
#include "HomePositionManager.h"
#include "UASMessageHandler.h"
#include "AutoPilotPluginManager.h"
#include "QGCTemporaryFile.h"
#include "QGCPalette.h"
#include "QGCMapPalette.h"
#include "QGCLoggingCategory.h"
#include "ViewWidgetController.h"
#include "ParameterEditorController.h"
#include "CustomCommandWidgetController.h"
#include "FlightModesComponentController.h"
#include "APMFlightModesComponentController.h"
#include "AirframeComponentController.h"
#include "SensorsComponentController.h"
#include "APMSensorsComponentController.h"
#include "PowerComponentController.h"
#include "RadioComponentController.h"
#include "ScreenToolsController.h"
#include "AutoPilotPlugin.h"
#include "VehicleComponent.h"
#include "FirmwarePluginManager.h"
#include "MultiVehicleManager.h"
#include "Generic/GenericFirmwarePlugin.h"
#include "APM/ArduCopterFirmwarePlugin.h"
#include "APM/ArduPlaneFirmwarePlugin.h"
#include "APM/ArduRoverFirmwarePlugin.h"
#include "APM/APMAirframeComponentController.h"
#include "PX4/PX4FirmwarePlugin.h"
#include "Vehicle.h"
#include "MavlinkQmlSingleton.h"
#include "JoystickManager.h"
#include "QmlObjectListModel.h"
#include "MissionManager.h"
#include "QGroundControlQmlGlobal.h"
#include "HomePositionManager.h"
#include "FlightMapSettings.h"
#include "QGCQGeoCoordinate.h"
#include "CoordinateVector.h"
#include "MainToolBarController.h"
#include "MissionController.h"
#include "MissionCommands.h"
#include "FlightDisplayViewController.h"
#include "VideoSurface.h"
#include "VideoReceiver.h"

#ifndef __ios__
    #include "SerialLink.h"
#endif

#ifndef __mobile__
    #include "QGCFileDialog.h"
    #include "QGCMessageBox.h"
    #include "FirmwareUpgradeController.h"
    #include "JoystickConfigController.h"
    #include "MainWindow.h"
#endif

#ifdef QGC_RTLAB_ENABLED
    #include "OpalLink.h"
#endif


QGCApplication* QGCApplication::_app = NULL;

const char* QGCApplication::_deleteAllSettingsKey           = "DeleteAllSettingsNextBoot";
const char* QGCApplication::_settingsVersionKey             = "SettingsVersion";
const char* QGCApplication::_savedFilesLocationKey          = "SavedFilesLocation";
const char* QGCApplication::_promptFlightDataSave           = "PromptFLightDataSave";
const char* QGCApplication::_promptFlightDataSaveNotArmed   = "PromptFLightDataSaveNotArmed";
const char* QGCApplication::_styleKey                       = "StyleIsDark";

const char* QGCApplication::_defaultSavedFileDirectoryName      = "QGroundControl";
const char* QGCApplication::_savedFileMavlinkLogDirectoryName   = "FlightData";
const char* QGCApplication::_savedFileParameterDirectoryName    = "SavedParameters";

const char* QGCApplication::_darkStyleFile          = ":/res/styles/style-dark.css";
const char* QGCApplication::_lightStyleFile         = ":/res/styles/style-light.css";


// Qml Singleton factories

static QObject* screenToolsControllerSingletonFactory(QQmlEngine*, QJSEngine*)
{
    ScreenToolsController* screenToolsController = new ScreenToolsController;
    return screenToolsController;
}

static QObject* mavlinkQmlSingletonFactory(QQmlEngine*, QJSEngine*)
{
    return new MavlinkQmlSingleton;
}

static QObject* qgroundcontrolQmlGlobalSingletonFactory(QQmlEngine*, QJSEngine*)
{
    // We create this object as a QGCTool even though it isn't int he toolbox
    QGroundControlQmlGlobal* qmlGlobal = new QGroundControlQmlGlobal(qgcApp());
    qmlGlobal->setToolbox(qgcApp()->toolbox());

    return qmlGlobal;
}

/**
 * @brief Constructor for the main application.
 *
 * This constructor initializes and starts the whole application. It takes standard
 * command-line parameters
 *
 * @param argc The number of command-line parameters
 * @param argv The string array of parameters
 **/

QGCApplication::QGCApplication(int &argc, char* argv[], bool unitTesting)
#ifdef __mobile__
    : QGuiApplication(argc, argv)
    , _qmlAppEngine(NULL)
#else
    : QApplication(argc, argv)
#endif
    , _runningUnitTests(unitTesting)
#if defined (__mobile__)
    , _styleIsDark(false)
#else
    , _styleIsDark(true)
#endif
    , _fakeMobile(false)
#ifdef QT_DEBUG
    , _testHighDPI(false)
#endif
    , _toolbox(NULL)
    , _bluetoothAvailable(false)
{
    Q_ASSERT(_app == NULL);
    _app = this;

    // This prevents usage of QQuickWidget to fail since it doesn't support native widget siblings
#ifndef __android__
    setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif

    // Parse command line options

    bool fClearSettingsOptions = false; // Clear stored settings
    bool logging = false;               // Turn on logging
    QString loggingOptions;

    CmdLineOpt_t rgCmdLineOptions[] = {
        { "--clear-settings",   &fClearSettingsOptions, NULL },
        { "--logging",          &logging,               &loggingOptions },
        { "--fake-mobile",      &_fakeMobile,           NULL },
#ifdef QT_DEBUG
        { "--test-high-dpi",    &_testHighDPI,          NULL },
#endif
        // Add additional command line option flags here
    };

    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, sizeof(rgCmdLineOptions)/sizeof(rgCmdLineOptions[0]), false);

#ifdef __mobile__
    QLoggingCategory::setFilterRules(QStringLiteral("*Log.debug=false"));
#else
    QString filterRules;

    // Turn off bogus ssl warning
    filterRules += "qt.network.ssl.warning=false\n";

    if (logging) {
        QStringList logList = loggingOptions.split(",");

        if (logList[0] == "full") {
            filterRules += "*Log.debug=true\n";
            for(int i=1; i<logList.count(); i++) {
                filterRules += logList[i];
                filterRules += ".debug=false\n";
            }
        } else {
            foreach(QString rule, logList) {
                filterRules += rule;
                filterRules += ".debug=true\n";
            }
        }
    } else {
        // First thing we want to do is set up the qtlogging.ini file. If it doesn't already exist we copy
        // it to the correct location. This way default debug builds will have logging turned off.

        static const char* qtProjectDir = "QtProject";
        static const char* qtLoggingFile = "qtlogging.ini";
        bool loggingDirectoryOk = false;

        QDir iniFileLocation(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
        if (!iniFileLocation.cd(qtProjectDir)) {
            if (!iniFileLocation.mkdir(qtProjectDir)) {
                qDebug() << "Unable to create qtlogging.ini directory" << iniFileLocation.filePath(qtProjectDir);
            } else {
                if (!iniFileLocation.cd(qtProjectDir)) {
                    qDebug() << "Unable to access qtlogging.ini directory" << iniFileLocation.filePath(qtProjectDir);;
                }
                loggingDirectoryOk = true;
            }
        } else {
            loggingDirectoryOk = true;
        }

        if (loggingDirectoryOk) {
            qDebug () << "Logging ini file directory" << iniFileLocation.absolutePath();
            if (!iniFileLocation.exists(qtLoggingFile)) {
                QFile loggingFile(iniFileLocation.filePath(qtLoggingFile));
                if (loggingFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&loggingFile);
                    out << "[Rules]\n";
                    out << "*Log.debug=false\n";
                    foreach(QString category, QGCLoggingCategoryRegister::instance()->registeredCategories()) {
                        out << category << ".debug=false\n";
                    }
                } else {
                    qDebug() << "Unable to create logging file" << QString(qtLoggingFile) << "in" << iniFileLocation;
                }
            }
        }
    }

    qDebug() << "Filter rules" << filterRules;
    QLoggingCategory::setFilterRules(filterRules);
#endif

    // Set up timer for delayed missing fact display
    _missingParamsDelayedDisplayTimer.setSingleShot(true);
    _missingParamsDelayedDisplayTimer.setInterval(_missingParamsDelayedDisplayTimerTimeout);
    connect(&_missingParamsDelayedDisplayTimer, &QTimer::timeout, this, &QGCApplication::_missingParamsDisplay);

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

    QString versionString(GIT_VERSION);
    // stable versions are on tags (v1.2.3)
    // development versions are full git describe versions (v1.2.3-18-g879e8b3)
    if (versionString.length() > 8) {
        versionString.append(" (Development)");
    }
    this->setApplicationVersion(versionString);

    // Set settings format
#if !defined(__mobile__) && !defined(__macos__)
    QSettings::setDefaultFormat(QSettings::IniFormat);
#else
    QString settingsLocation = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if(!settingsLocation.isEmpty())
    {
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsLocation);
    }
#endif

    QSettings settings;
    qDebug() << "Settings location" << settings.fileName() << settings.isWritable();

#ifdef UNITTEST_BUILD
    if (!settings.isWritable()) {
        qWarning() << "Setings location is not writable";
    }
#endif
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

    // Initialize Bluetooth
#ifdef QGC_ENABLE_BLUETOOTH
    QBluetoothLocalDevice localDevice;
    if (localDevice.isValid())
    {
        _bluetoothAvailable = true;
    }
#endif

    // Initialize Video Streaming
    initializeVideoStreaming(argc, argv);

    _toolbox = new QGCToolbox(this);
}

QGCApplication::~QGCApplication()
{
#ifndef __mobile__
    MainWindow* mainWindow = MainWindow::instance();
    if (mainWindow) {
        delete mainWindow;
    }
#endif
    shutdownVideoStreaming();
    delete _toolbox;
}

void QGCApplication::_initCommon(void)
{
    QSettings settings;

    // Register our Qml objects

    qmlRegisterType<QGCPalette>     ("QGroundControl.Palette", 1, 0, "QGCPalette");
    qmlRegisterType<QGCMapPalette>  ("QGroundControl.Palette", 1, 0, "QGCMapPalette");

    qmlRegisterUncreatableType<CoordinateVector>    ("QGroundControl",                  1, 0, "CoordinateVector",       "Reference only");
    qmlRegisterUncreatableType<MissionCommands>     ("QGroundControl",                  1, 0, "MissionCommands",        "Reference only");
    qmlRegisterUncreatableType<QGCQGeoCoordinate>   ("QGroundControl",                  1, 0, "QGCQGeoCoordinate",      "Reference only");
    qmlRegisterUncreatableType<QmlObjectListModel>  ("QGroundControl",                  1, 0, "QmlObjectListModel",     "Reference only");
    qmlRegisterUncreatableType<VideoReceiver>       ("QGroundControl",                  1, 0, "VideoReceiver",          "Reference only");
    qmlRegisterUncreatableType<VideoSurface>        ("QGroundControl",                  1, 0, "VideoSurface",           "Reference only");

    qmlRegisterUncreatableType<AutoPilotPlugin>     ("QGroundControl.AutoPilotPlugin",  1, 0, "AutoPilotPlugin",        "Reference only");
    qmlRegisterUncreatableType<VehicleComponent>    ("QGroundControl.AutoPilotPlugin",  1, 0, "VehicleComponent",       "Reference only");
    qmlRegisterUncreatableType<Vehicle>             ("QGroundControl.Vehicle",          1, 0, "Vehicle",                "Reference only");
    qmlRegisterUncreatableType<MissionItem>         ("QGroundControl.Vehicle",          1, 0, "MissionItem",            "Reference only");
    qmlRegisterUncreatableType<MissionManager>      ("QGroundControl.Vehicle",          1, 0, "MissionManager",         "Reference only");
    qmlRegisterUncreatableType<JoystickManager>     ("QGroundControl.JoystickManager",  1, 0, "JoystickManager",        "Reference only");
    qmlRegisterUncreatableType<Joystick>            ("QGroundControl.JoystickManager",  1, 0, "Joystick",               "Reference only");

    qmlRegisterType<ParameterEditorController>      ("QGroundControl.Controllers", 1, 0, "ParameterEditorController");
    qmlRegisterType<APMFlightModesComponentController>  ("QGroundControl.Controllers", 1, 0, "APMFlightModesComponentController");
    qmlRegisterType<FlightModesComponentController>     ("QGroundControl.Controllers", 1, 0, "FlightModesComponentController");
    qmlRegisterType<APMAirframeComponentController>     ("QGroundControl.Controllers", 1, 0, "APMAirframeComponentController");
    qmlRegisterType<AirframeComponentController>        ("QGroundControl.Controllers", 1, 0, "AirframeComponentController");
    qmlRegisterType<APMSensorsComponentController>      ("QGroundControl.Controllers", 1, 0, "APMSensorsComponentController");
    qmlRegisterType<SensorsComponentController>         ("QGroundControl.Controllers", 1, 0, "SensorsComponentController");
    qmlRegisterType<PowerComponentController>           ("QGroundControl.Controllers", 1, 0, "PowerComponentController");
    qmlRegisterType<RadioComponentController>           ("QGroundControl.Controllers", 1, 0, "RadioComponentController");
    qmlRegisterType<ScreenToolsController>              ("QGroundControl.Controllers", 1, 0, "ScreenToolsController");
    qmlRegisterType<MainToolBarController>              ("QGroundControl.Controllers", 1, 0, "MainToolBarController");
    qmlRegisterType<MissionController>                  ("QGroundControl.Controllers", 1, 0, "MissionController");
    qmlRegisterType<FlightDisplayViewController>        ("QGroundControl.Controllers", 1, 0, "FlightDisplayViewController");

#ifndef __mobile__
    qmlRegisterType<ViewWidgetController>           ("QGroundControl.Controllers", 1, 0, "ViewWidgetController");
    qmlRegisterType<CustomCommandWidgetController>  ("QGroundControl.Controllers", 1, 0, "CustomCommandWidgetController");
    qmlRegisterType<FirmwareUpgradeController>      ("QGroundControl.Controllers", 1, 0, "FirmwareUpgradeController");
    qmlRegisterType<JoystickConfigController>       ("QGroundControl.Controllers", 1, 0, "JoystickConfigController");
#endif

    // Register Qml Singletons
    qmlRegisterSingletonType<QGroundControlQmlGlobal>   ("QGroundControl",                          1, 0, "QGroundControl",         qgroundcontrolQmlGlobalSingletonFactory);
    qmlRegisterSingletonType<ScreenToolsController>     ("QGroundControl.ScreenToolsController",    1, 0, "ScreenToolsController",  screenToolsControllerSingletonFactory);
    qmlRegisterSingletonType<MavlinkQmlSingleton>       ("QGroundControl.Mavlink",                  1, 0, "Mavlink",                mavlinkQmlSingletonFactory);

    // Show user an upgrade message if the settings version has been bumped up
    bool settingsUpgraded = false;
    if (settings.contains(_settingsVersionKey)) {
        if (settings.value(_settingsVersionKey).toInt() != QGC_SETTINGS_VERSION) {
            settingsUpgraded = true;
        }
    } else if (settings.allKeys().count()) {
        // Settings version key is missing and there are settings. This is an upgrade scenario.
        settingsUpgraded = true;
    } else {
        settings.setValue(_settingsVersionKey, QGC_SETTINGS_VERSION);
    }

    if (settingsUpgraded) {
        settings.clear();
        settings.setValue(_settingsVersionKey, QGC_SETTINGS_VERSION);
        showMessage("The format for QGroundControl saved settings has been modified. "
                    "Your saved settings have been reset to defaults.");
    }

    // Load saved files location and validate

    QString savedFilesLocation;
    if (settings.contains(_savedFilesLocationKey)) {
        savedFilesLocation = settings.value(_savedFilesLocationKey).toString();
        if (!validatePossibleSavedFilesLocation(savedFilesLocation)) {
            savedFilesLocation.clear();
        }
    }

    if (savedFilesLocation.isEmpty()) {
        // No location set (or invalid). Create a default one in Documents standard location.

        QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

        QDir documentsDir(documentsLocation);
        if (!documentsDir.exists()) {
            qWarning() << "Documents directory doesn't exist" << documentsDir.absolutePath();
        }

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
    qDebug() << "Saved files location" << savedFilesLocation;
    settings.setValue(_savedFilesLocationKey, savedFilesLocation);

    settings.sync();
}

bool QGCApplication::_initForNormalAppBoot(void)
{
    QSettings settings;

    _styleIsDark = settings.value(_styleKey, _styleIsDark).toBool();
    _loadCurrentStyle();

    // Exit main application when last window is closed
    connect(this, SIGNAL(lastWindowClosed()), this, SLOT(quit()));

#ifdef __mobile__
    _qmlAppEngine = new QQmlApplicationEngine(this);
    _qmlAppEngine->addImportPath("qrc:/qml");
    _qmlAppEngine->rootContext()->setContextProperty("multiVehicleManager", toolbox()->multiVehicleManager());
    _qmlAppEngine->rootContext()->setContextProperty("joystickManager", toolbox()->joystickManager());
    _qmlAppEngine->load(QUrl(QStringLiteral("qrc:/qml/MainWindowNative.qml")));
#else
    // Start the user interface
    MainWindow* mainWindow = MainWindow::_create();
    Q_CHECK_PTR(mainWindow);

    // If we made it this far and we still don't have a location. Either the specfied location was invalid
    // or we coudn't create a default location. Either way, we need to let the user know and prompt for a new
    /// settings.
    QString savedFilesLocation = settings.value(_savedFilesLocationKey).toString();
    if (savedFilesLocation.isEmpty()) {
        showMessage("The location to save files to is invalid, or cannot be written to. Please provide a new one.");
    }

    // Now that main window is up check for lost log files
    connect(this, &QGCApplication::checkForLostLogFiles, toolbox()->mavlinkProtocol(), &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();
#endif

    // Load known link configurations
    toolbox()->linkManager()->loadLinkConfigurationList();

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

bool QGCApplication::promptFlightDataSaveNotArmed(void)
{
    QSettings settings;

    return settings.value(_promptFlightDataSaveNotArmed, false).toBool();
}

void QGCApplication::setPromptFlightDataSave(bool promptForSave)
{
    QSettings settings;
    settings.setValue(_promptFlightDataSave, promptForSave);
}

void QGCApplication::setPromptFlightDataSaveNotArmed(bool promptForSave)
{
    QSettings settings;
    settings.setValue(_promptFlightDataSaveNotArmed, promptForSave);
}

/// @brief Returns the QGCApplication object singleton.
QGCApplication* qgcApp(void)
{
    Q_ASSERT(QGCApplication::_app);
    return QGCApplication::_app;
}

void QGCApplication::informationMessageBoxOnMainThread(const QString& title, const QString& msg)
{
    Q_UNUSED(title);
    showMessage(msg);
}

void QGCApplication::warningMessageBoxOnMainThread(const QString& title, const QString& msg)
{
#ifdef __mobile__
    Q_UNUSED(title)
    showMessage(msg);
#else
    QGCMessageBox::warning(title, msg);
#endif
}

void QGCApplication::criticalMessageBoxOnMainThread(const QString& title, const QString& msg)
{
#ifdef __mobile__
    Q_UNUSED(title)
    showMessage(msg);
#else
    QGCMessageBox::critical(title, msg);
#endif
}

#ifndef __mobile__
void QGCApplication::saveTempFlightDataLogOnMainThread(QString tempLogfile)
{
    bool saveError;
    do{
        saveError = false;
        QString saveFilename = QGCFileDialog::getSaveFileName(
            MainWindow::instance(),
            tr("Save Flight Data Log"),
            qgcApp()->mavlinkLogFilesLocation(),
            tr("Flight Data Log Files (*.mavlink)"),
            "mavlink");

        if (!saveFilename.isEmpty()) {
            // if file exsits already, try to remove it first to overwrite it
            if(QFile::exists(saveFilename) && !QFile::remove(saveFilename)){
                // if the file cannot be removed, prompt user and ask new path
                saveError = true;
                QGCMessageBox::warning("File Error","Could not overwrite existing file.\nPlease provide a different file name to save to.");
            } else if(!QFile::copy(tempLogfile, saveFilename)) {
                // if file could not be copied, prompt user and ask new path
                saveError = true;
                QGCMessageBox::warning("File Error","Could not create file.\nPlease provide a different file name to save to.");
            }
        }
    } while(saveError); // if the file could not be overwritten, ask for new file
    QFile::remove(tempLogfile);
}
#endif

void QGCApplication::setStyle(bool styleIsDark)
{
    QSettings settings;

    settings.setValue(_styleKey, styleIsDark);
    _styleIsDark = styleIsDark;
    _loadCurrentStyle();
    emit styleChanged(_styleIsDark);
}

void QGCApplication::_loadCurrentStyle(void)
{
#ifndef __mobile__
    bool success = true;
    QString styles;

    // The dark style sheet is the master. Any other selected style sheet just overrides
    // the colors of the master sheet.
    QFile masterStyleSheet(_darkStyleFile);
    if (masterStyleSheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
        styles = masterStyleSheet.readAll();
    } else {
        qDebug() << "Unable to load master dark style sheet";
        success = false;
    }

    if (success && !_styleIsDark) {
        qDebug() << "LOADING LIGHT";
        // Load the slave light stylesheet.
        QFile styleSheet(_lightStyleFile);
        if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
            styles += styleSheet.readAll();
        } else {
            qDebug() << "Unable to load slave light sheet:";
            success = false;
        }
    }

    setStyleSheet(styles);

    if (!success) {
        // Fall back to plastique if we can't load our own
        setStyle("plastique");
    }
#endif

    QGCPalette::setGlobalTheme(_styleIsDark ? QGCPalette::Dark : QGCPalette::Light);
}

void QGCApplication::reportMissingParameter(int componentId, const QString& name)
{
    _missingParams += QString("%1:%2").arg(componentId).arg(name);
    _missingParamsDelayedDisplayTimer.start();
}

/// Called when the delay timer fires to show the missing parameters warning
void QGCApplication::_missingParamsDisplay(void)
{
    Q_ASSERT(_missingParams.count());

    QString params;
    foreach (QString name, _missingParams) {
        if (params.isEmpty()) {
            params += name;
        } else {
            params += QString(", %1").arg(name);
        }
    }
    _missingParams.clear();

    showMessage(QString("Parameters missing from firmware: %1.\n\nYou should quit QGroundControl immediately and update your firmware.").arg(params));
}

QObject* QGCApplication::_rootQmlObject(void)
{
#ifdef __mobile__
    return _qmlAppEngine->rootObjects()[0];
#else
    MainWindow * mainWindow = MainWindow::instance();
    if (mainWindow) {
        return mainWindow->rootQmlObject();
    } else if (runningUnitTests()){
        // Unit test can run without a main window
        return NULL;
    } else {
        qWarning() << "Why is MainWindow missing?";
        return NULL;
    }
#endif
}


void QGCApplication::showMessage(const QString& message)
{
    QObject* rootQmlObject = _rootQmlObject();

    if (rootQmlObject) {
        QVariant varReturn;
        QVariant varMessage = QVariant::fromValue(message);

        QMetaObject::invokeMethod(_rootQmlObject(), "showMessage", Q_RETURN_ARG(QVariant, varReturn), Q_ARG(QVariant, varMessage));
#ifndef __mobile__
    } else if (runningUnitTests()){
        // Unit test can run without a main window which will lead to no root qml object. Use QGCMessageBox instead
        QGCMessageBox::information("Unit Test", message);
#endif
    } else {
        qWarning() << "Internal error";
    }
}

void QGCApplication::showFlyView(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "showFlyView");
}

void QGCApplication::showPlanView(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "showPlanView");
}

void QGCApplication::showSetupView(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "showSetupView");
}

void QGCApplication::showWindowCloseMessage(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "showWindowCloseMessage");
}


void QGCApplication::_showSetupFirmware(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "showSetupFirmware");
}

void QGCApplication::_showSetupParameters(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "showSetupParameters");
}

void QGCApplication::_showSetupSummary(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "showSetupSummary");
}

void QGCApplication::_showSetupVehicleComponent(VehicleComponent* vehicleComponent)
{
    QVariant varReturn;
    QVariant varComponent = QVariant::fromValue(vehicleComponent);

    QMetaObject::invokeMethod(_rootQmlObject(), "showSetupVehicleComponent", Q_RETURN_ARG(QVariant, varReturn), Q_ARG(QVariant, varComponent));
}
