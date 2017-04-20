/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
#include <QStringListModel>

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
#include "UASMessageHandler.h"
#include "QGCTemporaryFile.h"
#include "QGCPalette.h"
#include "QGCMapPalette.h"
#include "QGCLoggingCategory.h"
#include "ViewWidgetController.h"
#include "ParameterEditorController.h"
#include "CustomCommandWidgetController.h"
#include "ESP8266ComponentController.h"
#include "ScreenToolsController.h"
#include "QFileDialogController.h"
#include "RCChannelMonitorController.h"
#include "AutoPilotPlugin.h"
#include "VehicleComponent.h"
#include "FirmwarePluginManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MavlinkQmlSingleton.h"
#include "JoystickConfigController.h"
#include "JoystickManager.h"
#include "QmlObjectListModel.h"
#include "MissionManager.h"
#include "QGroundControlQmlGlobal.h"
#include "FlightMapSettings.h"
#include "CoordinateVector.h"
#include "PlanMasterController.h"
#include "VideoManager.h"
#include "VideoSurface.h"
#include "VideoReceiver.h"
#include "LogDownloadController.h"
#include "ValuesWidgetController.h"
#include "AppMessages.h"
#include "SimulatedPosition.h"
#include "PositionManager.h"
#include "FollowMe.h"
#include "MissionCommandTree.h"
#include "QGCMapPolygon.h"
#include "ParameterManager.h"
#include "SettingsManager.h"

#ifndef NO_SERIAL_LINK
#include "SerialLink.h"
#endif

#ifndef __mobile__
#include "QGCQFileDialog.h"
#include "QGCMessageBox.h"
#include "FirmwareUpgradeController.h"
#include "MainWindow.h"
#include "GeoTagController.h"
#include "MavlinkConsoleController.h"
#endif

#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif

#ifdef Q_OS_LINUX
#ifndef __mobile__
#include <unistd.h>
#include <sys/types.h>
#endif
#endif

#include "QGCMapEngine.h"

QGCApplication* QGCApplication::_app = NULL;

const char* QGCApplication::_deleteAllSettingsKey           = "DeleteAllSettingsNextBoot";
const char* QGCApplication::_settingsVersionKey             = "SettingsVersion";

const char* QGCApplication::_darkStyleFile          = ":/res/styles/style-dark.css";
const char* QGCApplication::_lightStyleFile         = ":/res/styles/style-light.css";

// Mavlink status structures for entire app
mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];

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
    // We create this object as a QGCTool even though it isn't in the toolbox
    QGroundControlQmlGlobal* qmlGlobal = new QGroundControlQmlGlobal(qgcApp(), qgcApp()->toolbox());
    qmlGlobal->setToolbox(qgcApp()->toolbox());

    return qmlGlobal;
}

#ifdef __android__
// breakpad support
#include "client/linux/handler/exception_handler.h"

static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
  qDebug() << "dumpCallback" << succeeded << descriptor.path();
  return succeeded;
}
#endif


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
    , _fakeMobile(false)
    , _settingsUpgraded(false)
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

    // Setup for network proxy support
    QNetworkProxyFactory::setUseSystemConfiguration(true);

#ifdef Q_OS_LINUX
#ifndef __mobile__
    if (!_runningUnitTests) {
        if (getuid() == 0) {
            QMessageBox msgBox;
            msgBox.setInformativeText(tr("You are running %1 as root. "
                                      "You should not do this since it will cause other issues with %1. "
                                      "%1 will now exit. "
                                      "If you are having serial port issues on Ubuntu, execute the following commands to fix most issues:\n"
                                      "sudo usermod -a -G dialout $USER\n"
                                      "sudo apt-get remove modemmanager").arg(qgcApp()->applicationName()));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            _exit(0);
        }

        // Determine if we have the correct permissions to access USB serial devices
        QFile permFile("/etc/group");
        if(permFile.open(QIODevice::ReadOnly)) {
            while(!permFile.atEnd()) {
                QString line = permFile.readLine();
                if (line.contains("dialout") && !line.contains(getenv("USER"))) {
                    QMessageBox msgBox;
                    msgBox.setInformativeText("The current user does not have the correct permissions to access serial devices. "
                                              "You should also remove modemmanager since it also interferes. "
                                              "If you are using Ubuntu, execute the following commands to fix these issues:\n"
                                              "sudo usermod -a -G dialout $USER\n"
                                              "sudo apt-get remove modemmanager");
                    msgBox.setStandardButtons(QMessageBox::Ok);
                    msgBox.setDefaultButton(QMessageBox::Ok);
                    msgBox.exec();
                    break;
                }
            }
            permFile.close();
        }
    }
#endif
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

    this->setApplicationVersion(QString(GIT_VERSION));

    // Set settings format
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;
    qDebug() << "Settings location" << settings.fileName() << "Is writable?:" << settings.isWritable();

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

        // Clear parameter cache
        QDir paramDir(ParameterManager::parameterCacheDir());
        paramDir.removeRecursively();
        paramDir.mkpath(paramDir.absolutePath());
    } else {
        // Determine if upgrade message for settings version bump is required. Check and clear must happen before toolbox is started since
        // that will write some settings.
        if (settings.contains(_settingsVersionKey)) {
            if (settings.value(_settingsVersionKey).toInt() != QGC_SETTINGS_VERSION) {
                settings.clear();
                _settingsUpgraded = true;
            }
        } else if (settings.allKeys().count()) {
            // Settings version key is missing and there are settings. This is an upgrade scenario.
            settings.clear();
            _settingsUpgraded = true;
        }
    }
    settings.setValue(_settingsVersionKey, QGC_SETTINGS_VERSION);

    // Set up our logging filters
    QGCLoggingCategoryRegister::instance()->setFilterRulesFromSettings(loggingOptions);

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
    _toolbox->setChildToolboxes();

#ifdef __android__
    std::string pathAsStr = toolbox()->settingsManager()->appSettings()->savePath()->rawValue().toString().toStdString();
    qDebug() << "dump location" << QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    google_breakpad::MinidumpDescriptor descriptor(pathAsStr);
    google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback, NULL, true, -1);
#endif
}

void QGCApplication::_shutdown(void)
{
    // This code is specifically not in the destructor since the application object may not be available in the destructor.
    // This cause problems for deleting object like settings which are in the toolbox which may have qml references. By
    // moving them here and having main.cc call this prior to deleting the app object we make sure app object is still
    // around while these things are shutting down.
#ifndef __mobile__
    MainWindow* mainWindow = MainWindow::instance();
    if (mainWindow) {
        delete mainWindow;
    }
#endif
    shutdownVideoStreaming();
    delete _toolbox;
}

QGCApplication::~QGCApplication()
{
    // Place shutdown code in _shutdown
}

void QGCApplication::_initCommon(void)
{
    QSettings settings;

    // Register our Qml objects

    qmlRegisterType<QGCPalette>     ("QGroundControl.Palette", 1, 0, "QGCPalette");
    qmlRegisterType<QGCMapPalette>  ("QGroundControl.Palette", 1, 0, "QGCMapPalette");

    qmlRegisterUncreatableType<CoordinateVector>    ("QGroundControl",                  1, 0, "CoordinateVector",       "Reference only");
    qmlRegisterUncreatableType<QmlObjectListModel>  ("QGroundControl",                  1, 0, "QmlObjectListModel",     "Reference only");
    qmlRegisterUncreatableType<VideoReceiver>       ("QGroundControl",                  1, 0, "VideoReceiver",          "Reference only");
    qmlRegisterUncreatableType<VideoSurface>        ("QGroundControl",                  1, 0, "VideoSurface",           "Reference only");
    qmlRegisterUncreatableType<MissionCommandTree>  ("QGroundControl",                  1, 0, "MissionCommandTree",     "Reference only");

    qmlRegisterUncreatableType<AutoPilotPlugin>     ("QGroundControl.AutoPilotPlugin",      1, 0, "AutoPilotPlugin",        "Reference only");
    qmlRegisterUncreatableType<VehicleComponent>    ("QGroundControl.AutoPilotPlugin",      1, 0, "VehicleComponent",       "Reference only");
    qmlRegisterUncreatableType<Vehicle>             ("QGroundControl.Vehicle",              1, 0, "Vehicle",                "Reference only");
    qmlRegisterUncreatableType<MissionItem>         ("QGroundControl.Vehicle",              1, 0, "MissionItem",            "Reference only");
    qmlRegisterUncreatableType<MissionManager>      ("QGroundControl.Vehicle",              1, 0, "MissionManager",         "Reference only");
    qmlRegisterUncreatableType<ParameterManager>    ("QGroundControl.Vehicle",              1, 0, "ParameterManager",       "Reference only");
    qmlRegisterUncreatableType<JoystickManager>     ("QGroundControl.JoystickManager",      1, 0, "JoystickManager",        "Reference only");
    qmlRegisterUncreatableType<Joystick>            ("QGroundControl.JoystickManager",      1, 0, "Joystick",               "Reference only");
    qmlRegisterUncreatableType<QGCPositionManager>  ("QGroundControl.QGCPositionManager",   1, 0, "QGCPositionManager",     "Reference only");
    qmlRegisterUncreatableType<QGCMapPolygon>       ("QGroundControl.FlightMap",            1, 0, "QGCMapPolygon",          "Reference only");

    qmlRegisterType<ParameterEditorController>          ("QGroundControl.Controllers", 1, 0, "ParameterEditorController");
    qmlRegisterType<ESP8266ComponentController>         ("QGroundControl.Controllers", 1, 0, "ESP8266ComponentController");
    qmlRegisterType<ScreenToolsController>              ("QGroundControl.Controllers", 1, 0, "ScreenToolsController");
    qmlRegisterType<PlanMasterController>        ("QGroundControl.Controllers", 1, 0, "PlanElemementMasterController");
    qmlRegisterType<MissionController>                  ("QGroundControl.Controllers", 1, 0, "MissionController");
    qmlRegisterType<GeoFenceController>                 ("QGroundControl.Controllers", 1, 0, "GeoFenceController");
    qmlRegisterType<RallyPointController>               ("QGroundControl.Controllers", 1, 0, "RallyPointController");
    qmlRegisterType<ValuesWidgetController>             ("QGroundControl.Controllers", 1, 0, "ValuesWidgetController");
    qmlRegisterType<QFileDialogController>      ("QGroundControl.Controllers", 1, 0, "QFileDialogController");
    qmlRegisterType<RCChannelMonitorController>         ("QGroundControl.Controllers", 1, 0, "RCChannelMonitorController");
    qmlRegisterType<JoystickConfigController>           ("QGroundControl.Controllers", 1, 0, "JoystickConfigController");
    qmlRegisterType<LogDownloadController>              ("QGroundControl.Controllers", 1, 0, "LogDownloadController");
#ifndef __mobile__
    qmlRegisterType<ViewWidgetController>           ("QGroundControl.Controllers", 1, 0, "ViewWidgetController");
    qmlRegisterType<CustomCommandWidgetController>  ("QGroundControl.Controllers", 1, 0, "CustomCommandWidgetController");
    qmlRegisterType<FirmwareUpgradeController>      ("QGroundControl.Controllers", 1, 0, "FirmwareUpgradeController");
    qmlRegisterType<GeoTagController>               ("QGroundControl.Controllers", 1, 0, "GeoTagController");
    qmlRegisterType<MavlinkConsoleController>       ("QGroundControl.Controllers", 1, 0, "MavlinkConsoleController");
#endif

    // Register Qml Singletons
    qmlRegisterSingletonType<QGroundControlQmlGlobal>   ("QGroundControl",                          1, 0, "QGroundControl",         qgroundcontrolQmlGlobalSingletonFactory);
    qmlRegisterSingletonType<ScreenToolsController>     ("QGroundControl.ScreenToolsController",    1, 0, "ScreenToolsController",  screenToolsControllerSingletonFactory);
    qmlRegisterSingletonType<MavlinkQmlSingleton>       ("QGroundControl.Mavlink",                  1, 0, "Mavlink",                mavlinkQmlSingletonFactory);
}

bool QGCApplication::_initForNormalAppBoot(void)
{
    QSettings settings;

    _loadCurrentStyleSheet();

    // Exit main application when last window is closed
    connect(this, &QGCApplication::lastWindowClosed, this, QGCApplication::quit);

#ifdef __mobile__
    _qmlAppEngine = new QQmlApplicationEngine(this);
    _qmlAppEngine->addImportPath("qrc:/qml");
    _qmlAppEngine->rootContext()->setContextProperty("joystickManager", toolbox()->joystickManager());
    _qmlAppEngine->rootContext()->setContextProperty("debugMessageModel", AppMessages::getModel());
    _qmlAppEngine->load(QUrl(QStringLiteral("qrc:/qml/MainWindowNative.qml")));
#else
    // Start the user interface
    MainWindow* mainWindow = MainWindow::_create();
    Q_CHECK_PTR(mainWindow);

    // Now that main window is up check for lost log files
    connect(this, &QGCApplication::checkForLostLogFiles, toolbox()->mavlinkProtocol(), &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();
#endif

    // Load known link configurations
    toolbox()->linkManager()->loadLinkConfigurationList();

    // Probe for joysticks
    toolbox()->joystickManager()->init();

    if (_settingsUpgraded) {
        showMessage("The format for QGroundControl saved settings has been modified. "
                    "Your saved settings have been reset to defaults.");
    }

    // Connect links with flag AutoconnectLink
    toolbox()->linkManager()->startAutoConnectedLinks();

    if (getQGCMapEngine()->wasCacheReset()) {
        showMessage("The Offline Map Cache database has been upgraded. "
                    "Your old map cache sets have been reset.");
    }

    settings.sync();
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
void QGCApplication::saveTelemetryLogOnMainThread(QString tempLogfile)
{
    // The vehicle is gone now and we are shutting down so we need to use a message box for errors to hold shutdown and show the error
    if (_checkTelemetrySavePath(true /* useMessageBox */)) {

        QString saveDirPath = _toolbox->settingsManager()->appSettings()->telemetrySavePath();
        QDir saveDir(saveDirPath);

        QString nameFormat("%1%2.tlog");
        QString dtFormat("yyyy-MM-dd hh-mm-ss");

        int tryIndex = 1;
        QString saveFileName = nameFormat.arg(QDateTime::currentDateTime().toString(dtFormat)).arg("");
        while (saveDir.exists(saveFileName)) {
            saveFileName = nameFormat.arg(QDateTime::currentDateTime().toString(dtFormat)).arg(QStringLiteral(".%1").arg(tryIndex++));
        }
        QString saveFilePath = saveDir.absoluteFilePath(saveFileName);

        QFile tempFile(tempLogfile);
        if (!tempFile.copy(saveFilePath)) {
            QGCMessageBox::warning(tr("Telemetry save error"), tr("Unable to save telemetry log. Error copying telemetry to '%1': '%2'.").arg(saveFilePath).arg(tempFile.errorString()));
        }
    }

    QFile::remove(tempLogfile);
}

void QGCApplication::checkTelemetrySavePathOnMainThread(void)
{
    // This is called with an active vehicle so don't pop message boxes which holds ui thread
    _checkTelemetrySavePath(false /* useMessageBox */);
}

bool QGCApplication::_checkTelemetrySavePath(bool useMessageBox)
{
    QString errorTitle = tr("Telemetry save error");

    QString saveDirPath = _toolbox->settingsManager()->appSettings()->telemetrySavePath();
    if (saveDirPath.isEmpty()) {
        QString error = tr("Unable to save telemetry log. Application save directory is not set.");
        if (useMessageBox) {
            QGCMessageBox::warning(errorTitle, error);
        } else {
            showMessage(error);
        }
        return false;
    }

    QDir saveDir(saveDirPath);
    if (!saveDir.exists()) {
        QString error = tr("Unable to save telemetry log. Telemetry save directory \"%1\" does not exist.").arg(saveDirPath);
        if (useMessageBox) {
            QGCMessageBox::warning(errorTitle, error);
        } else {
            showMessage(error);
        }
        return false;
    }

    return true;
}
#endif

void QGCApplication::_loadCurrentStyleSheet(void)
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

    if (success && !_toolbox->settingsManager()->appSettings()->indoorPalette()->rawValue().toBool()) {
        // Load the slave light stylesheet.
        QFile styleSheet(_lightStyleFile);
        if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
            styles += styleSheet.readAll();
        } else {
            qWarning() << "Unable to load slave light sheet:";
            success = false;
        }
    }

    setStyleSheet(styles);

    if (!success) {
        // Fall back to plastique if we can't load our own
        setStyle("plastique");
    }
#endif
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
    foreach (const QString &name, _missingParams) {
        if (params.isEmpty()) {
            params += name;
        } else {
            params += QString(", %1").arg(name);
        }
    }
    _missingParams.clear();

    showMessage(QString("Parameters are missing from firmware. You may be running a version of firmware QGC does not work correctly with or your firmware has a bug in it. Missing params: %1").arg(params));
}

QObject* QGCApplication::_rootQmlObject()
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
    // Special case hack for ArduPilot prearm messages. These show up in the center of the map, so no need for popup.
    if (message.contains("PreArm:")) {
        return;
    }

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

void QGCApplication::showSetupView(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "showSetupView");
}

void QGCApplication::qmlAttemptWindowClose(void)
{
    QMetaObject::invokeMethod(_rootQmlObject(), "attemptWindowClose");
}
