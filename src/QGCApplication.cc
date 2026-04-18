#include "QGCApplication.h"
#include "qgc_version.h"

#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QRegularExpression>
#include <QtGui/QFontDatabase>
#include <QtGui/QIcon>
#include <QtGui/QOpenGLContext>
#include "QGCNetworkHelper.h"
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickImageProvider>
#include <QtQuick/QQuickWindow>
#include <QtQuickControls2/QQuickStyle>
#include <QtSvg/QSvgRenderer>

#include <QtCore/private/qthread_p.h>

#include "LogManager.h"
#include "LogRemoteSink.h"
#include "AudioOutput.h"
#include "FollowMe.h"
#include "JoystickManager.h"
#include "JsonHelper.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "PositionManager.h"
#include "QGCCommandLineParser.h"
#include "QGCCorePlugin.h"
#include "QGCFileDownload.h"
#include "QGCImageProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCLoggingCategoryManager.h"
#include "SettingsManager.h"
#include "MavlinkSettings.h"
#include "AppSettings.h"
#include "UDPLink.h"
#include "Vehicle.h"
#include "VehicleComponent.h"
#include "VideoManager.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialLink.h"
#endif

QGC_LOGGING_CATEGORY(QGCApplicationLog, "API.QGCApplication")

QGCApplication::QGCApplication(int &argc, char *argv[], const QGCCommandLineParser::CommandLineParseResult &cli)
    : QApplication(argc, argv)
    , _runningUnitTests(cli.runningUnitTests)
    , _simpleBootTest(cli.simpleBootTest)
    , _fakeMobile(cli.fakeMobile)
    , _logOutput(cli.logOutput)
    , _systemId(cli.systemId.value_or(0))
{
    _msecsElapsedTime.start();

    // Setup for network proxy support
    QGCNetworkHelper::initializeProxySupport();

    bool fClearSettingsOptions = cli.clearSettingsOptions;  // Clear stored settings
    const bool fClearCache = cli.clearCache;                // Clear parameter/airframe caches
    const QString loggingOptions = cli.loggingOptions.value_or(QString(""));

    // Set up timer for delayed missing fact display
    _missingParamsDelayedDisplayTimer.setSingleShot(true);
    _missingParamsDelayedDisplayTimer.setInterval(_missingParamsDelayedDisplayTimerTimeout);
    (void) connect(&_missingParamsDelayedDisplayTimer, &QTimer::timeout, this, &QGCApplication::_missingParamsDisplay);

    // Set application information
    QString applicationName;
    if (_runningUnitTests || _simpleBootTest) {
        // We don't want unit tests to use the same QSettings space as the normal app. So we tweak the app
        // name. Also we want to run unit tests with clean settings every time.
        // Include test name or PID to prevent settings file conflicts when tests run in parallel
        if (!cli.unitTests.isEmpty()) {
            applicationName = QStringLiteral("%1_unittest_%2").arg(QGC_APP_NAME, cli.unitTests.first());
        } else {
            applicationName = QStringLiteral("%1_unittest_%2").arg(QGC_APP_NAME).arg(QCoreApplication::applicationPid());
        }
    } else {
#ifdef QGC_DAILY_BUILD
        // This gives daily builds their own separate settings space. Allowing you to use daily and stable builds
        // side by side without daily screwing up your stable settings.
        applicationName = QStringLiteral("%1 Daily").arg(QGC_APP_NAME);
#else
        applicationName = QGC_APP_NAME;
#endif
    }
    setApplicationName(applicationName);
    setDesktopFileName(QGC_PACKAGE_NAME);
    setOrganizationName(QGC_ORG_NAME);
    setOrganizationDomain(QGC_ORG_DOMAIN);
    setApplicationVersion(QString(QGC_APP_VERSION_STR));

    // Set settings format
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;
    qCDebug(QGCApplicationLog) << "Settings location" << settings.fileName() << "Is writable?:" << settings.isWritable();

    if (!settings.isWritable()) {
        qCWarning(QGCApplicationLog) << "Setings location is not writable";
    }

    // The setting will delete all settings on this boot
    fClearSettingsOptions |= settings.value(AppSettings::clearSettingsNextBootKey, false).toBool();

    if (_runningUnitTests || _simpleBootTest) {
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
        QFile parameter(cachedParameterMetaDataFile());
        parameter.remove();
        QFile airframe(cachedAirframeMetaDataFile());
        airframe.remove();

        // Clear versioned parameter metadata cache
        const QString metaDataCachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                          + QStringLiteral("/ParameterMetaData");
        QDir(metaDataCachePath).removeRecursively();
    }

    // Set up our logging filters
    QGCLoggingCategoryManager::init();
    QGCLoggingCategoryManager::instance()->installFilter(loggingOptions);

    // We need to set language as early as possible prior to loading on JSON files.
    setLanguage();

    // Force old SVG Tiny 1.2 behavior for compatibility
    QSvgRenderer::setDefaultOptions(QtSvg::Tiny12FeaturesOnly);

#ifndef QGC_DAILY_BUILD
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
    if (_locale == QLocale::Korean) {
        qCDebug(QGCApplicationLog) << "Loading Korean fonts" << _locale.name();
        if(QFontDatabase::addApplicationFont(":/fonts/NanumGothic-Regular") < 0) {
            qCWarning(QGCApplicationLog) << "Could not load /fonts/NanumGothic-Regular font";
        }
        if(QFontDatabase::addApplicationFont(":/fonts/NanumGothic-Bold") < 0) {
            qCWarning(QGCApplicationLog) << "Could not load /fonts/NanumGothic-Bold font";
        }
    }
    qCDebug(QGCApplicationLog) << "Loading localizations for" << _locale.name();
    removeTranslator(JsonHelper::translator());
    removeTranslator(&_qgcTranslatorSourceCode);
    removeTranslator(&_qgcTranslatorQtLibs);
    if (_locale.name() != "en_US") {
        QLocale::setDefault(_locale);
        if (_qgcTranslatorQtLibs.load("qt_" + _locale.name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            installTranslator(&_qgcTranslatorQtLibs);
        } else {
            qCWarning(QGCApplicationLog) << "Qt lib localization for" << _locale.name() << "is not present";
        }
        if (_qgcTranslatorSourceCode.load(_locale, QLatin1String("qgc_source_"), "", ":/i18n")) {
            installTranslator(&_qgcTranslatorSourceCode);
        } else {
            qCWarning(QGCApplicationLog) << "Error loading source localization for" << _locale.name();
        }
        if (JsonHelper::translator()->load(_locale, QLatin1String("qgc_json_"), "", ":/i18n")) {
            installTranslator(JsonHelper::translator());
        } else {
            qCWarning(QGCApplicationLog) << "Error loading json localization for" << _locale.name();
        }
    }

    if (_qmlAppEngine) {
        _qmlAppEngine->retranslate();
    }

    emit languageChanged(_locale);
}

QGCApplication::~QGCApplication()
{

}

void QGCApplication::init()
{
    SettingsManager::instance()->init();
    if (_systemId > 0) {
        qCDebug(QGCApplicationLog) << "Setting MAVLink System ID to:" << _systemId;
        SettingsManager::instance()->mavlinkSettings()->gcsMavlinkSystemID()->setRawValue(_systemId);
    }

    // Set up log directory for disk logging and SQLite store, and re-apply on path change
    {
        auto *logMgr = LogManager::instance();
        auto *appSettings = SettingsManager::instance()->appSettings();
        logMgr->setLogDirectory(appSettings->logSavePath());
        QObject::connect(appSettings, &AppSettings::savePathsChanged, logMgr, [logMgr, appSettings]() {
            logMgr->setLogDirectory(appSettings->logSavePath());
        });
    }

    // Wire remote logging settings to the sink
    {
        auto *appSettings = SettingsManager::instance()->appSettings();
        auto *sink = LogManager::instance()->remoteSink();
        auto applySetting = [appSettings, sink]() {
            sink->setHost(appSettings->remoteLoggingHost()->rawValue().toString());
            sink->setPort(static_cast<quint16>(appSettings->remoteLoggingPort()->rawValue().toUInt()));
            sink->setProtocol(static_cast<TransportStrategy::Protocol>(
                appSettings->remoteLoggingProtocol()->rawValue().toInt()));
            sink->setVehicleId(appSettings->remoteLoggingVehicleId()->rawValue().toString());
            sink->setTlsEnabled(appSettings->remoteLoggingTlsEnabled()->rawValue().toBool());
            sink->setTlsVerifyPeer(appSettings->remoteLoggingTlsVerifyPeer()->rawValue().toBool());
            sink->setCompressionEnabled(appSettings->remoteLoggingCompressionEnabled()->rawValue().toBool());
            sink->setCompressionLevel(appSettings->remoteLoggingCompressionLevel()->rawValue().toInt());
            sink->setEnabled(appSettings->remoteLoggingEnabled()->rawValue().toBool());
        };
        for (auto *fact : {
                 appSettings->remoteLoggingEnabled(),    appSettings->remoteLoggingHost(),
                 appSettings->remoteLoggingPort(),       appSettings->remoteLoggingProtocol(),
                 appSettings->remoteLoggingVehicleId(),  appSettings->remoteLoggingTlsEnabled(),
                 appSettings->remoteLoggingTlsVerifyPeer(),
                 appSettings->remoteLoggingCompressionEnabled(),
                 appSettings->remoteLoggingCompressionLevel(),
             }) {
            QObject::connect(fact, &Fact::rawValueChanged, sink, applySetting);
        }
        applySetting();
    }

    // Although this should really be in _initForNormalAppBoot putting it here allowws us to create unit tests which pop up more easily
    if (QFontDatabase::addApplicationFont(":/fonts/opensans") < 0) {
        qCWarning(QGCApplicationLog) << "Could not load /fonts/opensans font";
    }

    if (QFontDatabase::addApplicationFont(":/fonts/opensans-demibold") < 0) {
        qCWarning(QGCApplicationLog) << "Could not load /fonts/opensans-demibold font";
    }

    if (_simpleBootTest) {
        // Since GStream builds are so problematic we initialize video during the simple boot test
        // to make sure it works and verfies plugin availability.
        _bootTestPassed = _initVideo();
    } else if (!_runningUnitTests) {
        _initForNormalAppBoot();
    }
}

bool QGCApplication::_initVideo()
{
#ifdef QGC_GST_STREAMING
    // GStreamer video rendering backend selection:
    //  - Windows D3D11: native RHI, no OpenGL needed.
    //  - macOS: appsink → QVideoSink → Metal RHI VideoOutput, no OpenGL needed.
    //  - Linux/other: qml6glsink requires OpenGL. Probe for a working GL context
    //    and fall back to the default graphics API if unavailable.
    //
    // The offscreen platform (used in CI boot tests) never provides a real
    // GL context, so skip the probe there — just set OpenGL API to exercise
    // the full GStreamer init path.
    const bool isOffscreen = (qApp->platformName() == QLatin1String("offscreen"));

#if defined(QGC_GST_D3D11_SINK)
    // D3D11 sink renders via Qt's native D3D11 RHI — no OpenGL needed.
    if (isOffscreen) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    }
    qCDebug(QGCApplicationLog) << "D3D11 video sink available, using default graphics API";
#elif defined(Q_OS_MACOS)
    // macOS Metal rendering path: appsink → QVideoSink → VideoOutput.
    // Do NOT force OpenGL — let Qt use the default Metal RHI backend.
    // The appsink path in qgcvideosinkbin avoids the GL-dependent qml6glsink.
    if (isOffscreen) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    } else {
        qCDebug(QGCApplicationLog) << "macOS: using default RHI backend (Metal) for appsink video path";
    }
#else
    const bool skipGLProbe = isOffscreen;

    if (skipGLProbe) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    } else {
        QOpenGLContext testCtx;
        if (testCtx.create()) {
            QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
        } else {
            qCWarning(QGCApplicationLog) << "OpenGL not available; GStreamer video will be disabled."
                                         << "Using default graphics API (Metal/Vulkan).";
        }
    }
#endif  // QGC_GST_D3D11_SINK / Q_OS_MACOS
#endif

    QGCCorePlugin::instance();  // CorePlugin must be initialized before VideoManager for Video Cleanup
    VideoManager *videoManager = VideoManager::instance();
    videoManager->startGStreamerInit();
    const bool initSucceeded = !_simpleBootTest || videoManager->waitForGStreamerInit();
    _videoManagerInitialized = true;
    return initSucceeded;
}

void QGCApplication::_initForNormalAppBoot()
{
    (void) _initVideo();

    QQuickStyle::setStyle("Basic");
    QGCCorePlugin::instance()->init();
    MAVLinkProtocol::instance()->init();
    MultiVehicleManager::instance()->init();
    _qmlAppEngine = QGCCorePlugin::instance()->createQmlApplicationEngine(this);
    QObject::connect(_qmlAppEngine, &QQmlApplicationEngine::objectCreationFailed, this, QCoreApplication::quit, Qt::QueuedConnection);
    QGCCorePlugin::instance()->createRootWindow(_qmlAppEngine);

    AudioOutput::instance()->init(SettingsManager::instance()->appSettings()->audioVolume(), SettingsManager::instance()->appSettings()->audioMuted());
    FollowMe::instance()->init();
    QGCPositionManager::instance()->init();
    LinkManager::instance()->init();
    VideoManager::instance()->init(mainRootWindow());

    // Image provider for Optical Flow
    _qmlAppEngine->addImageProvider(_qgcImageProviderId, new QGCImageProvider());

    // Set the window icon now that custom plugin has a chance to override it
#ifdef Q_OS_LINUX
    QUrl windowIcon = QUrl("qrc:/res/qgroundcontrol.ico");
    windowIcon = _qmlAppEngine->interceptUrl(windowIcon, QQmlAbstractUrlInterceptor::UrlString);
    // The interceptor needs "qrc:/path" but QIcon expects ":/path"
    setWindowIcon(QIcon(":" + windowIcon.path()));
#endif

    // Safe to show popup error messages now that main window is created
    _showErrorsInToolbar = true;

    #ifdef Q_OS_LINUX
    #ifndef Q_OS_ANDROID
    #ifndef QGC_NO_SERIAL_LINK
        if (!_runningUnitTests) {
            // Determine if we have the correct permissions to access USB serial devices
            QFile permFile("/etc/group");
            if(permFile.open(QIODevice::ReadOnly)) {
                while(!permFile.atEnd()) {
                    const QString line = permFile.readLine();
                    if (line.contains("dialout") && !line.contains(getenv("USER"))) {
                        permFile.close();
                        showAppMessage(tr(
                            "The current user does not have the correct permissions to access serial devices. "
                            "You should also remove modemmanager since it also interferes.<br/><br/>"
                            "If you are using Ubuntu, execute the following commands to fix these issues:<br/>"
                            "<pre>sudo usermod -a -G dialout $USER<br/>"
                            "sudo apt-get remove modemmanager</pre>"));
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
    MAVLinkProtocol::instance()->checkForLostLogFiles();

    // Load known link configurations
    LinkManager::instance()->loadLinkConfigurationList();

    // Probe for joysticks
    JoystickManager::instance()->init();

    if (_settingsUpgraded) {
        showAppMessage(tr("The format for %1 saved settings has been modified. "
                    "Your saved settings have been reset to defaults.").arg(applicationName()));
    }

    // Connect links with flag AutoconnectLink
    LinkManager::instance()->startAutoConnectedLinks();
}

void QGCApplication::reportMissingParameter(int componentId, const QString &name)
{
    const QPair<int, QString> missingParam(componentId, name);

    if (!_missingParams.contains(missingParam)) {
        _missingParams.append(missingParam);
    }
    _missingParamsDelayedDisplayTimer.start();
}

void QGCApplication::_missingParamsDisplay()
{
    if (_missingParams.isEmpty()) {
        return;
    }

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

QObject *QGCApplication::_rootQmlObject()
{
    if (_qmlAppEngine && _qmlAppEngine->rootObjects().size()) {
        return _qmlAppEngine->rootObjects()[0];
    }

    return nullptr;
}

void QGCApplication::showCriticalVehicleMessage(const QString &message)
{
    // PreArm messages are handled by Vehicle and shown in Map
    if (message.startsWith(QStringLiteral("PreArm")) || message.startsWith(QStringLiteral("preflight"), Qt::CaseInsensitive)) {
        return;
    }

    QObject *const rootQmlObject = _rootQmlObject();
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

void QGCApplication::showAppMessage(const QString &message, const QString &title)
{
    const QString dialogTitle = title.isEmpty() ? applicationName() : title;

    QObject *const rootQmlObject = _rootQmlObject();
    if (rootQmlObject) {
        QVariant varReturn;
        QVariant varMessage = QVariant::fromValue(message);
        QMetaObject::invokeMethod(rootQmlObject, "_showMessageDialog", Q_RETURN_ARG(QVariant, varReturn), Q_ARG(QVariant, dialogTitle), Q_ARG(QVariant, varMessage));
    } else if (runningUnitTests()) {
        // Unit tests can run without UI
        // We don't use a logging category to make it easier to debug unit tests
        qDebug() << "QGCApplication::showAppMessage unittest title:message" << dialogTitle << message;
    } else {
        // UI isn't ready yet
        _delayedAppMessages.append(QPair<QString, QString>(dialogTitle, message));
        QTimer::singleShot(200, this, &QGCApplication::_showDelayedAppMessages);
    }
}

void QGCApplication::showRebootAppMessage(const QString &message, const QString &title)
{
    static QTime lastRebootMessage;

    const QTime currentTime = QTime::currentTime();
    const QTime previousTime = lastRebootMessage;
    lastRebootMessage = currentTime;

    if (previousTime.isValid() && (previousTime.msecsTo(currentTime) < (60 * 1000 * 2))) {
        // Debounce reboot messages
        return;
    }

    showAppMessage(message, title);
}

void QGCApplication::_showDelayedAppMessages()
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

QQuickWindow *QGCApplication::mainRootWindow()
{
    if (!_mainRootWindow) {
        _mainRootWindow = qobject_cast<QQuickWindow*>(_rootQmlObject());
    }

    return _mainRootWindow;
}

void QGCApplication::showVehicleConfig()
{
    if (_rootQmlObject()) {
      QMetaObject::invokeMethod(_rootQmlObject(), "showVehicleConfig");
    }
}

void QGCApplication::qmlAttemptWindowClose()
{
    if (_rootQmlObject()) {
        QMetaObject::invokeMethod(_rootQmlObject(), "attemptWindowClose");
    }
}

void QGCApplication::_checkForNewVersion()
{
    if (_runningUnitTests) {
        return;
    }

    if (!_parseVersionText(applicationVersion(), _majorVersion, _minorVersion, _buildVersion)) {
        return;
    }

    const QString versionCheckFile = QGCCorePlugin::instance()->stableVersionCheckFileUrl();
    if (!versionCheckFile.isEmpty()) {
        QGCFileDownload *const download = new QGCFileDownload(this);
        (void) connect(download, &QGCFileDownload::finished, this, &QGCApplication::_qgcCurrentStableVersionDownloadComplete);
        if (!download->start(versionCheckFile)) {
            qCDebug(QGCApplicationLog) << "Download QGC stable version failed to start" << download->errorString();
            download->deleteLater();
        }
    }
}

void QGCApplication::_qgcCurrentStableVersionDownloadComplete(bool success, const QString &localFile, const QString &errorMsg)
{
    if (success) {
        QFile versionFile(localFile);
        if (versionFile.open(QIODevice::ReadOnly)) {
            QTextStream textStream(&versionFile);
            const QString version = textStream.readLine();

            qCDebug(QGCApplicationLog) << version;

            int majorVersion, minorVersion, buildVersion;
            if (_parseVersionText(version, majorVersion, minorVersion, buildVersion)) {
                if (_majorVersion < majorVersion ||
                        ((_majorVersion == majorVersion) && (_minorVersion < minorVersion)) ||
                        ((_majorVersion == majorVersion) && (_minorVersion == minorVersion) && (_buildVersion < buildVersion))) {
                    showAppMessage(tr("There is a newer version of %1 available. You can download it from %2.").arg(applicationName()).arg(QGCCorePlugin::instance()->stableDownloadLocation()), tr("New Version Available"));
                }
            }
        }
    } else if (!errorMsg.isEmpty()) {
        qCDebug(QGCApplicationLog) << "Download QGC stable version failed" << errorMsg;
    }

    sender()->deleteLater();
}

bool QGCApplication::_parseVersionText(const QString &versionString, int &majorVersion, int &minorVersion, int &buildVersion)
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

QString QGCApplication::cachedParameterMetaDataFile()
{
    QSettings settings;
    const QDir parameterDir = QFileInfo(settings.fileName()).dir();
    return parameterDir.filePath(QStringLiteral("ParameterFactMetaData.json"));
}

QString QGCApplication::cachedAirframeMetaDataFile()
{
    QSettings settings;
    const QDir airframeDir = QFileInfo(settings.fileName()).dir();
    return airframeDir.filePath(QStringLiteral("PX4AirframeFactMetaData.xml"));
}

int QGCApplication::CompressedSignalList::_signalIndex(const QMetaMethod &method)
{
    if (method.methodType() != QMetaMethod::Signal) {
        qCWarning(QGCApplicationLog) << "Internal error:" << Q_FUNC_INFO <<  "not a signal" << method.methodType();
        return -1;
    }

    int index = -1;
    const QMetaObject *metaObject = method.enclosingMetaObject();
    for (int i=0; i<=method.methodIndex(); i++) {
        if (metaObject->method(i).methodType() != QMetaMethod::Signal) {
            continue;
        }
        index++;
    }

    return index;
}

void QGCApplication::CompressedSignalList::add(const QMetaMethod &method)
{
    const QMetaObject *metaObject = method.enclosingMetaObject();
    const int signalIndex = _signalIndex(method);

    if (signalIndex != -1 && !contains(metaObject, signalIndex)) {
        _signalMap[method.enclosingMetaObject()].insert(signalIndex);
    }
}

void QGCApplication::CompressedSignalList::remove(const QMetaMethod &method)
{
    const int signalIndex = _signalIndex(method);
    const QMetaObject *const metaObject = method.enclosingMetaObject();

    if (signalIndex != -1 && _signalMap.contains(metaObject) && _signalMap[metaObject].contains(signalIndex)) {
        _signalMap[metaObject].remove(signalIndex);
        if (_signalMap[metaObject].count() == 0) {
            _signalMap.remove(metaObject);
        }
    }
}

bool QGCApplication::CompressedSignalList::contains(const QMetaObject *metaObject, int signalIndex)
{
    return _signalMap.contains(metaObject) && _signalMap[metaObject].contains(signalIndex);
}

void QGCApplication::addCompressedSignal(const QMetaMethod &method)
{
    _compressedSignals.add(method);
}

void QGCApplication::removeCompressedSignal(const QMetaMethod &method)
{
    _compressedSignals.remove(method);
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
bool QGCApplication::compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents)
{
    if (event->type() != QEvent::MetaCall) {
        return QApplication::compressEvent(event, receiver, postedEvents);
    }

    const QMetaCallEvent *mce = static_cast<QMetaCallEvent*>(event);
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
QT_WARNING_POP

bool QGCApplication::event(QEvent *e)
{
    if (e->type() == QEvent::Quit) {
        if (!_mainRootWindow) {
            return QApplication::event(e);
        }
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

QGCImageProvider *QGCApplication::qgcImageProvider()
{
    return dynamic_cast<QGCImageProvider*>(_qmlAppEngine->imageProvider(_qgcImageProviderId));
}

void QGCApplication::shutdown()
{
    qCDebug(QGCApplicationLog) << "Exit";

    if (_videoManagerInitialized) {
        VideoManager::instance()->cleanup();
    }

    QGCCorePlugin::instance()->cleanup();

    if (_runningUnitTests || _simpleBootTest) {
        const QSettings settings;
        const QString settingsFile = settings.fileName();
        if (QFile::exists(settingsFile)) {
            if (QFile::remove(settingsFile)) {
                qCDebug(QGCApplicationLog) << "Removed test run settings file:" << settingsFile;
            } else {
                qCWarning(QGCApplicationLog) << "Failed to remove test run settings file:" << settingsFile;
            }
        }

        // Remove the app-specific settings directory (parent of ParamCache)
        QDir settingsAppDir(ParameterManager::parameterCacheDir());
        settingsAppDir.cdUp();
        if (settingsAppDir.exists()) {
            if (settingsAppDir.removeRecursively()) {
                qCDebug(QGCApplicationLog) << "Removed test run settings directory:" << settingsAppDir.absolutePath();
            } else {
                qCWarning(QGCApplicationLog) << "Failed to remove test run settings directory:" << settingsAppDir.absolutePath();
            }
        }

        QDir appDir(SettingsManager::instance()->appSettings()->savePath()->rawValue().toString());
        if (appDir.exists()) {
            if (appDir.removeRecursively()) {
                qCDebug(QGCApplicationLog) << "Removed test run app data directory:" << appDir.absolutePath();
            } else {
                qCWarning(QGCApplicationLog) << "Failed to remove test run app data directory:" << appDir.absolutePath();
            }
        }
    }

    // This is bad, but currently qobject inheritances are incorrect and cause crashes on exit without
    delete _qmlAppEngine;
}

