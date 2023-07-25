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
#include <QRegularExpression>
#include <QFontDatabase>
#include <QQuickWindow>
#include <QQuickImageProvider>
#include <QQuickStyle>

#ifdef QGC_ENABLE_BLUETOOTH
#include <QBluetoothLocalDevice>
#endif

#include <QDebug>

#if defined(QGC_GST_STREAMING)
#include "GStreamer.h"
#endif

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
#include "ParameterEditorController.h"
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
#include "FlightPathSegment.h"
#include "PlanMasterController.h"
#include "VideoManager.h"
#include "VideoReceiver.h"
#include "LogDownloadController.h"
#if !defined(QGC_DISABLE_MAVLINK_INSPECTOR)
#include "MAVLinkInspectorController.h"
#endif
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
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
#include "RCToParamDialogController.h"
#include "QGCImageProvider.h"
#include "TerrainProfile.h"
#include "ToolStripAction.h"
#include "ToolStripActionList.h"
#include "QGCMAVLink.h"
#include "VehicleLinkManager.h"
#include "Autotune.h"

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

static QObject* mavlinkSingletonFactory(QQmlEngine*, QJSEngine*)
{
    return new QGCMAVLink();
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

        // Always set style to default, this way QT_QUICK_CONTROLS_STYLE environment variable doesn't cause random changes in ui
        QQuickStyle::setStyle("Default");
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
    QString applicationName;
    if (_runningUnitTests) {
        // We don't want unit tests to use the same QSettings space as the normal app. So we tweak the app
        // name. Also we want to run unit tests with clean settings every time.
        applicationName = QStringLiteral("%1_unittest").arg(QGC_APPLICATION_NAME);
    } else {
#ifdef DAILY_BUILD
        // This gives daily builds their own separate settings space. Allowing you to use daily and stable builds
        // side by side without daily screwing up your stable settings.
        applicationName = QStringLiteral("%1 Daily").arg(QGC_APPLICATION_NAME);
#else
        applicationName = QGC_APPLICATION_NAME;
#endif
    }
    setApplicationName(applicationName);
    setOrganizationName(QGC_ORG_NAME);
    setOrganizationDomain(QGC_ORG_DOMAIN);

    this->setApplicationVersion(QString(APP_VERSION_STR));

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
    int gstDebugLevel = 0;
    if (settings.contains(AppSettings::gstDebugLevelName)) {
        gstDebugLevel = settings.value(AppSettings::gstDebugLevelName).toInt();
    }

#if defined(QGC_GST_STREAMING)
    // Initialize Video Receiver
    GStreamer::initialize(argc, argv, gstDebugLevel);
#else
    Q_UNUSED(gstDebugLevel)
#endif

    // We need to set language as early as possible prior to loading on JSON files.
    setLanguage();

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
    qDebug() << "System reported locale:" << _locale << "; Name" << _locale.name() << "; Preffered (used in maps): " << (QLocale::system().uiLanguages().length() > 0 ? QLocale::system().uiLanguages()[0] : "None");

    QLocale::Language possibleLocale = AppSettings::_qLocaleLanguageID();
    if (possibleLocale != QLocale::AnyLanguage) {
        _locale = QLocale(possibleLocale);
    }
    //-- We have specific fonts for Korean
    if(_locale == QLocale::Korean) {
        qCDebug(LocalizationLog) << "Loading Korean fonts" << _locale.name();
        if(QFontDatabase::addApplicationFont(":/fonts/NanumGothic-Regular") < 0) {
            qCWarning(LocalizationLog) << "Could not load /fonts/NanumGothic-Regular font";
        }
        if(QFontDatabase::addApplicationFont(":/fonts/NanumGothic-Bold") < 0) {
            qCWarning(LocalizationLog) << "Could not load /fonts/NanumGothic-Bold font";
        }
    }
    qCDebug(LocalizationLog) << "Loading localizations for" << _locale.name();
    _app->removeTranslator(&_qgcTranslatorJSON);
    _app->removeTranslator(&_qgcTranslatorSourceCode);
    _app->removeTranslator(&_qgcTranslatorQtLibs);
    if (_locale.name() != "en_US") {
        QLocale::setDefault(_locale);
        if(_qgcTranslatorQtLibs.load("qt_" + _locale.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
            _app->installTranslator(&_qgcTranslatorQtLibs);
        } else {
            qCWarning(LocalizationLog) << "Qt lib localization for" << _locale.name() << "is not present";
        }
        if(_qgcTranslatorSourceCode.load(_locale, QLatin1String("qgc_source_"), "", ":/i18n")) {
            _app->installTranslator(&_qgcTranslatorSourceCode);
        } else {
            qCWarning(LocalizationLog) << "Error loading source localization for" << _locale.name();
        }
        if(_qgcTranslatorJSON.load(_locale, QLatin1String("qgc_json_"), "", ":/i18n")) {
            _app->installTranslator(&_qgcTranslatorJSON);
        } else {
            qCWarning(LocalizationLog) << "Error loading json localization for" << _locale.name();
        }
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
    delete _gpsRtkFactGroup;
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
    static const char* kQGCTemplates    = "QGroundControl.Templates";

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
    qmlRegisterUncreatableType<VehicleLinkManager>      (kQGCVehicle,                       1, 0, "VehicleLinkManager",         kRefOnly);
    qmlRegisterUncreatableType<Autotune>                (kQGCVehicle,                       1, 0, "Autotune",                   kRefOnly);

    qmlRegisterUncreatableType<MissionController>       (kQGCControllers,                   1, 0, "MissionController",          kRefOnly);
    qmlRegisterUncreatableType<GeoFenceController>      (kQGCControllers,                   1, 0, "GeoFenceController",         kRefOnly);
    qmlRegisterUncreatableType<RallyPointController>    (kQGCControllers,                   1, 0, "RallyPointController",       kRefOnly);

    qmlRegisterUncreatableType<MissionItem>         (kQGroundControl,                       1, 0, "MissionItem",                kRefOnly);
    qmlRegisterUncreatableType<VisualMissionItem>   (kQGroundControl,                       1, 0, "VisualMissionItem",          kRefOnly);
    qmlRegisterUncreatableType<FlightPathSegment>   (kQGroundControl,                       1, 0, "FlightPathSegment",          kRefOnly);
    qmlRegisterUncreatableType<QmlObjectListModel>  (kQGroundControl,                       1, 0, "QmlObjectListModel",         kRefOnly);
    qmlRegisterUncreatableType<MissionCommandTree>  (kQGroundControl,                       1, 0, "MissionCommandTree",         kRefOnly);
    qmlRegisterUncreatableType<CameraCalc>          (kQGroundControl,                       1, 0, "CameraCalc",                 kRefOnly);
    qmlRegisterUncreatableType<LogReplayLink>       (kQGroundControl,                       1, 0, "LogReplayLink",              kRefOnly);
    qmlRegisterUncreatableType<InstrumentValueData> (kQGroundControl,                       1, 0, "InstrumentValueData",        kRefOnly);
    qmlRegisterType<LogReplayLinkController>        (kQGroundControl,                       1, 0, "LogReplayLinkController");
#if !defined(QGC_DISABLE_MAVLINK_INSPECTOR)
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

    qmlRegisterUncreatableType<FactValueGrid>       (kQGCTemplates,                         1, 0, "FactValueGrid",              kRefOnly);
    qmlRegisterType<HorizontalFactValueGrid>        (kQGCTemplates,                         1, 0, "HorizontalFactValueGrid");

    qmlRegisterType<QGCMapCircle>                   ("QGroundControl.FlightMap",            1, 0, "QGCMapCircle");

    qmlRegisterType<ParameterEditorController>      (kQGCControllers,                       1, 0, "ParameterEditorController");
    qmlRegisterType<ESP8266ComponentController>     (kQGCControllers,                       1, 0, "ESP8266ComponentController");
    qmlRegisterType<ScreenToolsController>          (kQGCControllers,                       1, 0, "ScreenToolsController");
    qmlRegisterType<PlanMasterController>           (kQGCControllers,                       1, 0, "PlanMasterController");
    qmlRegisterType<QGCFileDialogController>        (kQGCControllers,                       1, 0, "QGCFileDialogController");
    qmlRegisterType<RCChannelMonitorController>     (kQGCControllers,                       1, 0, "RCChannelMonitorController");
    qmlRegisterType<JoystickConfigController>       (kQGCControllers,                       1, 0, "JoystickConfigController");
    qmlRegisterType<LogDownloadController>          (kQGCControllers,                       1, 0, "LogDownloadController");
    qmlRegisterType<SyslinkComponentController>     (kQGCControllers,                       1, 0, "SyslinkComponentController");
    qmlRegisterType<EditPositionDialogController>   (kQGCControllers,                       1, 0, "EditPositionDialogController");
    qmlRegisterType<RCToParamDialogController>      (kQGCControllers,                       1, 0, "RCToParamDialogController");

    qmlRegisterType<TerrainProfile>                 ("QGroundControl.Controls",             1, 0, "TerrainProfile");
    qmlRegisterType<ToolStripAction>                ("QGroundControl.Controls",             1, 0, "ToolStripAction");
    qmlRegisterType<ToolStripActionList>            ("QGroundControl.Controls",             1, 0, "ToolStripActionList");

#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    qmlRegisterType<FirmwareUpgradeController>      (kQGCControllers,                       1, 0, "FirmwareUpgradeController");
#endif
#endif
    qmlRegisterType<GeoTagController>               (kQGCControllers,                       1, 0, "GeoTagController");
    qmlRegisterType<MavlinkConsoleController>       (kQGCControllers,                       1, 0, "MavlinkConsoleController");
#if !defined(QGC_DISABLE_MAVLINK_INSPECTOR)
    qmlRegisterType<MAVLinkInspectorController>     (kQGCControllers,                       1, 0, "MAVLinkInspectorController");
#endif

    // Register Qml Singletons
    qmlRegisterSingletonType<QGroundControlQmlGlobal>   ("QGroundControl",                          1, 0, "QGroundControl",         qgroundcontrolQmlGlobalSingletonFactory);
    qmlRegisterSingletonType<ScreenToolsController>     ("QGroundControl.ScreenToolsController",    1, 0, "ScreenToolsController",  screenToolsControllerSingletonFactory);
    qmlRegisterSingletonType<ShapeFileHelper>           ("QGroundControl.ShapeFileHelper",          1, 0, "ShapeFileHelper",        shapeFileHelperSingletonFactory);
    qmlRegisterSingletonType<ShapeFileHelper>           ("MAVLink",                                 1, 0, "MAVLink",                mavlinkSingletonFactory);

    // Although this should really be in _initForNormalAppBoot putting it here allowws us to create unit tests which pop up more easily
    if(QFontDatabase::addApplicationFont(":/fonts/opensans") < 0) {
        qWarning() << "Could not load /fonts/opensans font";
    }
    if(QFontDatabase::addApplicationFont(":/fonts/opensans-demibold") < 0) {
        qWarning() << "Could not load /fonts/opensans-demibold font";
    }
}

bool QGCApplication::_initForNormalAppBoot()
{
    QSettings settings;

    _qmlAppEngine = toolbox()->corePlugin()->createQmlApplicationEngine(this);
    toolbox()->corePlugin()->createRootWindow(_qmlAppEngine);

    // Image provider for PX4 Flow
    QQuickImageProvider* pImgProvider = dynamic_cast<QQuickImageProvider*>(qgcApp()->toolbox()->imageProvider());
    _qmlAppEngine->addImageProvider(QStringLiteral("QGCImages"), pImgProvider);

    QQuickWindow* rootWindow = qgcApp()->mainRootWindow();

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
        showAppMessage(QString(tr("The format for %1 saved settings has been modified. "
                    "Your saved settings have been reset to defaults.")).arg(applicationName()));
    }

    // Connect links with flag AutoconnectLink
    toolbox()->linkManager()->startAutoConnectedLinks();

    if (getQGCMapEngine()->wasCacheReset()) {
        showAppMessage(tr("The Offline Map Cache database has been upgraded. "
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
    showAppMessage(msg);
}

void QGCApplication::warningMessageBoxOnMainThread(const QString& /*title*/, const QString& msg)
{
    showAppMessage(msg);
}

void QGCApplication::criticalMessageBoxOnMainThread(const QString& /*title*/, const QString& msg)
{
    showAppMessage(msg);
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
            showAppMessage(error);
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
        showAppMessage(error);
        return false;
    }

    QDir saveDir(saveDirPath);
    if (!saveDir.exists()) {
        QString error = tr("Unable to save telemetry log. Telemetry save directory \"%1\" does not exist.").arg(saveDirPath);
        showAppMessage(error);
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

        showAppMessage(tr("Parameters are missing from firmware. You may be running a version of firmware which is not fully supported or your firmware has a bug in it. Missing params: %1").arg(params));
    }
}

QObject* QGCApplication::_rootQmlObject()
{
    if (_qmlAppEngine && _qmlAppEngine->rootObjects().size())
        return _qmlAppEngine->rootObjects()[0];
    return nullptr;
}

void QGCApplication::showCriticalVehicleMessage(const QString& message)
{
    // PreArm messages are handled by Vehicle and shown in Map
    if (message.startsWith(QStringLiteral("PreArm")) || message.startsWith(QStringLiteral("preflight"), Qt::CaseInsensitive)) {
        return;
    }
    QObject* rootQmlObject = _rootQmlObject();
    if (rootQmlObject) {
        QVariant varReturn;
        QVariant varMessage = QVariant::fromValue(message);
        QMetaObject::invokeMethod(_rootQmlObject(), "showCriticalVehicleMessage", Q_RETURN_ARG(QVariant, varReturn), Q_ARG(QVariant, varMessage));
    } else if (runningUnitTests()) {
        // Unit tests can run without UI
        qDebug() << "QGCApplication::showCriticalVehicleMessage unittest" << message;
    } else {
        qWarning() << "Internal error";
    }
}

void QGCApplication::showAppMessage(const QString& message, const QString& title)
{
    QString dialogTitle = title.isEmpty() ? applicationName() : title;

    QObject* rootQmlObject = _rootQmlObject();
    if (rootQmlObject) {
        QVariant varReturn;
        QVariant varMessage = QVariant::fromValue(message);
        QMetaObject::invokeMethod(_rootQmlObject(), "showMessageDialog", Q_RETURN_ARG(QVariant, varReturn), Q_ARG(QVariant, dialogTitle), Q_ARG(QVariant, varMessage));
    } else if (runningUnitTests()) {
        // Unit tests can run without UI
        qDebug() << "QGCApplication::showAppMessage unittest title:message" << dialogTitle << message;
    } else {
        // UI isn't ready yet
        _delayedAppMessages.append(QPair<QString, QString>(dialogTitle, message));
        QTimer::singleShot(200, this, &QGCApplication::_showDelayedAppMessages);
    }
}

void QGCApplication::showRebootAppMessage(const QString& message, const QString& title)
{
    static QTime lastRebootMessage;

    QTime currentTime = QTime::currentTime();
    QTime previousTime = lastRebootMessage;
    lastRebootMessage = currentTime;

    if (previousTime.isValid() && previousTime.msecsTo(currentTime) < 60 * 1000 * 2) {
        // Debounce reboot messages
        return;
    }

    showAppMessage(message, title);
}

void QGCApplication::_showDelayedAppMessages(void)
{
    if (_rootQmlObject()) {
        for (const QPair<QString, QString>& appMsg: _delayedAppMessages) {
            showAppMessage(appMsg.second, appMsg.first);
        }
        _delayedAppMessages.clear();
    } else {
        QTimer::singleShot(200, this, &QGCApplication::_showDelayedAppMessages);
    }
}

QQuickWindow* QGCApplication::mainRootWindow()
{
    if(!_mainRootWindow) {
        _mainRootWindow = qobject_cast<QQuickWindow*>(_rootQmlObject());
    }
    return _mainRootWindow;
}

void QGCApplication::showSetupView()
{
    if(_rootQmlObject()) {
        QMetaObject::invokeMethod(_rootQmlObject(), "showSetupTool");
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
                QGCFileDownload* download = new QGCFileDownload(this);
                connect(download, &QGCFileDownload::downloadComplete, this, &QGCApplication::_qgcCurrentStableVersionDownloadComplete);
                download->download(versionCheckFile);
            }
        }
    }
#endif
}

void QGCApplication::_qgcCurrentStableVersionDownloadComplete(QString /*remoteFile*/, QString localFile, QString errorMsg)
{
#ifdef __mobile__
    Q_UNUSED(localFile)
    Q_UNUSED(errorMsg)
#else
    if (errorMsg.isEmpty()) {
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
                    showAppMessage(tr("There is a newer version of %1 available. You can download it from %2.").arg(applicationName()).arg(toolbox()->corePlugin()->stableDownloadLocation()), tr("New Version Available"));
                }
            }
        }
    } else {
        qDebug() << "Download QGC stable version failed" << errorMsg;
    }

    sender()->deleteLater();
#endif
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

/// Returns a signal index that is can be compared to QMetaCallEvent.signalId
int QGCApplication::CompressedSignalList::_signalIndex(const QMetaMethod & method)
{
    if (method.methodType() != QMetaMethod::Signal) {
        qWarning() << "Internal error: QGCApplication::CompressedSignalList::_signalIndex not a signal" << method.methodType();
        return -1;
    }

    int index = -1;
    const QMetaObject* metaObject = method.enclosingMetaObject();
    for (int i=0; i<=method.methodIndex(); i++) {
        if (metaObject->method(i).methodType() != QMetaMethod::Signal) {
            continue;
        }
        index++;
    }
    return index;
}

void QGCApplication::CompressedSignalList::add(const QMetaMethod & method)
{
    const QMetaObject*  metaObject  = method.enclosingMetaObject();
    int                 signalIndex = _signalIndex(method);

    if (signalIndex != -1 && !contains(metaObject, signalIndex)) {
        _signalMap[method.enclosingMetaObject()].insert(signalIndex);
    }
}

void QGCApplication::CompressedSignalList::remove(const QMetaMethod & method)
{
    int                 signalIndex = _signalIndex(method);
    const QMetaObject*  metaObject  = method.enclosingMetaObject();

    if (signalIndex != -1 && _signalMap.contains(metaObject) && _signalMap[metaObject].contains(signalIndex)) {
        _signalMap[metaObject].remove(signalIndex);
        if (_signalMap[metaObject].count() == 0) {
            _signalMap.remove(metaObject);
        }
    }
}

bool QGCApplication::CompressedSignalList::contains(const QMetaObject* metaObject, int signalIndex)
{
    return _signalMap.contains(metaObject) && _signalMap[metaObject].contains(signalIndex);
}

void QGCApplication::addCompressedSignal(const QMetaMethod & method)
{
    _compressedSignals.add(method);
}

void QGCApplication::removeCompressedSignal(const QMetaMethod & method)
{
    _compressedSignals.remove(method);
}

bool QGCApplication::compressEvent(QEvent*event, QObject* receiver, QPostEventList* postedEvents)
{
    if (event->type() != QEvent::MetaCall) {
        return QApplication::compressEvent(event, receiver, postedEvents);
    }

    QMetaCallEvent* mce = static_cast<QMetaCallEvent*>(event);
    if (!mce->sender() || !_compressedSignals.contains(mce->sender()->metaObject(), mce->signalId())) {
        return QApplication::compressEvent(event, receiver, postedEvents);
    }

    for (QPostEventList::iterator it = postedEvents->begin(); it != postedEvents->end(); ++it) {
        QPostEvent &cur = *it;
        if (cur.receiver != receiver || cur.event == 0 || cur.event->type() != event->type()) {
            continue;
        }
        QMetaCallEvent *cur_mce = static_cast<QMetaCallEvent*>(cur.event);
        if (cur_mce->sender() != mce->sender() || cur_mce->signalId() != mce->signalId() || cur_mce->id() != mce->id()) {
            continue;
        }
        /* Keep The Newest Call */
        // We can't merely qSwap the existing posted event with the new one, since QEvent
        // keeps track of whether it has been posted. Deletion of a formerly posted event
        // takes the posted event list mutex and does a useless search of the posted event
        // list upon deletion. We thus clear the QEvent::posted flag before deletion.
        struct EventHelper : private QEvent {
            static void clearPostedFlag(QEvent * ev) {
                (&static_cast<EventHelper*>(ev)->t)[1] &= ~0x8001; // Hack to clear QEvent::posted
            }
        };
        EventHelper::clearPostedFlag(cur.event);
        delete cur.event;
        cur.event = event;
        return true;
    }

    return false;
}

bool QGCApplication::event(QEvent *e)
{
    if (e->type() == QEvent::Quit) {
        // On OSX if the user selects Quit from the menu (or Command-Q) the ApplicationWindow does not signal closing. Instead you get a Quit even here only.
        // This in turn causes the standard QGC shutdown sequence to not run. So in this case we close the window ourselves such that the
        // signal is sent and the normal shutdown sequence runs.
        bool forceClose = _mainRootWindow->property("_forceClose").toBool();
        qDebug() << "Quit event" << forceClose;
        // forceClose
        //  true:   Standard QGC shutdown sequence is complete. Let the app quit normally by falling through to the base class processing.
        //  false:  QGC shutdown sequence has not been run yet. Don't let this event close the app yet. Close the main window to kick off the normal shutdown.
        if (!forceClose) {
            //
            _mainRootWindow->close();
            e->ignore();
            return true;
        }
    }
    return QApplication::event(e);
}
