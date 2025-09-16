/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCApplication.h"

#include <QtCore/private/qthread_p.h>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtGui/QFontDatabase>
#include <QtGui/QIcon>
#include <QtGui/QOpenGLContext>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickImageProvider>
#include <QtQuick/QQuickWindow>
#include <QtQuickControls2/QQuickStyle>

#include "AppSettings.h"
#include "AudioOutput.h"
#include "FollowMe.h"
#include "JoystickManager.h"
#include "JsonHelper.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MavlinkSettings.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "PositionManager.h"
#include "QGCCommandLineParser.h"
#include "QGCCorePlugin.h"
#include "QGCFileDownload.h"
#include "QGCImageProvider.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VideoManager.h"

QGC_LOGGING_CATEGORY(QGCApplicationLog, "qgc.qgcapplication")

using namespace Qt::StringLiterals;

QGCApplication::QGCApplication(int &argc, char *argv[], const QGCCommandLineParser::CommandLineParseResult &cli)
    : QApplication(argc, argv)
    , _runningUnitTests(cli.runningUnitTests)
    , _simpleBootTest(cli.simpleBootTest)
    , _fakeMobile(cli.fakeMobile)
    , _logOutput(cli.logOutput)
    , _systemId(cli.systemId.value_or(0))
    , _qgcTranslatorSourceCode(std::make_unique<QTranslator>())
    , _qgcTranslatorQtLibs(std::make_unique<QTranslator>())
    , _currentLocale(QLocale::system())
{
    qCDebug(QGCApplicationLog) << this;

    // Start elapsed timer immediately
    _msecsElapsedTime.start();

    // Setup for network proxy support
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    const bool fClearSettingsOptions = cli.clearSettingsOptions;
    const bool fClearCache = cli.clearCache;
    const QString loggingOptions = cli.loggingOptions.value_or(QString());

    // Set up timer for delayed missing fact display with proper parent
    _missingParamsDelayedDisplayTimer.setSingleShot(true);
    _missingParamsDelayedDisplayTimer.setInterval(_missingParamsDelayedDisplayTimerTimeout);
    (void) connect(&_missingParamsDelayedDisplayTimer, &QTimer::timeout, this, &QGCApplication::_missingParamsDisplay, Qt::QueuedConnection);

    // Set application information
    QString applicationName;
    if (_runningUnitTests || _simpleBootTest) {
        // We don't want unit tests to use the same QSettings space as the normal app. So we tweak the app
        // name. Also we want to run unit tests with clean settings every time.
        applicationName = QLatin1String("%1_unittest").arg(QLatin1String(QGC_APP_NAME));
    } else {
#ifdef QGC_DAILY_BUILD
        // This gives daily builds their own separate settings space. Allowing you to use daily and stable builds
        // side by side without daily screwing up your stable settings.
        applicationName = QLatin1String("%1 Daily").arg(QLatin1String(QGC_APP_NAME));
#else
        applicationName = QLatin1String(QGC_APP_NAME);
#endif
    }

    setApplicationName(applicationName);
    setOrganizationName(QLatin1String(QGC_ORG_NAME));
    setOrganizationDomain(QLatin1String(QGC_ORG_DOMAIN));

    const QString versionStr = QLatin1String(QGC_APP_VERSION_STR);
    setApplicationVersion(versionStr);
    _appVersion = _parseVersionText(versionStr);

    if (_appVersion.isNull()) {
        qCWarning(QGCApplicationLog) << "Failed to parse application version:" << versionStr;
        // Set a default version to prevent issues
        _appVersion = QVersionNumber(1, 0, 0);
    }

    // Set settings format
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;
    qCDebug(QGCApplicationLog) << "Settings location" << settings.fileName() << "Is writable?:" << settings.isWritable();

    if (!settings.isWritable()) {
        qCWarning(QGCApplicationLog) << "Settings location is not writable";
    }

    // The setting will delete all settings on this boot
    bool fClearSettings = fClearSettingsOptions || settings.contains(_deleteAllSettingsKey);

    if (_runningUnitTests || _simpleBootTest) {
        // Unit tests run with clean settings
        fClearSettings = true;
    }

    if (fClearSettings) {
        // User requested settings to be cleared on command line
        settings.clear();
        settings.remove(_deleteAllSettingsKey);

        // Clear parameter cache
        QDir paramDir = ParameterManager::parameterCacheDir();
        if (paramDir.exists()) {
            (void) paramDir.removeRecursively();
            (void) paramDir.mkpath(paramDir.absolutePath());
        }
    } else {
        // Determine if upgrade message for settings version bump is required
        if (settings.contains(_settingsVersionKey)) {
            bool ok = false;
            const int version = settings.value(_settingsVersionKey).toInt(&ok);
            if (ok && (version != QGC_SETTINGS_VERSION)) {
                settings.clear();
                _settingsUpgraded = true;
                emit settingsUpgraded();
            }
        }
    }
    settings.setValue(_settingsVersionKey, QGC_SETTINGS_VERSION);

    if (fClearCache) {
        const QDir paramCacheDir = ParameterManager::parameterCacheDir();
        if (paramCacheDir.exists()) {
            QDir dir(paramCacheDir);
            (void) dir.removeRecursively();
        }

        QFile airframe(cachedAirframeMetaDataFile());
        if (airframe.exists()) {
            (void) airframe.remove();
        }

        QFile parameter(cachedParameterMetaDataFile());
        if (parameter.exists()) {
            (void) parameter.remove();
        }
    }

    // Set up our logging filters
    QGCLoggingCategoryRegister::instance()->setFilterRulesFromSettings(loggingOptions);

    // We need to set language as early as possible prior to loading JSON files.
    // This is safe here because no threads have been created yet
    setLanguage();

#ifndef QGC_DAILY_BUILD
    _checkForNewVersion();
#endif
}

QGCApplication::~QGCApplication()
{
    qCDebug(QGCApplicationLog) << this;
}

void QGCApplication::init()
{
    SettingsManager::instance()->init();

    if (_systemId > 0) {
        qCDebug(QGCApplicationLog) << "Setting MAVLink System ID to:" << _systemId;
        SettingsManager::instance()->mavlinkSettings()->gcsMavlinkSystemID()->setRawValue(_systemId);
    }

    // Load fonts - although this should really be in _initForNormalAppBoot,
    // putting it here allows us to create unit tests which pop up more easily
    if (QFontDatabase::addApplicationFont(QLatin1String(":/fonts/opensans")) < 0) {
        qCWarning(QGCApplicationLog) << "Could not load /fonts/opensans font";
    }

    if (QFontDatabase::addApplicationFont(QLatin1String(":/fonts/opensans-demibold")) < 0) {
        qCWarning(QGCApplicationLog) << "Could not load /fonts/opensans-demibold font";
    }

    if (_simpleBootTest) {
        // Since GStream builds are so problematic we initialize video during the simple boot test
        // to make sure it works and verifies plugin availability.
        _initVideo();
    } else if (!_runningUnitTests) {
        _initForNormalAppBoot();
    }

    // Mark initialization complete - after this point, changing QLocale::setDefault() is unsafe
    _appInitialized = true;
}

void QGCApplication::_initVideo()
{
#ifdef QGC_GST_STREAMING
    QOpenGLContext testContext;
    if (!testContext.create()) {
        qCWarning(QGCApplicationLog) << "OpenGL context creation failed - video may not work";
    } else {
        const QSurfaceFormat format = testContext.format();
        if (format.majorVersion() < 2) {
            qCWarning(QGCApplicationLog) << "OpenGL version too old:"
                                         << format.majorVersion() << "." << format.minorVersion()
                                         << "- video may not work properly";
        }
    }

    // Gstreamer video playback requires OpenGL
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    QGCCorePlugin::instance();  // CorePlugin must be initialized before VideoManager for Video Cleanup
    VideoManager::instance();
    _videoManagerInitialized = true;
}

void QGCApplication::_initForNormalAppBoot()
{
    _initVideo(); // GStreamer must be initialized before QmlEngine

    QQuickStyle::setStyle(QLatin1String("Basic"));

    QGCCorePlugin::instance()->init();
    MAVLinkProtocol::instance()->init();
    MultiVehicleManager::instance()->init();

    _qmlAppEngine = QGCCorePlugin::instance()->createQmlApplicationEngine(this);
    if (!_qmlAppEngine) {
        qCCritical(QGCApplicationLog) << "Failed to create QML application engine";
        return;
    }

    (void) connect(_qmlAppEngine, &QQmlApplicationEngine::objectCreationFailed, this, &QCoreApplication::quit, Qt::QueuedConnection);

    QGCCorePlugin::instance()->createRootWindow(_qmlAppEngine);

    AudioOutput::instance()->init(SettingsManager::instance()->appSettings()->audioMuted());
    FollowMe::instance()->init();
    QGCPositionManager::instance()->init();
    LinkManager::instance()->init();

    QQuickWindow *rootWindow = mainRootWindow();
    if (rootWindow) {
        VideoManager::instance()->init(rootWindow);
    } else {
        qCWarning(QGCApplicationLog) << "No root window available for VideoManager";
    }

    // Image provider for Optical Flow
    _qmlAppEngine->addImageProvider(_qgcImageProviderId, new QGCImageProvider());

    // Set the window icon now that custom plugin has a chance to override it
#ifdef Q_OS_LINUX
    QUrl windowIcon = QUrl(QLatin1String("qrc:/res/qgroundcontrol.ico"));
    windowIcon = _qmlAppEngine->interceptUrl(windowIcon, QQmlAbstractUrlInterceptor::UrlString);
    // The interceptor needs "qrc:/path" but QIcon expects ":/path"
    const QString iconPath = windowIcon.path();
    if (!iconPath.isEmpty()) {
        setWindowIcon(QIcon(":" + iconPath));
    }
#endif

    // Safe to show popup error messages now that main window is created
    _showErrorsInToolbar = true;

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
    if (!_runningUnitTests) {
        _checkSerialPortPermissions();
    }
#endif

    // Now that main window is up check for lost log files
    MAVLinkProtocol::instance()->checkForLostLogFiles();

    // Load known link configurations
    LinkManager::instance()->loadLinkConfigurationList();

    // Probe for joysticks
    JoystickManager::instance()->init();

    if (_settingsUpgraded) {
        showAppMessage(tr(
            "The format for %1 saved settings has been modified. "
            "Your saved settings have been reset to defaults."
        ).arg(applicationName()));
    }

    // Connect links with flag AutoconnectLink
    LinkManager::instance()->startAutoConnectedLinks();
}

void QGCApplication::deleteAllSettingsNextBoot()
{
    QSettings settings;
    settings.setValue(_deleteAllSettingsKey, true);
}

void QGCApplication::clearDeleteAllSettingsNextBoot()
{
    QSettings settings;
    settings.remove(_deleteAllSettingsKey);
}

void QGCApplication::reportMissingParameter(int componentId, const QString &name)
{
    if (name.isEmpty()) {
        qCWarning(QGCApplicationLog) << "Empty parameter name reported for component" << componentId;
        return;
    }

    const QPair<int, QString> missingParam(componentId, name);
    {
        const QMutexLocker locker(&_missingParamsMutex);
        if (!_missingParams.contains(missingParam)) {
            _missingParams.append(missingParam);
        }
    }
    _missingParamsDelayedDisplayTimer.start();
}

void QGCApplication::_missingParamsDisplay()
{
    QList<QPair<int, QString>> paramsCopy;
    {
        const QMutexLocker locker(&_missingParamsMutex);
        if (_missingParams.isEmpty()) {
            return;
        }
        paramsCopy = std::move(_missingParams);
        _missingParams.clear();
    }

    QStringList paramList;
    paramList.reserve(paramsCopy.size());

    for (const QPair<int, QString>& missingParam: std::as_const(paramsCopy)) {
        paramList.append(QStringLiteral("%1:%2").arg(missingParam.first).arg(missingParam.second));
    }

    showAppMessage(tr(
        "Parameters are missing from firmware. You may be running a version of firmware "
        "which is not fully supported or your firmware has a bug in it. Missing params: %1"
    ).arg(paramList.join(QLatin1String(", "))));
}

QObject *QGCApplication::_rootQmlObject()
{
    if (_qmlAppEngine && !_qmlAppEngine->rootObjects().isEmpty()) {
        return _qmlAppEngine->rootObjects().first();
    }
    return nullptr;
}

void QGCApplication::showCriticalVehicleMessage(const QString &message)
{
    if (message.isEmpty()) {
        return;
    }

    // PreArm messages are handled by Vehicle and shown in Map
    if (message.startsWith(QLatin1String("PreArm")) ||
        message.startsWith(QLatin1String("preflight"), Qt::CaseInsensitive)) {
        return;
    }

    QObject * const rootQmlObject = _rootQmlObject();
    if (rootQmlObject && _showErrorsInToolbar) {
        QVariant varReturn;
        const QVariant varMessage = QVariant::fromValue(message);
        const bool success = QMetaObject::invokeMethod(rootQmlObject, "showCriticalVehicleMessage",
                                                       Q_RETURN_ARG(QVariant, varReturn),
                                                       Q_ARG(QVariant, varMessage));
        if (!success) {
            qCWarning(QGCApplicationLog) << "Failed to invoke";
        }
    } else if (runningUnitTests() || !_showErrorsInToolbar) {
        // Unit tests can run without UI
        qCDebug(QGCApplicationLog) << "unittest" << message;
    } else {
        qCWarning(QGCApplicationLog) << "Internal error: no root object available";
    }
}

void QGCApplication::showAppMessage(const QString &message, const QString &title)
{
    if (message.isEmpty()) {
        return;
    }

    const QString dialogTitle = title.isEmpty() ? applicationName() : title;

    QObject * const rootQmlObject = _rootQmlObject();
    if (rootQmlObject) {
        QVariant varReturn;
        const QVariant varMessage = QVariant::fromValue(message);
        const QVariant varTitle = QVariant::fromValue(dialogTitle);
        const bool success = QMetaObject::invokeMethod(rootQmlObject, "_showMessageDialog",
                                                       Q_RETURN_ARG(QVariant, varReturn),
                                                       Q_ARG(QVariant, varTitle),
                                                       Q_ARG(QVariant, varMessage));
        if (!success) {
            qCWarning(QGCApplicationLog) << "Failed to invoke _showMessageDialog";
        }
    } else if (runningUnitTests()) {
        // Unit tests can run without UI
        qCDebug(QGCApplicationLog) << "unittest title:message" << dialogTitle << message;
    } else {
        // UI isn't ready yet - queue for later
        {
            const QMutexLocker locker(&_delayedMessagesMutex);
            _delayedAppMessages.append(QPair<QString, QString>(dialogTitle, message));
        }
        QTimer::singleShot(200, this, &QGCApplication::_showDelayedAppMessages);
    }
}

void QGCApplication::showRebootAppMessage(const QString &message, const QString &title)
{
    static QMutex rebootMutex;
    static QTime lastRebootMessage;

    QMutexLocker locker(&rebootMutex);

    const QTime currentTime = QTime::currentTime();

    if (lastRebootMessage.isValid() &&
        (lastRebootMessage.msecsTo(currentTime) < (60 * 1000 * 2))) {
        // Debounce reboot messages - ignore if within 2 minutes
        return;
    }

    lastRebootMessage = currentTime;
    locker.unlock();

    showAppMessage(message, title);
}

void QGCApplication::_showDelayedAppMessages()
{
    if (_rootQmlObject()) {
        QList<QPair<QString, QString>> messagesCopy;
        {
            const QMutexLocker locker(&_delayedMessagesMutex);
            messagesCopy = std::move(_delayedAppMessages);
            _delayedAppMessages.clear();
        }

        for (const QPair<QString, QString>& appMsg: std::as_const(messagesCopy)) {
            showAppMessage(appMsg.second, appMsg.first);
        }
    } else {
        QTimer::singleShot(200, this, &QGCApplication::_showDelayedAppMessages);
    }
}

QQuickWindow *QGCApplication::mainRootWindow()
{
    if (!_mainRootWindow) {
        _mainRootWindow = qobject_cast<QQuickWindow*>(_rootQmlObject());
    }
    return _mainRootWindow.data();
}

void QGCApplication::showVehicleConfig()
{
    QObject * const rootObject = _rootQmlObject();
    if (rootObject) {
        const bool success = QMetaObject::invokeMethod(rootObject, "showVehicleConfig");
        if (!success) {
            qCWarning(QGCApplicationLog) << "Failed to invoke showVehicleConfig";
        }
    }
}

void QGCApplication::qmlAttemptWindowClose()
{
    QObject * const rootObject = _rootQmlObject();
    if (rootObject) {
        const bool success = QMetaObject::invokeMethod(rootObject, "attemptWindowClose");
        if (!success) {
            qCWarning(QGCApplicationLog) << "Failed to invoke attemptWindowClose";
        }
    }
}

void QGCApplication::_checkForNewVersion()
{
    if (_runningUnitTests) {
        return;
    }

    if (_appVersion.isNull() || (_appVersion.majorVersion() <= 0)) {
        return;
    }

    const QString versionCheckFile = QGCCorePlugin::instance()->stableVersionCheckFileUrl();
    if (versionCheckFile.isEmpty()) {
        return;
    }

    // Parent object ensures proper cleanup
    QGCFileDownload * const download = new QGCFileDownload(this);
    (void) connect(download, &QGCFileDownload::downloadComplete, this, &QGCApplication::_qgcCurrentStableVersionDownloadComplete);
    if (!download->download(versionCheckFile)) {
        download->deleteLater();
        qCWarning(QGCApplicationLog) << "Download QGC stable version failed";
    }
}

void QGCApplication::_qgcCurrentStableVersionDownloadComplete(const QString &remoteFile, const QString &localFile, const QString &errorMsg)
{
    Q_UNUSED(remoteFile)

    if (errorMsg.isEmpty()) {
        QFile versionFile(localFile);
        if (versionFile.open(QIODevice::ReadOnly)) {
            QTextStream textStream(&versionFile);
            const QString versionText = textStream.readLine();
            versionFile.close();

            qCDebug(QGCApplicationLog) << "Remote version:" << versionText;

            const QVersionNumber remoteVersion = _parseVersionText(versionText);
            if (!remoteVersion.isNull() && _appVersion < remoteVersion) {
                const QString message = tr("There is a newer version of %1 available. You can download it from %2.")
                                        .arg(applicationName())
                                        .arg(QGCCorePlugin::instance()->stableDownloadLocation());
                showAppMessage(message, tr("New Version Available"));
            }
        }
    } else {
        qCDebug(QGCApplicationLog) << "Download QGC stable version failed:" << errorMsg;
    }

    sender()->deleteLater();
}

QVersionNumber QGCApplication::_parseVersionText(const QString &versionString)
{
    if (versionString.isEmpty()) {
        return QVersionNumber();
    }

    static const QRegularExpression regExp(uR"(v?(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?)"_s);
    const QRegularExpressionMatch match = regExp.match(versionString);

    if (match.hasMatch()) {
        QList<int> segments;
        for (int i = 1; i <= match.lastCapturedIndex(); ++i) {
            bool ok = false;
            const int value = match.captured(i).toInt(&ok);
            if (ok) {
                segments.append(value);
            }
        }

        if (!segments.isEmpty()) {
            return QVersionNumber(segments);
        }
    }

    return QVersionNumber();
}

QString QGCApplication::cachedParameterMetaDataFile()
{
    QSettings settings;
    const QDir parameterDir = QFileInfo(settings.fileName()).dir();
    return parameterDir.filePath(QLatin1String("ParameterFactMetaData.xml"));
}

QString QGCApplication::cachedAirframeMetaDataFile()
{
    QSettings settings;
    const QDir airframeDir = QFileInfo(settings.fileName()).dir();
    return airframeDir.filePath(QLatin1String("PX4AirframeFactMetaData.xml"));
}

void QGCApplication::addCompressedSignal(const QMetaMethod &method)
{
    _compressedSignals.add(method);
}

void QGCApplication::removeCompressedSignal(const QMetaMethod &method)
{
    _compressedSignals.remove(method);
}

bool QGCApplication::compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents)
{
    if (!event || !receiver || (event->type() != QEvent::MetaCall)) {
        return QApplication::compressEvent(event, receiver, postedEvents);
    }

    const QMetaCallEvent * const mce = static_cast<const QMetaCallEvent*>(event);
    if (!mce->sender() || !_compressedSignals.contains(mce->sender()->metaObject(), mce->signalId())) {
        return QApplication::compressEvent(event, receiver, postedEvents);
    }

    for (auto it = postedEvents->begin(); it != postedEvents->end(); ++it) {
        QPostEvent &cur = *it;
        if ((cur.receiver != receiver) || !cur.event || (cur.event->type() != event->type())) {
            continue;
        }

        const QMetaCallEvent * const cur_mce = static_cast<const QMetaCallEvent*>(cur.event);
        if ((cur_mce->sender() != mce->sender()) ||
            (cur_mce->signalId() != mce->signalId()) ||
            (cur_mce->id() != mce->id())) {
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
    if (!e) {
        return false;
    }

    if (e->type() == QEvent::Quit) {
        // On OSX if the user selects Quit from the menu (or Command-Q) the ApplicationWindow does not signal closing.
        // Instead you get a Quit event here only. This in turn causes the standard QGC shutdown sequence to not run.
        // So in this case we close the window ourselves such that the signal is sent and the normal shutdown sequence runs.
        if (_mainRootWindow) {
            const bool forceClose = _mainRootWindow->property("_forceClose").toBool();
            qCDebug(QGCApplicationLog) << "Quit event, forceClose:" << forceClose;

            // forceClose
            //  true:   Standard QGC shutdown sequence is complete. Let the app quit normally.
            //  false:  QGC shutdown sequence has not been run yet. Close the main window to kick off shutdown.
            if (!forceClose) {
                (void) _mainRootWindow->close();
                e->ignore();
                return true;
            }
        }
    }

    return QApplication::event(e);
}

QGCImageProvider *QGCApplication::qgcImageProvider()
{
    if (!_qmlAppEngine) {
        return nullptr;
    }

    return dynamic_cast<QGCImageProvider*>(_qmlAppEngine->imageProvider(_qgcImageProviderId));
}

void QGCApplication::shutdown()
{
    qCDebug(QGCApplicationLog) << "Shutdown started";

    // Stop any timers
    _missingParamsDelayedDisplayTimer.stop();

    // Clean up video manager first if initialized
    if (_videoManagerInitialized) {
        VideoManager::instance()->cleanup();
        _videoManagerInitialized = false;
    }

    // Clean up core plugin
    QGCCorePlugin::instance()->cleanup();

    // Clear window reference
    _mainRootWindow = nullptr;

    delete _qmlAppEngine;

    qCDebug(QGCApplicationLog) << "Shutdown complete";
}

QString QGCApplication::numberToString(quint64 number) const
{
    return _currentLocale.toString(number);
}

QString QGCApplication::bigSizeToString(quint64 size) const
{
    QString result;
    const QLocale &locale = _currentLocale;

    if (size < 1024ULL) {
        result = locale.toString(size) + "B";
    } else if (size < (1024ULL * 1024)) {
        result = locale.toString(static_cast<double>(size) / 1024.0, 'f', 1) + "KB";
    } else if (size < (1024ULL * 1024 * 1024)) {
        result = locale.toString(static_cast<double>(size) / (1024.0 * 1024), 'f', 1) + "MB";
    } else if (size < (1024ULL * 1024 * 1024 * 1024)) {
        result = locale.toString(static_cast<double>(size) / (1024.0 * 1024 * 1024), 'f', 1) + "GB";
    } else {
        result = locale.toString(static_cast<double>(size) / (1024.0 * 1024 * 1024 * 1024), 'f', 1) + "TB";
    }

    return result;
}

QString QGCApplication::bigSizeMBToString(quint64 size_MB) const
{
    QString result;
    const QLocale &locale = _currentLocale;

    if (size_MB < 1024) {
        result = locale.toString(static_cast<double>(size_MB), 'f', 0) + " MB";
    } else if (size_MB < (1024ULL * 1024)) {
        result = locale.toString(static_cast<double>(size_MB) / 1024.0, 'f', 1) + " GB";
    } else {
        result = locale.toString(static_cast<double>(size_MB) / (1024.0 * 1024), 'f', 2) + " TB";
    }

    return result;
}

void QGCApplication::setLanguage()
{
    // WARNING: This method is called both at startup (safe) and potentially later (unsafe)
    // QLocale::setDefault() should only be called before threads are created

    QLocale locale = QLocale::system();
    qCDebug(QGCApplicationLog) << "System reported locale:" << locale
                               << "; Name:" << locale.name()
                               << "; Preferred (used in maps):"
                               << (!locale.uiLanguages().isEmpty() ? locale.uiLanguages().constFirst() : "None");

    const QLocale::Language possibleLocale = AppSettings::_qLocaleLanguageEarlyAccess();
    if (possibleLocale != QLocale::AnyLanguage) {
        locale = QLocale(possibleLocale);
    }

    // Store the selected locale for later use
    _currentLocale = locale;

    // Load Korean fonts if needed
    if (locale == QLocale::Korean) {
        qCDebug(QGCApplicationLog) << "Loading Korean fonts for locale:" << locale.name();
        if (QFontDatabase::addApplicationFont(QLatin1String(":/fonts/NanumGothic-Regular")) < 0) {
            qCWarning(QGCApplicationLog) << "Could not load /fonts/NanumGothic-Regular font";
        }
        if (QFontDatabase::addApplicationFont(QLatin1String(":/fonts/NanumGothic-Bold")) < 0) {
            qCWarning(QGCApplicationLog) << "Could not load /fonts/NanumGothic-Bold font";
        }
    }

    qCDebug(QGCApplicationLog) << "Loading localizations for:" << locale.name();

    // Remove existing translators
    (void) removeTranslator(JsonHelper::translator());
    (void) removeTranslator(_qgcTranslatorSourceCode.get());
    (void) removeTranslator(_qgcTranslatorQtLibs.get());

    if ((locale.language() != QLocale::English) || (locale.territory() != QLocale::UnitedStates)) {
        // Only set default locale during initial startup (constructor)
        // After threads are created, changing the default locale is unsafe
        if (!_appInitialized) {
            QLocale::setDefault(locale);
        } else {
            qCWarning(QGCApplicationLog) << "Cannot change default locale after initialization due to thread safety."
                                         << "Translations will be updated but default locale remains unchanged.";
        }

        // Qt6 API for translations path
        const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);

        if (_qgcTranslatorQtLibs->load("qt_" + locale.name(), qtTranslationsPath)) {
            (void) installTranslator(_qgcTranslatorQtLibs.get());
        } else {
            qCWarning(QGCApplicationLog) << "Qt lib localization for" << locale.name() << "is not present";
        }

        if (_qgcTranslatorSourceCode->load(locale, QLatin1String("qgc_source_"), QString(), QLatin1String(":/i18n"))) {
            (void) installTranslator(_qgcTranslatorSourceCode.get());
        } else {
            qCWarning(QGCApplicationLog) << "Error loading source localization for" << locale.name();
        }

        if (JsonHelper::translator()->load(locale, QLatin1String("qgc_json_"), QString(), QLatin1String(":/i18n"))) {
            (void) installTranslator(JsonHelper::translator());
        } else {
            qCWarning(QGCApplicationLog) << "Error loading json localization for" << locale.name();
        }
    } else if (!_appInitialized) {
        // Even for en_US, set it as default during startup
        QLocale::setDefault(locale);
    }

    if (_qmlAppEngine) {
        _qmlAppEngine->retranslate();
    }

    emit languageChanged(locale);
}

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
void QGCApplication::_checkSerialPortPermissions()
{
    // Method 1: Check actual group membership using system calls
    bool hasDialoutAccess = false;
    bool hasUucp = false;
    bool hasTty = false;
    QString missingGroups;

    // Get the current user's effective UID
    const uid_t uid = getuid();

    // Get user info
    struct passwd *pw = getpwuid(uid);
    if (!pw) {
        qCWarning(QGCApplicationLog) << "Cannot determine current user";
        return;
    }

    const QString username = QString::fromLocal8Bit(pw->pw_name);
    const gid_t primary_gid = pw->pw_gid;

    // Get number of supplementary groups
    int ngroups = getgroups(0, nullptr);
    if (ngroups > 0) {
        QVector<gid_t> groups(ngroups);
        if (getgroups(ngroups, groups.data()) != -1) {
            // Add primary group to the list
            groups.append(primary_gid);

            // Check each group
            for (gid_t gid : groups) {
                struct group *grp = getgrgid(gid);
                if (grp) {
                    const QString groupName = QString::fromLocal8Bit(grp->gr_name);
                    if (groupName == "dialout") {
                        hasDialoutAccess = true;
                    } else if (groupName == "uucp") {
                        hasUucp = true;
                    } else if (groupName == "tty") {
                        hasTty = true;
                    }
                }
            }
        }
    }

    // Method 2: Also check by trying to access a serial port
    bool canAccessSerialDevices = false;
    QDir devDir("/dev");

    // Check common serial device patterns
    const QStringList serialPatterns = {
        "ttyUSB*",
        "ttyACM*",
        "ttyS*",
        "rfcomm*"
    };

    for (const QString &pattern : serialPatterns) {
        const QStringList devices = devDir.entryList(QStringList() << pattern, QDir::System);
        for (const QString &device : devices) {
            const QString devicePath = QStringLiteral("/dev/") + device;
            QFileInfo deviceInfo(devicePath);

            // Check if it's a character device (serial ports are character devices)
            if (!deviceInfo.exists()) {
                continue;
            }

            // On Linux, we can check if it's a character device by checking the file type
            struct stat st;
            if (stat(devicePath.toLocal8Bit().constData(), &st) == 0) {
                if (!S_ISCHR(st.st_mode)) {
                    continue;  // Not a character device
                }
            }

            QFile deviceFile(devicePath);
            // Try to open for read/write
            if (deviceFile.open(QIODevice::ReadWrite)) {
                canAccessSerialDevices = true;
                deviceFile.close();
                break;
            }
            // If we can't open it, check if it's a permission issue
            else if (deviceFile.exists()) {
                qCDebug(QGCApplicationLog) << "Cannot access serial device:" << devicePath
                                           << "Owner:" << deviceInfo.owner()
                                           << "Group:" << deviceInfo.group();
            }
        }
        if (canAccessSerialDevices) {
            break;
        }
    }

    // Method 3: Check ModemManager interference
    bool modemManagerRunning = false;
    QProcess mmCheck;
    mmCheck.start("systemctl", QStringList() << "is-active" << "ModemManager");
    if (mmCheck.waitForFinished(1000)) {
        const QString output = mmCheck.readAllStandardOutput().trimmed();
        modemManagerRunning = (output == "active");
    } else {
        // Fallback: check if process is running
        QProcess psCheck;
        psCheck.start("pgrep", QStringList() << "ModemManager");
        if (psCheck.waitForFinished(1000)) {
            modemManagerRunning = !psCheck.readAllStandardOutput().isEmpty();
        }
    }

    // Build list of missing groups and check which groups exist on system
    QStringList neededGroups;
    bool hasDialoutGroup = false;
    bool hasUucpGroup = false;

    if (!hasDialoutAccess && !hasUucp && !hasTty) {
        // User has no serial access groups at all

        // Check which groups exist on the system
        QFile groupFile("/etc/group");
        if (groupFile.open(QIODevice::ReadOnly)) {
            QTextStream stream(&groupFile);
            QString line;

            while (stream.readLineInto(&line)) {
                if (line.startsWith("dialout:")) {
                    hasDialoutGroup = true;
                } else if (line.startsWith("uucp:")) {
                    hasUucpGroup = true;
                }
            }
            groupFile.close();
        }

        // Prefer dialout if it exists, otherwise use uucp
        if (hasDialoutGroup) {
            neededGroups << "dialout";
        } else if (hasUucpGroup) {
            neededGroups << "uucp";
        } else {
            // Default to dialout if neither exists (shouldn't happen)
            neededGroups << "dialout";
        }
    }

    // Determine if we need to show a warning
    bool showWarning = false;
    QString warningMessage;

    if (!canAccessSerialDevices && !neededGroups.isEmpty()) {
        showWarning = true;

        // Build distribution-specific instructions
        QString distroName = "Linux";
        QString instructions;

        // Try to determine distribution
        QFile osRelease("/etc/os-release");
        if (osRelease.open(QIODevice::ReadOnly)) {
            QTextStream stream(&osRelease);
            QString line;
            while (stream.readLineInto(&line)) {
                if (line.startsWith("NAME=")) {
                    distroName = line.mid(5).remove('"').trimmed();
                    break;
                }
            }
            osRelease.close();
        }

        // Build instructions based on distribution
        if (distroName.contains("Ubuntu") || distroName.contains("Debian")) {
            instructions = QStringLiteral(
                "If you are using %1, execute the following commands to fix these issues:<br/>"
                "<pre>sudo usermod -a -G %2 $USER<br/>")
                .arg(distroName)
                .arg(neededGroups.first());

            if (modemManagerRunning) {
                instructions += "sudo apt-get remove modemmanager<br/>";
            }

            instructions += "# Then log out and log back in for changes to take effect</pre>";

        } else if (distroName.contains("Fedora") || distroName.contains("Red Hat") || distroName.contains("CentOS")) {
            instructions = QStringLiteral(
                "If you are using %1, execute the following commands to fix these issues:<br/>"
                "<pre>sudo usermod -a -G %2 $USER<br/>")
                .arg(distroName)
                .arg(neededGroups.first());

            if (modemManagerRunning) {
                instructions += "sudo systemctl disable ModemManager<br/>"
                                "sudo systemctl stop ModemManager<br/>";
            }

            instructions += "# Then log out and log back in for changes to take effect</pre>";

        } else if (distroName.contains("Arch") || distroName.contains("Manjaro")) {
            const QString groupName = hasUucpGroup ? "uucp" : "dialout";
            instructions = QStringLiteral(
                "If you are using %1, execute the following commands to fix these issues:<br/>"
                "<pre>sudo usermod -a -G %2 $USER<br/>")
                .arg(distroName)
                .arg(groupName);

            if (modemManagerRunning) {
                instructions += "sudo systemctl disable ModemManager<br/>"
                                "sudo systemctl stop ModemManager<br/>";
            }

            instructions += "# Then log out and log back in for changes to take effect</pre>";

        } else {
            // Generic instructions
            instructions = QStringLiteral(
                "To fix these issues:<br/>"
                "<pre>sudo usermod -a -G %1 $USER<br/>")
                .arg(neededGroups.first());

            if (modemManagerRunning) {
                instructions += "# Disable ModemManager (command varies by distribution)<br/>";
            }

            instructions += "# Then log out and log back in for changes to take effect</pre>";
        }

        warningMessage = tr(
            "The current user (%1) does not have the correct permissions to access serial devices. "
            "You need to be a member of the '%2' group.<br/><br/>")
            .arg(username)
            .arg(neededGroups.first());

        if (modemManagerRunning) {
            warningMessage += tr("Additionally, ModemManager is running and may interfere with serial devices.<br/><br/>");
        }

        warningMessage += instructions;

    } else if (modemManagerRunning && !canAccessSerialDevices) {
        // User has permissions but ModemManager might be interfering
        showWarning = true;
        warningMessage = tr(
            "ModemManager is running and may interfere with serial device access. "
            "Consider disabling it if you experience connection issues.<br/><br/>"
            "To disable ModemManager:<br/>"
            "<pre>sudo systemctl disable ModemManager<br/>"
            "sudo systemctl stop ModemManager</pre>");
    }

    if (showWarning) {
        showAppMessage(warningMessage, tr("Serial Port Permission Warning"));
    } else if (!canAccessSerialDevices) {
        // User has group membership but still can't access devices
        // This might be because no devices are connected, which is fine
        qCDebug(QGCApplicationLog) << "User" << username << "has serial group membership but no accessible devices found";
    }
}
#endif

/*===========================================================================*/

QGCApplication::CompressedSignalList::CompressedSignalList()
{
    qCDebug(QGCApplicationLog) << this;
}

QGCApplication::CompressedSignalList::~CompressedSignalList()
{
    qCDebug(QGCApplicationLog) << this;
}

int QGCApplication::CompressedSignalList::_signalIndex(const QMetaMethod &method)
{
    if (method.methodType() != QMetaMethod::Signal) {
        qCWarning(QGCApplicationLog) << "Internal error:" << "not a signal" << method.methodType();
        return -1;
    }

    int index = -1;
    const QMetaObject * const object = method.enclosingMetaObject();
    for (int i = 0; i <= method.methodIndex(); ++i) {
        if (object->method(i).methodType() != QMetaMethod::Signal) {
            continue;
        }
        ++index;
    }

    return index;
}

void QGCApplication::CompressedSignalList::add(const QMetaMethod &method)
{
    const QMetaObject * const object = method.enclosingMetaObject();
    const int signalIndex = _signalIndex(method);

    if ((signalIndex != -1) && object) {
        const QMutexLocker locker(&_mutex);
        (void) _signalMap[object].insert(signalIndex);
    }
}

void QGCApplication::CompressedSignalList::remove(const QMetaMethod &method)
{
    const int signalIndex = _signalIndex(method);
    const QMetaObject * const object = method.enclosingMetaObject();

    if ((signalIndex != -1) && object) {
        const QMutexLocker locker(&_mutex);
        auto it = _signalMap.find(object);
        if (it != _signalMap.constEnd()) {
            (void) it->remove(signalIndex);
            if (it->isEmpty()) {
                (void) _signalMap.erase(it);
            }
        }
    }
}

bool QGCApplication::CompressedSignalList::contains(const QMetaObject *metaObject, int signalIndex) const
{
    if (!metaObject) {
        return false;
    }

    const QMutexLocker locker(&_mutex);
    const auto it = _signalMap.constFind(metaObject);
    return ((it != _signalMap.constEnd()) && it->contains(signalIndex));
}
