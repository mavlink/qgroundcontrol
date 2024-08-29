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

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtGui/QFontDatabase>
#include <QtGui/QIcon>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickImageProvider>
#include <QtQuickControls2/QQuickStyle>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlApplicationEngine>

#include "QGCConfig.h"
#include "QGCApplication.h"
#include "CmdLineOptParser.h"
#include "UDPLink.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
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
#include "FlightPathSegment.h"
#include "PlanMasterController.h"
#include "VideoManager.h"
#include "LogDownloadController.h"
#if !defined(QGC_DISABLE_MAVLINK_INSPECTOR)
#include "MAVLinkInspectorController.h"
#endif
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "AppMessages.h"
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
#include "FactGroup.h"
#include "FactPanelController.h"
#include "FactValueSliderListModel.h"
#include "ShapeFileHelper.h"
#include "QGCFileDownload.h"
#include "MAVLinkConsoleController.h"
#include "MAVLinkChartController.h"
#include "GeoTagController.h"
#include "LogReplayLink.h"
#include "VehicleObjectAvoidance.h"
#include "TrajectoryPoints.h"
#include "RCToParamDialogController.h"
#include "QGCImageProvider.h"
#include "TerrainProfile.h"
#include "ToolStripAction.h"
#include "ToolStripActionList.h"
#include "VehicleLinkManager.h"
#include "Autotune.h"
#include "RemoteIDManager.h"
#include "CustomAction.h"
#include "CustomActionManager.h"
#include "AudioOutput.h"
#include "JsonHelper.h"
// #ifdef QGC_VIEWER3D
#include "Viewer3DManager.h"
// #endif
#include "GimbalController.h"
#ifndef NO_SERIAL_LINK
#include "FirmwareUpgradeController.h"
#include "SerialLink.h"
#endif

#ifdef Q_OS_LINUX
#ifndef Q_OS_ANDROID
#include <unistd.h>
#include <sys/types.h>
#endif
#endif

QGC_LOGGING_CATEGORY(QGCApplicationLog, "qgc.qgcapplication")

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
    : QApplication(argc, argv)
    , _runningUnitTests(unitTesting)
{
    _msecsElapsedTime.start();

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
        applicationName = QStringLiteral("%1_unittest").arg(QGC_APP_NAME);
    } else {
#ifdef DAILY_BUILD
        // This gives daily builds their own separate settings space. Allowing you to use daily and stable builds
        // side by side without daily screwing up your stable settings.
        applicationName = QStringLiteral("%1 Daily").arg(QGC_APP_NAME);
#else
        applicationName = QGC_APP_NAME;
#endif
    }
    setApplicationName(applicationName);
    setOrganizationName(QGC_ORG_NAME);
    setOrganizationDomain(QGC_ORG_DOMAIN);
    setApplicationVersion(QString(QGC_APP_VERSION_STR));
    #ifdef Q_OS_LINUX
        setWindowIcon(QIcon(":/res/resources/icons/qgroundcontrol.ico"));
    #endif

    // Set settings format
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;
    qCDebug(QGCApplicationLog) << "Settings location" << settings.fileName() << "Is writable?:" << settings.isWritable();

    if (!settings.isWritable()) {
        qCWarning(QGCApplicationLog) << "Setings location is not writable";
    }

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

    // We need to set language as early as possible prior to loading on JSON files.
    setLanguage();

    _toolbox = new QGCToolbox(this);
    _toolbox->setChildToolboxes();

#ifndef DAILY_BUILD
    _checkForNewVersion();
#endif
}

void QGCApplication::setLanguage()
{
    _locale = QLocale::system();
    qCDebug(QGCApplicationLog) << "System reported locale:" << _locale << "; Name" << _locale.name() << "; Preffered (used in maps): " << (QLocale::system().uiLanguages().length() > 0 ? QLocale::system().uiLanguages()[0] : "None");

    QLocale::Language possibleLocale = AppSettings::_qLocaleLanguageEarlyAccess();
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
    removeTranslator(JsonHelper::translator());
    removeTranslator(&_qgcTranslatorSourceCode);
    removeTranslator(&_qgcTranslatorQtLibs);
    if (_locale.name() != "en_US") {
        QLocale::setDefault(_locale);
        if(_qgcTranslatorQtLibs.load("qt_" + _locale.name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            installTranslator(&_qgcTranslatorQtLibs);
        } else {
            qCWarning(LocalizationLog) << "Qt lib localization for" << _locale.name() << "is not present";
        }
        if(_qgcTranslatorSourceCode.load(_locale, QLatin1String("qgc_source_"), "", ":/i18n")) {
            installTranslator(&_qgcTranslatorSourceCode);
        } else {
            qCWarning(LocalizationLog) << "Error loading source localization for" << _locale.name();
        }
        if(JsonHelper::translator()->load(_locale, QLatin1String("qgc_json_"), "", ":/i18n")) {
            installTranslator(JsonHelper::translator());
        } else {
            qCWarning(LocalizationLog) << "Error loading json localization for" << _locale.name();
        }
    }
    if(_qmlAppEngine) {
        _qmlAppEngine->retranslate();
    }
    emit languageChanged(_locale);
}

QGCApplication::~QGCApplication()
{

}

void QGCApplication::init()
{
    // Register our Qml objects

    qmlRegisterType<Fact>               ("QGroundControl.FactSystem", 1, 0, "Fact");
    qmlRegisterType<FactMetaData>       ("QGroundControl.FactSystem", 1, 0, "FactMetaData");
    qmlRegisterType<FactPanelController>("QGroundControl.FactSystem", 1, 0, "FactPanelController");

    qmlRegisterUncreatableType<FactGroup>               ("QGroundControl.FactSystem",   1, 0, "FactGroup",                  "Reference only");
    qmlRegisterUncreatableType<FactValueSliderListModel>("QGroundControl.FactControls", 1, 0, "FactValueSliderListModel",   "Reference only");
    qmlRegisterUncreatableType<ParameterManager>        ("QGroundControl.Vehicle",      1, 0, "ParameterManager",           "Reference only");

    qmlRegisterUncreatableType<FactValueGrid>        ("QGroundControl.Templates",             1, 0, "FactValueGrid",       "Reference only");
    qmlRegisterUncreatableType<FlightPathSegment>    ("QGroundControl",                       1, 0, "FlightPathSegment",   "Reference only");
    qmlRegisterUncreatableType<InstrumentValueData>  ("QGroundControl",                       1, 0, "InstrumentValueData", "Reference only");
    qmlRegisterUncreatableType<QGCGeoBoundingCube>   ("QGroundControl.FlightMap",             1, 0, "QGCGeoBoundingCube",  "Reference only");
    qmlRegisterUncreatableType<QGCMapPolygon>        ("QGroundControl.FlightMap",             1, 0, "QGCMapPolygon",       "Reference only");
    qmlRegisterUncreatableType<QmlObjectListModel>   ("QGroundControl",                       1, 0, "QmlObjectListModel",  "Reference only");
    qmlRegisterType<CustomAction>                    ("QGroundControl.Controllers",           1, 0, "CustomAction");
    qmlRegisterType<CustomActionManager>             ("QGroundControl.Controllers",           1, 0, "CustomActionManager");
    qmlRegisterType<EditPositionDialogController>    ("QGroundControl.Controllers",           1, 0, "EditPositionDialogController");
    qmlRegisterType<HorizontalFactValueGrid>         ("QGroundControl.Templates",             1, 0, "HorizontalFactValueGrid");
    qmlRegisterType<ParameterEditorController>       ("QGroundControl.Controllers",           1, 0, "ParameterEditorController");
    qmlRegisterType<QGCFileDialogController>         ("QGroundControl.Controllers",           1, 0, "QGCFileDialogController");
    qmlRegisterType<QGCMapCircle>                    ("QGroundControl.FlightMap",             1, 0, "QGCMapCircle");
    qmlRegisterType<QGCMapPalette>                   ("QGroundControl.Palette",               1, 0, "QGCMapPalette");
    qmlRegisterType<QGCPalette>                      ("QGroundControl.Palette",               1, 0, "QGCPalette");
    qmlRegisterType<RCChannelMonitorController>      ("QGroundControl.Controllers",           1, 0, "RCChannelMonitorController");
    qmlRegisterType<RCToParamDialogController>       ("QGroundControl.Controllers",           1, 0, "RCToParamDialogController");
    qmlRegisterType<ScreenToolsController>           ("QGroundControl.Controllers",           1, 0, "ScreenToolsController");
    qmlRegisterType<TerrainProfile>                  ("QGroundControl.Controls",              1, 0, "TerrainProfile");
    qmlRegisterType<ToolStripAction>                 ("QGroundControl.Controls",              1, 0, "ToolStripAction");
    qmlRegisterType<ToolStripActionList>             ("QGroundControl.Controls",              1, 0, "ToolStripActionList");
    qmlRegisterSingletonType<QGroundControlQmlGlobal>("QGroundControl",                       1, 0, "QGroundControl",         qgroundcontrolQmlGlobalSingletonFactory);
    qmlRegisterSingletonType<ScreenToolsController>  ("QGroundControl.ScreenToolsController", 1, 0, "ScreenToolsController",  screenToolsControllerSingletonFactory);


    // #ifdef QGC_VIEWER3D
    Viewer3DManager::registerQmlTypes();
    // #endif

    qmlRegisterUncreatableType<Autotune>              ("QGroundControl.Vehicle",   1, 0, "Autotune",               "Reference only");
    qmlRegisterUncreatableType<RemoteIDManager>       ("QGroundControl.Vehicle",   1, 0, "RemoteIDManager",        "Reference only");
    qmlRegisterUncreatableType<TrajectoryPoints>      ("QGroundControl.FlightMap", 1, 0, "TrajectoryPoints",       "Reference only");
    qmlRegisterUncreatableType<VehicleObjectAvoidance>("QGroundControl.Vehicle",   1, 0, "VehicleObjectAvoidance", "Reference only");


    qmlRegisterUncreatableType<CameraCalc>          ("QGroundControl",              1, 0, "CameraCalc",           "Reference only");
    qmlRegisterUncreatableType<GeoFenceController>  ("QGroundControl.Controllers",  1, 0, "GeoFenceController",   "Reference only");
    qmlRegisterUncreatableType<MissionController>   ("QGroundControl.Controllers",  1, 0, "MissionController",    "Reference only");
    qmlRegisterUncreatableType<MissionItem>         ("QGroundControl",              1, 0, "MissionItem",          "Reference only");
    qmlRegisterUncreatableType<MissionManager>      ("QGroundControl.Vehicle",      1, 0, "MissionManager",       "Reference only");
    qmlRegisterUncreatableType<RallyPointController>("QGroundControl.Controllers",  1, 0, "RallyPointController", "Reference only");
    qmlRegisterUncreatableType<VisualMissionItem>   ("QGroundControl",              1, 0, "VisualMissionItem",    "Reference only");
    qmlRegisterType<PlanMasterController>           ("QGroundControl.Controllers",  1, 0, "PlanMasterController");


    qmlRegisterUncreatableType<MavlinkCameraControl>("QGroundControl.Vehicle", 1, 0, "MavlinkCameraControl", "Reference only");
    qmlRegisterUncreatableType<QGCCameraManager>    ("QGroundControl.Vehicle", 1, 0, "QGCCameraManager",     "Reference only");
    qmlRegisterUncreatableType<QGCVideoStreamInfo>  ("QGroundControl.Vehicle", 1, 0, "QGCVideoStreamInfo",   "Reference only");
    qmlRegisterUncreatableType<GimbalController>    ("QGroundControl.Vehicle", 1, 0, "GimbalController",     "Reference only");

#if !defined(QGC_DISABLE_MAVLINK_INSPECTOR)
    qmlRegisterUncreatableType<MAVLinkChartController>("QGroundControl",             1, 0, "MAVLinkChart", "Reference only");
    qmlRegisterType<MAVLinkInspectorController>       ("QGroundControl.Controllers", 1, 0, "MAVLinkInspectorController");
#endif
    qmlRegisterType<GeoTagController>        ("QGroundControl.Controllers", 1, 0, "GeoTagController");
    qmlRegisterType<LogDownloadController>   ("QGroundControl.Controllers", 1, 0, "LogDownloadController");
    qmlRegisterType<MAVLinkConsoleController>("QGroundControl.Controllers", 1, 0, "MAVLinkConsoleController");


    qmlRegisterUncreatableType<AutoPilotPlugin>("QGroundControl.AutoPilotPlugin", 1, 0, "AutoPilotPlugin", "Reference only");
    qmlRegisterType<ESP8266ComponentController>("QGroundControl.Controllers",     1, 0, "ESP8266ComponentController");
    qmlRegisterType<SyslinkComponentController>("QGroundControl.Controllers",     1, 0, "SyslinkComponentController");


    qmlRegisterUncreatableType<VehicleComponent>("QGroundControl.AutoPilotPlugin", 1, 0, "VehicleComponent", "Reference only");
#ifndef NO_SERIAL_LINK
    qmlRegisterType<FirmwareUpgradeController>("QGroundControl.Controllers",       1, 0, "FirmwareUpgradeController");
#endif
    qmlRegisterType<JoystickConfigController>("QGroundControl.Controllers",        1, 0, "JoystickConfigController");


    qmlRegisterSingletonType<ShapeFileHelper>("QGroundControl.ShapeFileHelper", 1, 0, "ShapeFileHelper", shapeFileHelperSingletonFactory);


    // Although this should really be in _initForNormalAppBoot putting it here allowws us to create unit tests which pop up more easily
    if(QFontDatabase::addApplicationFont(":/fonts/opensans") < 0) {
        qCWarning(QGCApplicationLog) << "Could not load /fonts/opensans font";
    }
    if(QFontDatabase::addApplicationFont(":/fonts/opensans-demibold") < 0) {
        qCWarning(QGCApplicationLog) << "Could not load /fonts/opensans-demibold font";
    }

    if (!_runningUnitTests) {
        _initForNormalAppBoot();
    } else {
        AudioOutput::instance()->setMuted(true);
    }
}

void QGCApplication::_initForNormalAppBoot()
{
#ifdef Q_OS_DARWIN
    // Gstreamer video playback requires OpenGL
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    QQuickStyle::setStyle("Basic");
    _qmlAppEngine = _toolbox->corePlugin()->createQmlApplicationEngine(this);
    QObject::connect(_qmlAppEngine, &QQmlApplicationEngine::objectCreationFailed, this, QCoreApplication::quit, Qt::QueuedConnection);
    _toolbox->corePlugin()->createRootWindow(_qmlAppEngine);

    ( void ) connect( _toolbox->settingsManager()->appSettings()->audioMuted(), &Fact::valueChanged, AudioOutput::instance(), []( QVariant value )
    {
        AudioOutput::instance()->setMuted( value.toBool() );
    });
    AudioOutput::instance()->setMuted( _toolbox->settingsManager()->appSettings()->audioMuted()->rawValue().toBool() );

    // Image provider for PX4 Flow
    _qmlAppEngine->addImageProvider(qgcImageProviderId, new QGCImageProvider());

    QQuickWindow* rootWindow = mainRootWindow();
    if (rootWindow) {
        rootWindow->scheduleRenderJob(new FinishVideoInitialization(_toolbox->videoManager()),
                QQuickWindow::BeforeSynchronizingStage);
    }

    // Safe to show popup error messages now that main window is created
    _showErrorsInToolbar = true;

    #ifdef Q_OS_LINUX
    #ifndef Q_OS_ANDROID
    #ifndef NO_SERIAL_LINK
        if (!_runningUnitTests) {
            // Determine if we have the correct permissions to access USB serial devices
            QFile permFile("/etc/group");
            if(permFile.open(QIODevice::ReadOnly)) {
                while(!permFile.atEnd()) {
                    const QString line = permFile.readLine();
                    if (line.contains("dialout") && !line.contains(getenv("USER"))) {
                        permFile.close();
                        showAppMessage(QString(
                            tr("The current user does not have the correct permissions to access serial devices. "
                               "You should also remove modemmanager since it also interferes.<br/><br/>"
                               "If you are using Ubuntu, execute the following commands to fix these issues:<br/>"
                               "<pre>sudo usermod -a -G dialout $USER<br/>"
                               "sudo apt-get remove modemmanager</pre>")));
                        break;
                    }
                }
                permFile.close();
            }
        }
    #endif
    #endif
    #endif

    // Now that main window is up check for lost log files
    connect(this, &QGCApplication::checkForLostLogFiles, _toolbox->mavlinkProtocol(), &MAVLinkProtocol::checkForLostLogFiles);
    emit checkForLostLogFiles();

    // Load known link configurations
    _toolbox->linkManager()->loadLinkConfigurationList();

    // Probe for joysticks
    _toolbox->joystickManager()->init();

    if (_settingsUpgraded) {
        showAppMessage(QString(tr("The format for %1 saved settings has been modified. "
                    "Your saved settings have been reset to defaults.")).arg(applicationName()));
    }

    // Connect links with flag AutoconnectLink
    _toolbox->linkManager()->startAutoConnectedLinks();
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

void QGCApplication::saveTelemetryLogOnMainThread(const QString &tempLogfile)
{
    // The vehicle is gone now and we are shutting down so we need to use a message box for errors to hold shutdown and show the error
    if (_checkTelemetrySavePath(true /* useMessageBox */)) {

        const QString saveDirPath = _toolbox->settingsManager()->appSettings()->telemetrySavePath();
        const QDir saveDir(saveDirPath);

        const QString nameFormat("%1%2.%3");
        const QString dtFormat("yyyy-MM-dd hh-mm-ss");

        int tryIndex = 1;
        QString saveFileName = nameFormat.arg(
            QDateTime::currentDateTime().toString(dtFormat)).arg(QStringLiteral("")).arg(_toolbox->settingsManager()->appSettings()->telemetryFileExtension);
        while (saveDir.exists(saveFileName)) {
            saveFileName = nameFormat.arg(
                QDateTime::currentDateTime().toString(dtFormat)).arg(QStringLiteral(".%1").arg(tryIndex++)).arg(_toolbox->settingsManager()->appSettings()->telemetryFileExtension);
        }
        const QString saveFilePath = saveDir.absoluteFilePath(saveFileName);

        QFile tempFile(tempLogfile);
        if (!tempFile.copy(saveFilePath)) {
            const QString error = tr("Unable to save telemetry log. Error copying telemetry to '%1': '%2'.").arg(saveFilePath).arg(tempFile.errorString());
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
    const QString saveDirPath = _toolbox->settingsManager()->appSettings()->telemetrySavePath();
    if (saveDirPath.isEmpty()) {
        const QString error = tr("Unable to save telemetry log. Application save directory is not set.");
        showAppMessage(error);
        return false;
    }

    const QDir saveDir(saveDirPath);
    if (!saveDir.exists()) {
        const QString error = tr("Unable to save telemetry log. Telemetry save directory \"%1\" does not exist.").arg(saveDirPath);
        showAppMessage(error);
        return false;
    }

    return true;
}

void QGCApplication::reportMissingParameter(int componentId, const QString& name)
{
    const QPair<int, QString> missingParam(componentId, name);

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
            const QString param = QStringLiteral("%1:%2").arg(missingParam.first).arg(missingParam.second);
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
    if (_qmlAppEngine && _qmlAppEngine->rootObjects().size()) {
        return _qmlAppEngine->rootObjects()[0];
    }
    return nullptr;
}

void QGCApplication::showCriticalVehicleMessage(const QString& message)
{
    // PreArm messages are handled by Vehicle and shown in Map
    if (message.startsWith(QStringLiteral("PreArm")) || message.startsWith(QStringLiteral("preflight"), Qt::CaseInsensitive)) {
        return;
    }
    QObject* rootQmlObject = _rootQmlObject();
    if (rootQmlObject && _showErrorsInToolbar) {
        QVariant varReturn;
        QVariant varMessage = QVariant::fromValue(message);
        QMetaObject::invokeMethod(rootQmlObject, "showCriticalVehicleMessage", Q_RETURN_ARG(QVariant, varReturn), Q_ARG(QVariant, varMessage));
    } else if (runningUnitTests() || !_showErrorsInToolbar) {
        // Unit tests can run without UI
        qCDebug(QGCApplicationLog) << "QGCApplication::showCriticalVehicleMessage unittest" << message;
    } else {
        qCWarning(QGCApplicationLog) << "Internal error";
    }
}

void QGCApplication::showAppMessage(const QString& message, const QString& title)
{
    QString dialogTitle = title.isEmpty() ? applicationName() : title;

    QObject* rootQmlObject = _rootQmlObject();
    if (rootQmlObject) {
        QVariant varReturn;
        QVariant varMessage = QVariant::fromValue(message);
        QMetaObject::invokeMethod(_rootQmlObject(), "_showMessageDialog", Q_RETURN_ARG(QVariant, varReturn), Q_ARG(QVariant, dialogTitle), Q_ARG(QVariant, varMessage));
    } else if (runningUnitTests()) {
        // Unit tests can run without UI
        qCDebug(QGCApplicationLog) << "QGCApplication::showAppMessage unittest title:message" << dialogTitle << message;
    } else {
        // UI isn't ready yet
        _delayedAppMessages.append(QPair<QString, QString>(dialogTitle, message));
        QTimer::singleShot(200, this, &QGCApplication::_showDelayedAppMessages);
    }
}

void QGCApplication::showRebootAppMessage(const QString& message, const QString& title)
{
    static QTime lastRebootMessage;

    const QTime currentTime = QTime::currentTime();
    const QTime previousTime = lastRebootMessage;
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
      QVariant arg = "";
      QMetaObject::invokeMethod(_rootQmlObject(), "showVehicleSetupTool", Q_ARG(QVariant, arg));
    }
}

void QGCApplication::qmlAttemptWindowClose()
{
    if(_rootQmlObject()) {
        QMetaObject::invokeMethod(_rootQmlObject(), "attemptWindowClose");
    }
}

void QGCApplication::_checkForNewVersion()
{
    if (!_runningUnitTests) {
        if (_parseVersionText(applicationVersion(), _majorVersion, _minorVersion, _buildVersion)) {
            const QString versionCheckFile = _toolbox->corePlugin()->stableVersionCheckFileUrl();
            if (!versionCheckFile.isEmpty()) {
                QGCFileDownload* download = new QGCFileDownload(this);
                connect(download, &QGCFileDownload::downloadComplete, this, &QGCApplication::_qgcCurrentStableVersionDownloadComplete);
                download->download(versionCheckFile);
            }
        }
    }
}

void QGCApplication::_qgcCurrentStableVersionDownloadComplete(QString /*remoteFile*/, QString localFile, QString errorMsg)
{
    if (errorMsg.isEmpty()) {
        QFile versionFile(localFile);
        if (versionFile.open(QIODevice::ReadOnly)) {
            QTextStream textStream(&versionFile);
            const QString version = textStream.readLine();

            qCDebug(QGCApplicationLog) << version;

            int majorVersion, minorVersion, buildVersion;
            if (_parseVersionText(version, majorVersion, minorVersion, buildVersion)) {
                if (_majorVersion < majorVersion ||
                        (_majorVersion == majorVersion && _minorVersion < minorVersion) ||
                        (_majorVersion == majorVersion && _minorVersion == minorVersion && _buildVersion < buildVersion)) {
                    showAppMessage(tr("There is a newer version of %1 available. You can download it from %2.").arg(applicationName()).arg(_toolbox->corePlugin()->stableDownloadLocation()), tr("New Version Available"));
                }
            }
        }
    } else {
        qCDebug(QGCApplicationLog) << "Download QGC stable version failed" << errorMsg;
    }

    sender()->deleteLater();
}

bool QGCApplication::_parseVersionText(const QString& versionString, int& majorVersion, int& minorVersion, int& buildVersion)
{
    static const QRegularExpression regExp("v(\\d+)\\.(\\d+)\\.(\\d+)");
    const QRegularExpressionMatch match = regExp.match(versionString);
    if (match.hasMatch() && match.lastCapturedIndex() == 3) {
        majorVersion = match.captured(1).toInt();
        minorVersion = match.captured(2).toInt();
        buildVersion = match.captured(3).toInt();
        return true;
    }

    return false;
}

QString QGCApplication::cachedParameterMetaDataFile(void)
{
    QSettings settings;
    const QDir parameterDir = QFileInfo(settings.fileName()).dir();
    return parameterDir.filePath(QStringLiteral("ParameterFactMetaData.xml"));
}

QString QGCApplication::cachedAirframeMetaDataFile(void)
{
    QSettings settings;
    const QDir airframeDir = QFileInfo(settings.fileName()).dir();
    return airframeDir.filePath(QStringLiteral("PX4AirframeFactMetaData.xml"));
}

/// Returns a signal index that is can be compared to QMetaCallEvent.signalId
int QGCApplication::CompressedSignalList::_signalIndex(const QMetaMethod & method)
{
    if (method.methodType() != QMetaMethod::Signal) {
        qCWarning(QGCApplicationLog) << "Internal error: QGCApplication::CompressedSignalList::_signalIndex not a signal" << method.methodType();
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
    const int signalIndex = _signalIndex(method);
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

    const QMetaCallEvent* mce = static_cast<QMetaCallEvent*>(event);
    if (!mce->sender() || !_compressedSignals.contains(mce->sender()->metaObject(), mce->signalId())) {
        return QApplication::compressEvent(event, receiver, postedEvents);
    }

    for (QPostEventList::iterator it = postedEvents->begin(); it != postedEvents->end(); ++it) {
        QPostEvent &cur = *it;
        if (cur.receiver != receiver || cur.event == 0 || cur.event->type() != event->type()) {
            continue;
        }
        const QMetaCallEvent *cur_mce = static_cast<QMetaCallEvent*>(cur.event);
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
        // On OSX if the user selects Quit from the menu (or Command-Q) the ApplicationWindow does not signal closing. Instead you get a Quit event here only.
        // This in turn causes the standard QGC shutdown sequence to not run. So in this case we close the window ourselves such that the
        // signal is sent and the normal shutdown sequence runs.
        const bool forceClose = _mainRootWindow->property("_forceClose").toBool();
        qCDebug(QGCApplicationLog) << "Quit event" << forceClose;
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

QGCImageProvider* QGCApplication::qgcImageProvider()
{
    return dynamic_cast<QGCImageProvider*>(_qmlAppEngine->imageProvider(qgcImageProviderId));
}

void QGCApplication::shutdown()
{
    qCDebug(QGCApplicationLog) << "Exit";
    // This is bad, but currently qobject inheritances are incorrect and cause crashes on exit without
    delete _qmlAppEngine;
}

QString QGCApplication::numberToString(quint64 number)
{
    return getCurrentLanguage().toString(number);
}

QString QGCApplication::bigSizeToString(quint64 size)
{
    QString result;
    const QLocale kLocale = getCurrentLanguage();
    if (size < 1024) {
        result = kLocale.toString(size);
    } else if (size < pow(1024, 2)) {
        result = kLocale.toString(static_cast<double>(size) / 1024.0, 'f', 1) + "kB";
    } else if (size < pow(1024, 3)) {
        result = kLocale.toString(static_cast<double>(size) / pow(1024, 2), 'f', 1) + "MB";
    } else if (size < pow(1024, 4)) {
        result = kLocale.toString(static_cast<double>(size) / pow(1024, 3), 'f', 1) + "GB";
    } else {
        result = kLocale.toString(static_cast<double>(size) / pow(1024, 4), 'f', 1) + "TB";
    }
    return result;
}

QString QGCApplication::bigSizeMBToString(quint64 size_MB)
{
    QString result;
    const QLocale kLocale = getCurrentLanguage();
    if (size_MB < 1024) {
        result = kLocale.toString(static_cast<double>(size_MB) , 'f', 0) + " MB";
    } else if(size_MB < pow(1024, 2)) {
        result = kLocale.toString(static_cast<double>(size_MB) / 1024.0, 'f', 1) + " GB";
    } else {
        result = kLocale.toString(static_cast<double>(size_MB) / pow(1024, 2), 'f', 2) + " TB";
    }
    return result;
}
