/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <atomic>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QTranslator>
#include <QtCore/QVersionNumber>
#include <QtWidgets/QApplication>

namespace QGCCommandLineParser {
    struct CommandLineParseResult;
}

class QQmlApplicationEngine;
class QQuickWindow;
class QGCImageProvider;
class QGCApplication;
class QEvent;
class QPostEventList;
class QMetaMethod;
class QMetaObject;

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<QGCApplication*>(QApplication::instance()))

#if defined(qGuiApp)
#undef qGuiApp
#endif
#define qGuiApp (static_cast<QGCApplication*>(QGuiApplication::instance()))

#define qgcApp() qApp

Q_DECLARE_LOGGING_CATEGORY(QGCApplicationLog)

/// The main application and management class.
/// Needs QApplication base to support QtCharts module.
/// TODO: Use QtGraphs to convert to QGuiApplication
class QGCApplication : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(QLocale currentLocale READ getCurrentLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool runningUnitTests READ runningUnitTests CONSTANT)
    Q_PROPERTY(bool fakeMobile READ fakeMobile CONSTANT)

    /// Unit Test have access to creating and destroying singletons
    friend class UnitTest;

public:
    explicit QGCApplication(int &argc, char *argv[], const QGCCommandLineParser::CommandLineParseResult &args);
    ~QGCApplication() override;

    /// Sets the persistent flag to delete all settings the next time QGroundControl is started.
    static void deleteAllSettingsNextBoot();

    /// Clears the persistent flag to delete all settings the next time QGroundControl is started.
    static void clearDeleteAllSettingsNextBoot();

    bool runningUnitTests() const { return _runningUnitTests; }
    bool simpleBootTest() const { return _simpleBootTest; }

    /// Returns true if Qt debug output should be logged to a file
    bool logOutput() const { return _logOutput; }

    /// Used to report a missing Parameter. Warning will be displayed to user. Method may be called
    /// multiple times.
    void reportMissingParameter(int componentId, const QString &name);

    /// @return true: Fake ui into showing mobile interface
    bool fakeMobile() const { return _fakeMobile; }

    /// Set application language. Safe to call at startup, may have limitations after threads are created.
    void setLanguage();

    /// Get main application window
    QQuickWindow *mainRootWindow();

    /// Get milliseconds since application boot
    uint64_t msecsSinceBoot() const { return _msecsElapsedTime.elapsed(); }

    /// Locale-aware number formatting
    QString numberToString(quint64 number) const;
    QString bigSizeToString(quint64 size) const;
    QString bigSizeMBToString(quint64 size_MB) const;

    /// Registers the signal such that only the last duplicate signal added is left in the queue.
    void addCompressedSignal(const QMetaMethod &method);
    void removeCompressedSignal(const QMetaMethod &method);

    /// Alternative: Modern signal throttling approach for Qt 6.8+
    template<typename Func>
    void throttleSignal(QObject *sender, Func signal, int delayMs = 100);

    /// Override event processing for special handling
    bool event(QEvent *e) override;

    /// Get cached file paths
    static QString cachedParameterMetaDataFile();
    static QString cachedAirframeMetaDataFile();

    /// Perform initialize which is common to both normal application running and unit tests.
    void init();

    /// Clean shutdown of the application
    void shutdown();

    /// Although public, these methods are internal and should only be called by UnitTest code
    QQmlApplicationEngine *qmlAppEngine() const { return _qmlAppEngine; }

    /// Get current language
    QLocale getCurrentLanguage() const { return _currentLocale; }

    /// Get the QGC image provider
    QGCImageProvider *qgcImageProvider();

    /// Check if application is initialized
    bool isInitialized() const { return _appInitialized.load(); }

signals:
    /// Emitted when language changes
    void languageChanged(const QLocale &locale);

    /// Emitted when settings are upgraded
    void settingsUpgraded();

public slots:
    /// Show vehicle configuration window
    void showVehicleConfig();

    /// Attempt to close the main window
    void qmlAttemptWindowClose();

    /// Show non-modal vehicle message to the user
    void showCriticalVehicleMessage(const QString &message);

    /// Show modal application message to the user
    void showAppMessage(const QString &message, const QString &title = QString());

    /// Show modal application message to the user about the need for a reboot.
    /// Multiple messages will be suppressed if they occur one after the other.
    void showRebootAppMessage(const QString &message, const QString &title = QString());

private slots:
    void _missingParamsDisplay();
    void _qgcCurrentStableVersionDownloadComplete(const QString &remoteFile, const QString &localFile, const QString &errorMsg);
    void _showDelayedAppMessages();

private:
    /// Parse version string into QVersionNumber
    static QVersionNumber _parseVersionText(const QString &versionString);

    /// Event filter for signal compression
    bool eventFilter(QObject *watched, QEvent *event) override;

    /// Initialize video subsystem
    void _initVideo();

    /// Initialize the application for normal application boot. Or in other words we are not going to run unit tests.
    void _initForNormalAppBoot();

    /// Get root QML object
    QObject *_rootQmlObject();

    /// Check for newer version availability
    void _checkForNewVersion();

    // Command line flags (const after construction)
    const bool _runningUnitTests;
    const bool _simpleBootTest;
    const bool _fakeMobile;    ///< true: Fake ui into displaying mobile interface
    const bool _logOutput;     ///< true: Log Qt debug output to file
    const quint8 _systemId;    ///< MAVLink system ID, 0 means not set

    // Missing parameters handling
    static constexpr int _missingParamsDelayedDisplayTimerTimeout = 1000;   ///< Timeout to wait for next missing fact to come in before display
    QTimer _missingParamsDelayedDisplayTimer;                               ///< Timer use to delay missing fact display
    QList<QPair<int,QString>> _missingParams;                               ///< List of missing parameter component id:name
    mutable QMutex _missingParamsMutex;                                     ///< Protects _missingParams

    // Core components
    QQmlApplicationEngine *_qmlAppEngine = nullptr;
    QPointer<QQuickWindow> _mainRootWindow;  ///< QPointer for safety

    // Version tracking
    bool _settingsUpgraded = false;    ///< true: Settings format has been upgrade to new version
    QVersionNumber _appVersion;        ///< Current application version

    // Translation
    std::unique_ptr<QTranslator> _qgcTranslatorSourceCode;  ///< translations for source code C++/Qml
    std::unique_ptr<QTranslator> _qgcTranslatorQtLibs;      ///< translations for Qt libraries
    QLocale _currentLocale;  ///< Current application locale (needed since we can't safely change default after threads start)
    mutable QMutex _localeMutex;  ///< Protects _currentLocale access

    // State tracking
    bool _showErrorsInToolbar = false;
    std::atomic<bool> _appInitialized{false};  ///< Set to true after init() completes (thread-safe)
    QElapsedTimer _msecsElapsedTime;
    bool _videoManagerInitialized = false;

    // Delayed messages
    QList<QPair<QString /* title */, QString /* message */>> _delayedAppMessages;
    mutable QMutex _delayedMessagesMutex;  ///< Protects _delayedAppMessages

    // Compressed signals handling
    class CompressedSignalList
    {
    public:
        CompressedSignalList();
        ~CompressedSignalList();

        void add(const QMetaMethod &method);
        void remove(const QMetaMethod &method);
        bool contains(const QMetaObject *metaObject, int signalIndex) const;

    private:
        /// Returns a signal index that is can be compared to QMetaCallEvent.signalId
        static int _signalIndex(const QMetaMethod &method);

        QMap<const QMetaObject*, QSet<int>> _signalMap;
        mutable QMutex _mutex;  ///< Thread safety

        Q_DISABLE_COPY(CompressedSignalList)
    };

    CompressedSignalList _compressedSignals;

    // Event compression for Qt 6.8+ compatibility
    struct CompressedEventInfo {
        QObject *sender;
        int signalId;
        int methodId;
        qint64 timestamp;
    };

    QMap<QObject*, QList<CompressedEventInfo>> _pendingCompressedEvents;
    mutable QMutex _compressionMutex;

    // Settings keys
    static constexpr const char *_settingsVersionKey = "SettingsVersion";
    static constexpr const char *_deleteAllSettingsKey = "DeleteAllSettingsNextBoot";
    static constexpr const char *_qgcImageProviderId = "QGCImages";

    // Prevent copying
    Q_DISABLE_COPY(QGCApplication)
};
