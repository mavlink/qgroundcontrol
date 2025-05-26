/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QTranslator>

#include <QtWidgets/QApplication>

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

/// The main application and management class.
/// Needs QApplication base to support QtCharts module.
/// TODO: Use QtGraphs to convert to QGuiApplication
class QGCApplication : public QApplication
{
    Q_OBJECT

    /// Unit Test have access to creating and destroying singletons
    friend class UnitTest;
public:
    QGCApplication(int &argc, char *argv[], bool unitTesting, bool simpleBootTest);
    ~QGCApplication();

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

    void setLanguage();
    QQuickWindow *mainRootWindow();
    uint64_t msecsSinceBoot() const { return _msecsElapsedTime.elapsed(); }
    QString numberToString(quint64 number);
    QString bigSizeToString(quint64 size);
    QString bigSizeMBToString(quint64 size_MB);

    /// Registers the signal such that only the last duplicate signal added is left in the queue.
    void addCompressedSignal(const QMetaMethod &method);

    void removeCompressedSignal(const QMetaMethod &method);

    bool event(QEvent *e) final;

    static QString cachedParameterMetaDataFile();
    static QString cachedAirframeMetaDataFile();

public:
    /// Perform initialize which is common to both normal application running and unit tests.
    void init();
    void shutdown();

    /// Although public, these methods are internal and should only be called by UnitTest code
    QQmlApplicationEngine *qmlAppEngine() const { return _qmlAppEngine; }

signals:
    void languageChanged(const QLocale locale);

public slots:
    void showVehicleConfig();

    void qmlAttemptWindowClose();

    /// Get current language
    QLocale getCurrentLanguage() const { return _locale; }

    /// Show non-modal vehicle message to the user
    void showCriticalVehicleMessage(const QString &message);

    /// Show modal application message to the user
    void showAppMessage(const QString &message, const QString &title = QString());

    /// Show modal application message to the user about the need for a reboot. Multiple messages will be supressed if they occur
    /// one after the other.
    void showRebootAppMessage(const QString &message, const QString &title = QString());

    QGCImageProvider *qgcImageProvider();

private slots:
    /// Called when the delay timer fires to show the missing parameters warning
    void _missingParamsDisplay();
    void _qgcCurrentStableVersionDownloadComplete(const QString &remoteFile, const QString &localFile, const QString &errorMsg);
    static bool _parseVersionText(const QString &versionString, int &majorVersion, int &minorVersion, int &buildVersion);
    void _showDelayedAppMessages();

private:
    bool compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents) final;

    void _initVideo();
    
    /// Initialize the application for normal application boot. Or in other words we are not going to run unit tests.
    void _initForNormalAppBoot();

    QObject *_rootQmlObject();
    void _checkForNewVersion();

    bool _runningUnitTests = false;
    bool _simpleBootTest = false; 
    static constexpr int _missingParamsDelayedDisplayTimerTimeout = 1000;   ///< Timeout to wait for next missing fact to come in before display
    QTimer _missingParamsDelayedDisplayTimer;                               ///< Timer use to delay missing fact display
    QList<QPair<int,QString>> _missingParams;                               ///< List of missing parameter component id:name

    QQmlApplicationEngine *_qmlAppEngine = nullptr;
    bool _logOutput = false;    ///< true: Log Qt debug output to file
    bool _fakeMobile = false;    ///< true: Fake ui into displaying mobile interface
    bool _settingsUpgraded = false;    ///< true: Settings format has been upgrade to new version
    int _majorVersion = 0;
    int _minorVersion = 0;
    int _buildVersion = 0;
    QQuickWindow *_mainRootWindow = nullptr;
    QTranslator _qgcTranslatorSourceCode;           ///< translations for source code C++/Qml
    QTranslator _qgcTranslatorQtLibs;               ///< tranlsations for Qt libraries
    QLocale _locale;
    bool _error = false;
    bool _showErrorsInToolbar = false;
    QElapsedTimer _msecsElapsedTime;
    bool _videoManagerInitialized = false;

    QList<QPair<QString /* title */, QString /* message */>> _delayedAppMessages;

    class CompressedSignalList
    {
    public:
        CompressedSignalList() {}
        void add(const QMetaMethod &method);
        void remove(const QMetaMethod &method);
        bool contains(const QMetaObject *metaObject, int signalIndex);

    private:
        /// Returns a signal index that is can be compared to QMetaCallEvent.signalId
        static int _signalIndex(const QMetaMethod &method);

        QMap<const QMetaObject*, QSet<int>> _signalMap;

        Q_DISABLE_COPY(CompressedSignalList)
    };

    CompressedSignalList _compressedSignals;

    const QString _settingsVersionKey = QStringLiteral("SettingsVersion"); ///< Settings key which hold settings version
    static constexpr const char *_deleteAllSettingsKey = "DeleteAllSettingsNextBoot"; ///< If this settings key is set on boot, all settings will be deleted

    const QString _qgcImageProviderId = QStringLiteral("QGCImages");
};
