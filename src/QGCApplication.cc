/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include <QRegularExpression>
#include <QFontDatabase>
#include <QQuickWindow>

#ifdef QGC_ENABLE_BLUETOOTH
#include <QBluetoothLocalDevice>
#endif

#include <QDebug>

#include "VideoStreaming.h"

#include "QGC.h"
#include "QGCApplication.h"
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
#include "QGCFileDialogController.h"
#include "RCChannelMonitorController.h"
#include "SyslinkComponentController.h"
#include "AutoPilotPlugin.h"
#include "VehicleComponent.h"
#include "FirmwarePluginManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "JoystickConfigController.h"
#include "JoystickManager.h"
#include "QmlObjectListModel.h"
#include "QGCGeoBoundingCube.h"
#include "MissionManager.h"
#include "QGroundControlQmlGlobal.h"
#include "FlightMapSettings.h"
#include "CoordinateVector.h"
#include "PlanMasterController.h"
#include "VideoManager.h"
#include "VideoReceiver.h"
#include "LogDownloadController.h"
#if defined(QGC_ENABLE_MAVLINK_INSPECTOR)
#include "MAVLinkInspectorController.h"
#endif
#include "ValuesWidgetController.h"
#include "AppMessages.h"
#include "SimulatedPosition.h"
#include "PositionManager.h"
#include "FollowMe.h"
#include "MissionCommandTree.h"
#include "QGCMapPolygon.h"
#include "QGCMapCircle.h"
#include "ParameterManager.h"
#include "SettingsManager.h"
#include "QGCCorePlugin.h"
#include "QGCCameraManager.h"
#include "CameraCalc.h"
#include "VisualMissionItem.h"
#include "EditPositionDialogController.h"
#include "FactValueSliderListModel.h"
#include "ShapeFileHelper.h"
#include "QGCFileDownload.h"
#include "FirmwareImage.h"
#include "MavlinkConsoleController.h"
#include "GeoTagController.h"
#include "LogReplayLink.h"
#include "VehicleObjectAvoidance.h"
#include "TrajectoryPoints.h"

#if defined(QGC_ENABLE_PAIRING)
#include "PairingManager.h"
#endif

#ifndef __mobile__
#include "FirmwareUpgradeController.h"
#endif

#ifndef NO_SERIAL_LINK
#include "SerialLink.h"
#endif

#ifndef __mobile__
#include "GPS/GPSManager.h"
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

class FinishVideoInitialization : public QRunnable
{
public:
  FinishVideoInitialization(VideoManager* manager)
      : _manager(manager)
  {}

  void run () {
      _manager->_initVideo();
  }

private:
  VideoManager* _manager;
};


QGCApplication* QGCApplication::_app = nullptr;

const char* QGCApplication::_deleteAllSettingsKey           = "DeleteAllSettingsNextBoot";
const char* QGCApplication::_settingsVersionKey             = "SettingsVersion";

// Mavlink status structures for entire app
mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];

// Qml Singleton factories

static QObject* screenToolsControllerSingletonFactory(QQmlEngine*, QJSEngine*)
{
    ScreenToolsController* screenToolsController = new ScreenToolsController;
    return screenToolsController;
}

static QObject* qgroundcontrolQmlGlobalSingletonFactory(QQmlEngine*, QJSEngine*)
{
    // We create this object as a QGCTool even though it isn't in the toolbox
    QGroundControlQmlGlobal* qmlGlobal = new QGroundControlQmlGlobal(qgcApp(), qgcApp()->toolbox());
    qmlGlobal->setToolbox(qgcApp()->toolbox());

    return qmlGlobal;
}

static QObject* shapeFileHelperSingletonFactory(QQmlEngine*, QJSEngine*)
{
    return new ShapeFileHelper;
}

QGCApplication::QGCApplication(int &argc, char* argv[], bool unitTesting)
    : QApplication          (argc, argv)
    , _runningUnitTests     (unitTesting)
{
    _app = this;
    _msecsElapsedTime.start();

#ifdef Q_OS_LINUX
#ifndef __mobile__
    if (!_runningUnitTests) {
        if (getuid() == 0) {
            _exitWithError(QString(
                tr("You are running %1 as root. "
                    "You should not do this since it will cause other issues with %1."
                    "%1 will now exit.<br/><br/>"
                    "If you are having serial port issues on Ubuntu, execute the following commands to fix most issues:<br/>"
                    "<pre>sudo usermod -a -G dialout $USER<br/>"
                    "sudo apt-get remove modemmanager</pre>").arg(qgcApp()->applicationName())));
            return;
        }
        // Determine if we have the correct permissions to access USB serial devices
        QFile permFile("/etc/group");
        if(permFile.open(QIODevice::ReadOnly)) {
            while(!permFile.atEnd()) {
                QString line = permFile.readLine();
                if (line.contains("dialout") && !line.contains(getenv("USER"))) {
                    permFile.close();
                    _exitWithError(QString(
                        tr("The current user does not have the correct permissions to access serial devices. "
                           "You should also remove modemmanager since it also interferes.<br/><br/>"
                           "If you are using Ubuntu, execute the following commands to fix these issues:<br/>"
                           "<pre>sudo usermod -a -G dialout $USER<br/>"
                           "sudo apt-get remove modemmanager</pre>")));
                    return;
                }
            }
            permFile.close();
        }
    }
#endif
#endif

    // Setup for network proxy support
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // Parse command line options

    bool fClearSettingsOptions = false; // Clear stored settings
    bool fClearCache = false;           // Clear parameter/airframe caches
    bool logging = false;               // Turn on logging
    QString loggingOptions;

    CmdLineOpt_t rgCmdLineOptions[] = {
        { "--clear-settings",   &fClearSettingsOptions, nullptr },
        { "--clear-cache",      &fClearCache,           nullptr },
        { "--logging",          &logging,               &loggingOptions },
        { "--fake-mobile",      &_fakeMobile,           nullptr },
        { "--log-output",       &_logOutput,            nullptr },
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

    if (fClearCache) {
        QDir dir(ParameterManager::parameterCacheDir());
        dir.removeRecursively();
        QFile airframe(cachedAirframeMetaDataFile());
        airframe.remove();
        QFile parameter(cachedParameterMetaDataFile());
        parameter.remove();
    }

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

    // Gstreamer debug settings
#if defined(__ios__) || defined(__android__)
    // Initialize Video Streaming
    initializeVideoStreaming(argc, argv, nullptr, nullptr);
#else
    QString savePath, gstDebugLevel;
    if (settings.contains(AppSettings::savePathName)) {
        savePath = settings.value(AppSettings::savePathName).toString();
    }
    if(savePath.isEmpty()) {
        savePath = "/tmp";
    }
    savePath = savePath + "/Logs/gst";
    if (!QDir(savePath).exists()) {
        QDir().mkpath(savePath);
    }
    if (settings.contains(AppSettings::gstDebugLevelName)) {
        gstDebugLevel = "*:" + settings.value(AppSettings::gstDebugLevelName).toString();
    }
    // Initialize Video Streaming
    initializeVideoStreaming(argc, argv, savePath.toUtf8().data(), gstDebugLevel.toUtf8().data());
#endif

    _toolbox = new QGCToolbox(this);
    _toolbox->setChildToolboxes();

#ifndef __mobile__
    _gpsRtkFactGroup = new GPSRTKFactGroup(this);
   GPSManager *gpsManager = _toolbox->gpsManager();
   if (gpsManager) {
       connect(gpsManager, &GPSManager::onConnect,          this, &QGCApplication::_onGPSConnect);
       connect(gpsManager, &GPSManager::onDisconnect,       this, &QGCApplication::_onGPSDisconnect);
       connect(gpsManager, &GPSManager::surveyInStatus,     this, &QGCApplication::_gpsSurveyInStatus);
       connect(gpsManager, &GPSManager::satelliteUpdate,    this, &QGCApplication::_gpsNumSatellites);
   }
#endif /* __mobile__ */

   setLanguage();
    _checkForNewVersion();
}

void QGCApplication::_exitWithError(QString errorMessage)
{
    _error = true;
    QQmlApplicationEngine* pEngine = new QQmlApplicationEngine(this);
    pEngine->addImportPath("qrc:/qml");
    pEngine->rootContext()->setContextProperty("errorMessage", errorMessage);
    pEngine->load(QUrl(QStringLiteral("qrc:/qml/ExitWithErrorWindow.qml")));
    // Exit main application when last window is closed
    connect(this, &QGCApplication::lastWindowClosed, this, QGCApplication::quit);
}

void QGCApplication::setLanguage()
{
    _locale = QLocale::system();
    qDebug() << "System reported locale:" << _locale << _locale.name();
    int langID = toolbox()->settingsManager()->appSettings()->language()->rawValue().toInt();
    //-- See App.SettinsGroup.json for index
    if(langID) {
        switch(langID) {
        case 1:
            _locale = QLocale(QLocale::Bulgarian);
            break;
        case 2:
            _locale = QLocale(QLocale::Chinese);
            break;
        case 3:
            _locale = QLocale(QLocale::Dutch);
            break;
        case 4:
            _locale = QLocale(QLocale::English);
            break;
        case 5:
            _locale = QLocale(QLocale::Finnish);
            break;
        case 6:
            _locale = QLocale(QLocale::French);
            break;
        case 7:
            _locale = QLocale(QLocale::German);
            break;
        case 8:
            _locale = QLocale(QLocale::Greek);
            break;
        case 9:
            _locale = QLocale(QLocale::Hebrew);
            break;
        case 10:
            _locale = QLocale(QLocale::Italian);
            break;
        case 11:
            _locale = QLocale(QLocale::Japanese);
            break;
        case 12:
            _locale = QLocale(QLocale::Korean);
            break;
        case 13:
            _locale = QLocale(QLocale::Norwegian);
            break;
        case 14:
            _locale = QLocale(QLocale::Polish);
            break;
        case 15:
            _locale = QLocale(QLocale::Portuguese);
            break;
        case 16:
            _locale = QLocale(QLocale::Russian);
            break;
        case 17:
            _locale = QLocale(QLocale::Spanish);
            break;
        case 18:
            _locale = QLocale(QLocale::Swedish);
            break;
        case 19:
            _locale = QLocale(QLocale::Turkish);
            break;
        }
    }
    //-- We have specific fonts for Korean
    if(_locale == QLocale::Korean) {
        qDebug() << "Loading Korean fonts" << _locale.name();
        if(QFontDatabase::addApplicationFont(":/fonts/NanumGothic-Regular") < 0) {
            qWarning() << "Could not load /fonts/NanumGothic-Regular font";
        }
        if(QFontDatabase::addApplicationFont(":/fonts/NanumGothic-Bold") < 0) {
            qWarning() << "Could not load /fonts/NanumGothic-Bold font";
        }
    }
    qDebug() << "Loading localization for" << _locale.name();
    _app->removeTranslator(&_QGCTranslator);
    _app->removeTranslator(&_QGCTranslatorQt);
    if(_QGCTranslatorQt.load("qt_" + _locale.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        _app->installTranslator(&_QGCTranslatorQt);
    } else {
        qDebug() << "Qt localization for" << _locale.name() << "is not present";
    }
    if(_QGCTranslator.load(_locale, QLatin1String("qgc_"), "", ":/i18n")) {
        QLocale::setDefault(_locale);
        _app->installTranslator(&_QGCTranslator);
    } else {
        qDebug() << "Error loading application localization for" << _locale.name();
    }
    if(_qmlAppEngine)
        _qmlAppEngine->retranslate();
    emit languageChanged(_locale);
}

void QGCApplication::_shutdown()
{
    // Close out all Qml before we delete toolbox. This way we don't get all sorts of null reference complaints from Qml.
    delete _qmlAppEngine;

    delete _toolbox;
}

QGCApplication::~QGCApplication()
{
    // Place shutdown code in _shutdown
    _app = nullptr;
}

void QGCApplication::_initCommon()
{
    static const char* kRefOnly         = "Reference only";
    static const char* kQGroundControl  = "QGroundControl";
    static const char* kQGCControllers  = "QGroundControl.Controllers";
    static const char* kQGCVehicle      = "QGroundControl.Vehicle";

    QSettings settings;

    // Register our Qml objects

    qmlRegisterType<QGCPalette>     ("QGroundControl.Palette", 1, 0, "QGCPalette");
    qmlRegisterType<QGCMapPalette>  ("QGroundControl.Palette", 1, 0, "QGCMapPalette");

    qmlRegisterUncreatableType<Vehicle>                 (kQGCVehicle,                       1, 0, "Vehicle",                    kRefOnly);
    qmlRegisterUncreatableType<MissionManager>          (kQGCVehicle,                       1, 0, "MissionManager",             kRefOnly);
    qmlRegisterUncreatableType<ParameterManager>        (kQGCVehicle,                       1, 0, "ParameterManager",           kRefOnly);
    qmlRegisterUncreatableType<VehicleObjectAvoidance>  (kQGCVehicle,                       1, 0, "VehicleObjectAvoidance",     kRefOnly);
    qmlRegisterUncreatableType<QGCCameraManager>        (kQGCVehicle,                       1, 0, "QGCCameraManager",           kRefOnly);
    qmlRegisterUncreatableType<QGCCameraControl>        (kQGCVehicle,                       1, 0, "QGCCameraControl",           kRefOnly);
    qmlRegisterUncreatableType<QGCVideoStreamInfo>      (kQGCVehicle,                       1, 0, "QGCVideoStreamInfo",         kRefOnly);
    qmlRegisterUncreatableType<LinkInterface>           (kQGCVehicle,                       1, 0, "LinkInterface",              kRefOnly);
    qmlRegisterUncreatableType<MissionController>       (kQGCControllers,                   1, 0, "MissionController",          kRefOnly);
    qmlRegisterUncreatableType<GeoFenceController>      (kQGCControllers,                   1, 0, "GeoFenceController",         kRefOnly);
    qmlRegisterUncreatableType<RallyPointController>    (kQGCControllers,                   1, 0, "RallyPointController",       kRefOnly);

    qmlRegisterUncreatableType<MissionItem>         (kQGroundControl,                       1, 0, "MissionItem",                kRefOnly);
    qmlRegisterUncreatableType<VisualMissionItem>   (kQGroundControl,                       1, 0, "VisualMissionItem",          kRefOnly);
    qmlRegisterUncreatableType<CoordinateVector>    (kQGroundControl,                       1, 0, "CoordinateVector",           kRefOnly);
    qmlRegisterUncreatableType<QmlObjectListModel>  (kQGroundControl,                       1, 0, "QmlObjectListModel",         kRefOnly);
    qmlRegisterUncreatableType<MissionCommandTree>  (kQGroundControl,                       1, 0, "MissionCommandTree",         kRefOnly);
    qmlRegisterUncreatableType<CameraCalc>          (kQGroundControl,                       1, 0, "CameraCalc",                 kRefOnly);
    qmlRegisterUncreatableType<LogReplayLink>       (kQGroundControl,                       1, 0, "LogReplayLink",              kRefOnly);
    qmlRegisterType<LogReplayLinkController>        (kQGroundControl,                       1, 0, "LogReplayLinkController");
#if defined(QGC_ENABLE_MAVLINK_INSPECTOR)
    qmlRegisterUncreatableType<MAVLinkChartController> (kQGroundControl,                    1, 0, "MAVLinkChart",               kRefOnly);
#endif
#if defined(QGC_ENABLE_PAIRING)
    qmlRegisterUncreatableType<PairingManager>      (kQGroundControl,                       1, 0, "PairingManager",             kRefOnly);
#endif

    qmlRegisterUncreatableType<AutoPilotPlugin>     ("QGroundControl.AutoPilotPlugin",      1, 0, "AutoPilotPlugin",            kRefOnly);
    qmlRegisterUncreatableType<VehicleComponent>    ("QGroundControl.AutoPilotPlugin",      1, 0, "VehicleComponent",           kRefOnly);
    qmlRegisterUncreatableType<JoystickManager>     ("QGroundControl.JoystickManager",      1, 0, "JoystickManager",            kRefOnly);
    qmlRegisterUncreatableType<Joystick>            ("QGroundControl.JoystickManager",      1, 0, "Joystick",                   kRefOnly);
    qmlRegisterUncreatableType<QGCPositionManager>  ("QGroundControl.QGCPositionManager",   1, 0, "QGCPositionManager",         kRefOnly);
    qmlRegisterUncreatableType<FactValueSliderListModel>("QGroundControl.FactControls",     1, 0, "FactValueSliderListModel",   kRefOnly);

    qmlRegisterUncreatableType<QGCMapPolygon>       ("QGroundControl.FlightMap",            1, 0, "QGCMapPolygon",              kRefOnly);
    qmlRegisterUncreatableType<QGCGeoBoundingCube>  ("QGroundControl.FlightMap",            1, 0, "QGCGeoBoundingCube",         kRefOnly);
    qmlRegisterUncreatableType<TrajectoryPoints>    ("QGroundControl.FlightMap",            1, 0, "TrajectoryPoints",           kRefOnly);

    qmlRegisterType<QGCMapCircle>                   ("QGroundControl.FlightMap",            1, 0, "QGCMapCircle");

    qmlRegisterType<ParameterEditorController>      (kQGCControllers,                       1, 0, "ParameterEditorController");
    qmlRegisterType<ESP8266ComponentController>     (kQGCControllers,                       1, 0, "ESP8266ComponentController");
    qmlRegisterType<ScreenToolsController>          (kQGCControllers,                       1, 0, "ScreenToolsController");
    qmlRegisterType<PlanMasterController>           (kQGCControllers,                       1, 0, "PlanMasterController");
    qmlRegisterType<ValuesWidgetController>         (kQGCControllers,                       1, 0, "ValuesWidgetController");
    qmlRegisterType<QGCFileDialogController>        (kQGCControllers,                       1, 0, "QGCFileDialogController");
    qmlRegisterType<RCChannelMonitorController>     (kQGCControllers,                       1, 0, "RCChannelMonitorController");
    qmlRegisterType<JoystickConfigController>       (kQGCControllers,                       1, 0, "JoystickConfigController");
    qmlRegisterType<LogDownloadController>          (kQGCControllers,                       1, 0, "LogDownloadController");
    qmlRegisterType<SyslinkComponentController>     (kQGCControllers,                       1, 0, "SyslinkComponentController");
    qmlRegisterType<EditPositionDialogController>   (kQGCControllers,                       1, 0, "EditPositionDialogController");

#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    qmlRegisterType<FirmwareUpgradeController>      (kQGCControllers,                       1, 0, "FirmwareUpgradeController");
#endif
#endif
    qmlRegisterType<GeoTagController>               (kQGCControllers,                       1, 0, "GeoTagController");
    qmlRegisterType<MavlinkConsoleController>       (kQGCControllers,                       1, 0, "MavlinkConsoleController");
#if defined(QGC_ENABLE_MAVLINK_INSPECTOR)
    qmlRegisterType<MAVLinkInspectorController>     (kQGCControllers,                       1, 0, "MAVLinkInspectorController");
#endif
    // Register Qml Singletons
    qmlRegisterSingletonType<QGroundControlQmlGlobal>   ("QGroundControl",                          1, 0, "QGroundControl",         qgroundcontrolQmlGlobalSingletonFactory);
    qmlRegisterSingletonType<ScreenToolsController>     ("QGroundControl.ScreenToolsController",    1, 0, "ScreenToolsController",  screenToolsControllerSingletonFactory);
    qmlRegisterSingletonType<ShapeFileHelper>           ("QGroundControl.ShapeFileHelper",          1, 0, "ShapeFileHelper",        shapeFileHelperSingletonFactory);
}

bool QGCApplication::_initForNormalAppBoot()
{

    if(QFontDatabase::addApplicationFont(":/fonts/opensans") < 0) {
        qWarning() << "Could not load /fonts/opensans font";
    }
    if(QFontDatabase::addApplicationFont(":/fonts/opensans-demibold") < 0) {
        qWarning() << "Could not load /fonts/opensans-demibold font";
    }

    QSettings settings;

    _qmlAppEngine = toolbox()->corePlugin()->createRootWindow(this);

    QQuickWindow* rootWindow = (QQuickWindow*)qgcApp()->mainRootWindow();

    if (rootWindow) {
        rootWindow->scheduleRenderJob (new FinishVideoInitialization (toolbox()->videoManager()),
                QQuickWindow::BeforeSynchronizingStage);
    }

    // Safe to show popup error messages now that main window is created
    UASMessageHandler* msgHandler = qgcApp()->toolbox()->uasMessageHandler();
    if (msgHandler) {
        msgHandler->showErrorsInToolbar();
    }

    // Now that main window is up check for lost log files
    connect(this, &QGCApplication::checkForLostLogFiles, toolbox()->mavlinkProtocol(), &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();

    // Load known link configurations
    toolbox()->linkManager()->loadLinkConfigurationList();

    // Probe for joysticks
    toolbox()->joystickManager()->init();

    if (_settingsUpgraded) {
        showMessage(QString(tr("The format for %1 saved settings has been modified. "
                    "Your saved settings have been reset to defaults.")).arg(applicationName()));
    }

    // Connect links with flag AutoconnectLink
    toolbox()->linkManager()->startAutoConnectedLinks();

    if (getQGCMapEngine()->wasCacheReset()) {
        showMessage(tr("The Offline Map Cache database has been upgraded. "
                    "Your old map cache sets have been reset."));
    }

    settings.sync();
    return true;
}

bool QGCApplication::_initForUnitTests()
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
    return QGCApplication::_app;
}

void QGCApplication::informationMessageBoxOnMainThread(const QString& /*title*/, const QString& msg)
{
    showMessage(msg);
}

void QGCApplication::warningMessageBoxOnMainThread(const QString& /*title*/, const QString& msg)
{
    showMessage(msg);
}

void QGCApplication::criticalMessageBoxOnMainThread(const QString& /*title*/, const QString& msg)
{
    showMessage(msg);
}

void QGCApplication::saveTelemetryLogOnMainThread(QString tempLogfile)
{
    // The vehicle is gone now and we are shutting down so we need to use a message box for errors to hold shutdown and show the error
    if (_checkTelemetrySavePath(true /* useMessageBox */)) {

        QString saveDirPath = _toolbox->settingsManager()->appSettings()->telemetrySavePath();
        QDir saveDir(saveDirPath);

        QString nameFormat("%1%2.%3");
        QString dtFormat("yyyy-MM-dd hh-mm-ss");

        int tryIndex = 1;
        QString saveFileName = nameFormat.arg(
            QDateTime::currentDateTime().toString(dtFormat)).arg(QStringLiteral("")).arg(toolbox()->settingsManager()->appSettings()->telemetryFileExtension);
        while (saveDir.exists(saveFileName)) {
            saveFileName = nameFormat.arg(
                QDateTime::currentDateTime().toString(dtFormat)).arg(QStringLiteral(".%1").arg(tryIndex++)).arg(toolbox()->settingsManager()->appSettings()->telemetryFileExtension);
        }
        QString saveFilePath = saveDir.absoluteFilePath(saveFileName);

        QFile tempFile(tempLogfile);
        if (!tempFile.copy(saveFilePath)) {
            QString error = tr("Unable to save telemetry log. Error copying telemetry to '%1': '%2'.").arg(saveFilePath).arg(tempFile.errorString());
            showMessage(error);
        }
    }
    QFile::remove(tempLogfile);
}

void QGCApplication::checkTelemetrySavePathOnMainThread()
{
    // This is called with an active vehicle so don't pop message boxes which holds ui thread
    _checkTelemetrySavePath(false /* useMessageBox */);
}

bool QGCApplication::_checkTelemetrySavePath(bool /*useMessageBox*/)
{
    QString saveDirPath = _toolbox->settingsManager()->appSettings()->telemetrySavePath();
    if (saveDirPath.isEmpty()) {
        QString error = tr("Unable to save telemetry log. Application save directory is not set.");
        showMessage(error);
        return false;
    }

    QDir saveDir(saveDirPath);
    if (!saveDir.exists()) {
        QString error = tr("Unable to save telemetry log. Telemetry save directory \"%1\" does not exist.").arg(saveDirPath);
        showMessage(error);
        return false;
    }

    return true;
}

void QGCApplication::reportMissingParameter(int componentId, const QString& name)
{
    QPair<int, QString>  missingParam(componentId, name);

    if (!_missingParams.contains(missingParam)) {
        _missingParams.append(missingParam);
    }
    _missingParamsDelayedDisplayTimer.start();
}

/// Called when the delay timer fires to show the missing parameters warning
void QGCApplication::_missingParamsDisplay(void)
{
    if (_missingParams.count()) {
        QString params;
        for (QPair<int, QString>& missingParam: _missingParams) {
            QString param = QStringLiteral("%1:%2").arg(missingParam.first).arg(missingParam.second);
            if (params.isEmpty()) {
                params += param;
            } else {
                params += QStringLiteral(", %1").arg(param);
            }

        }
        _missingParams.clear();

        showMessage(tr("Parameters are missing from firmware. You may be running a version of firmware QGC does not work correctly with or your firmware has a bug in it. Missing params: %1").arg(params));
    }
}

QObject* QGCApplication::_rootQmlObject()
{
    if(_qmlAppEngine && _qmlAppEngine->rootObjects().size())
        return _qmlAppEngine->rootObjects()[0];
    return nullptr;
}


void QGCApplication::showMessage(const QString& message)
{
    // PreArm messages are handled by Vehicle and shown in Map
    if (message.startsWith(QStringLiteral("PreArm")) || message.startsWith(QStringLiteral("preflight"), Qt::CaseInsensitive)) {
        return;
    }
    QObject* rootQmlObject = _rootQmlObject();
    if (rootQmlObject) {
        QVariant varReturn;
        QVariant varMessage = QVariant::fromValue(message);
        QMetaObject::invokeMethod(_rootQmlObject(), "showMessage", Q_RETURN_ARG(QVariant, varReturn), Q_ARG(QVariant, varMessage));
    } else if (runningUnitTests()) {
        // Unit tests can run without UI
        qDebug() << "QGCApplication::showMessage unittest" << message;
    } else {
        qWarning() << "Internal error";
    }
}

QQuickItem* QGCApplication::mainRootWindow()
{
    if(!_mainRootWindow) {
        _mainRootWindow = reinterpret_cast<QQuickItem*>(_rootQmlObject());
    }
    return _mainRootWindow;
}

void QGCApplication::showSetupView()
{
    if(_rootQmlObject()) {
        QMetaObject::invokeMethod(_rootQmlObject(), "showSetupView");
    }
}

void QGCApplication::qmlAttemptWindowClose()
{
    if(_rootQmlObject()) {
        QMetaObject::invokeMethod(_rootQmlObject(), "attemptWindowClose");
    }
}

bool QGCApplication::isInternetAvailable()
{
    if(_toolbox->settingsManager()->appSettings()->checkInternet()->rawValue().toBool())
        return getQGCMapEngine()->isInternetActive();
    return true;
}

void QGCApplication::_checkForNewVersion()
{
#ifndef __mobile__
    if (!_runningUnitTests) {
        if (_parseVersionText(applicationVersion(), _majorVersion, _minorVersion, _buildVersion)) {
            QString versionCheckFile = toolbox()->corePlugin()->stableVersionCheckFileUrl();
            if (!versionCheckFile.isEmpty()) {
                _currentVersionDownload = new QGCFileDownload(this);
                connect(_currentVersionDownload, &QGCFileDownload::downloadFinished, this, &QGCApplication::_currentVersionDownloadFinished);
                connect(_currentVersionDownload, &QGCFileDownload::error, this, &QGCApplication::_currentVersionDownloadError);
                _currentVersionDownload->download(versionCheckFile);
            }
        }
    }
#endif
}

void QGCApplication::_currentVersionDownloadFinished(QString /*remoteFile*/, QString localFile)
{
#ifdef __mobile__
    Q_UNUSED(localFile);
#else
    QFile versionFile(localFile);
    if (versionFile.open(QIODevice::ReadOnly)) {
        QTextStream textStream(&versionFile);
        QString version = textStream.readLine();

        qDebug() << version;

        int majorVersion, minorVersion, buildVersion;
        if (_parseVersionText(version, majorVersion, minorVersion, buildVersion)) {
            if (_majorVersion < majorVersion ||
                    (_majorVersion == majorVersion && _minorVersion < minorVersion) ||
                    (_majorVersion == majorVersion && _minorVersion == minorVersion && _buildVersion < buildVersion)) {
                //-- TODO
                ///QGCMessageBox::information(tr("New Version Available"), tr("There is a newer version of %1 available. You can download it from %2.").arg(applicationName()).arg(toolbox()->corePlugin()->stableDownloadLocation()));
            }
        }
    }

    _currentVersionDownload->deleteLater();
#endif
}

void QGCApplication::_currentVersionDownloadError(QString /*errorMsg*/)
{
    _currentVersionDownload->deleteLater();
}

bool QGCApplication::_parseVersionText(const QString& versionString, int& majorVersion, int& minorVersion, int& buildVersion)
{
    QRegularExpression regExp("v(\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = regExp.match(versionString);
    if (match.hasMatch() && match.lastCapturedIndex() == 3) {
        majorVersion = match.captured(1).toInt();
        minorVersion = match.captured(2).toInt();
        buildVersion = match.captured(3).toInt();
        return true;
    }

    return false;
}


void QGCApplication::_onGPSConnect()
{
    _gpsRtkFactGroup->connected()->setRawValue(true);
}

void QGCApplication::_onGPSDisconnect()
{
    _gpsRtkFactGroup->connected()->setRawValue(false);
}

void QGCApplication::_gpsSurveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active)
{
    _gpsRtkFactGroup->currentDuration()->setRawValue(duration);
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(static_cast<double>(accuracyMM) / 1000.0);
    _gpsRtkFactGroup->currentLatitude()->setRawValue(latitude);
    _gpsRtkFactGroup->currentLongitude()->setRawValue(longitude);
    _gpsRtkFactGroup->currentAltitude()->setRawValue(altitude);
    _gpsRtkFactGroup->valid()->setRawValue(valid);
    _gpsRtkFactGroup->active()->setRawValue(active);
}

void QGCApplication::_gpsNumSatellites(int numSatellites)
{
    _gpsRtkFactGroup->numSatellites()->setRawValue(numSatellites);
}

QString QGCApplication::cachedParameterMetaDataFile(void)
{
    QSettings settings;
    QDir parameterDir = QFileInfo(settings.fileName()).dir();
    return parameterDir.filePath(QStringLiteral("ParameterFactMetaData.xml"));
}

QString QGCApplication::cachedAirframeMetaDataFile(void)
{
    QSettings settings;
    QDir airframeDir = QFileInfo(settings.fileName()).dir();
    return airframeDir.filePath(QStringLiteral("PX4AirframeFactMetaData.xml"));
}
