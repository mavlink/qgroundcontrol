/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QEvent>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QTranslator>

// These private headers are require to implement the signal compress support below
#include <QtCore/private/qthread_p.h>
#include <QtCore/private/qobject_p.h>

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #define QGCAPPLICATION_CLASS QApplication
#else
    #include <SingleApplication>
    #define QGCAPPLICATION_CLASS SingleApplication
#endif

// Work around circular header includes
class QQmlApplicationEngine;
class QGCToolbox;
class QQuickWindow;
class QGCImageProvider;
class QGCApplication;

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<QGCApplication *>(QApplication::instance()))

#if defined(qGuiApp)
#undef qGuiApp
#endif
#define qGuiApp (static_cast<QGCApplication *>(QGuiApplication::instance()))

#define qgcApp() qApp

/**
 * @brief The main application and management class.
 *
 * This class is started by the main method and provides
 * the central management unit of the groundstation application.
 *
 * Needs QApplication base to support QtCharts module. This way
 * we avoid application crashing on 5.12 when using the module.
 *
 * Note: `lastWindowClosed` will be sent by MessageBox popups and other
 * dialogs, that are spawned in QML, when they are closed
**/

// TODO: Use QtGraphs to convert to QGuiApplication
class QGCApplication : public QGCAPPLICATION_CLASS
{
    Q_OBJECT
public:
    QGCApplication(int &argc, char* argv[], bool unitTesting);
    ~QGCApplication();

    /// @brief Sets the persistent flag to delete all settings the next time QGroundControl is started.
    void deleteAllSettingsNextBoot(void);

    /// @brief Clears the persistent flag to delete all settings the next time QGroundControl is started.
    void clearDeleteAllSettingsNextBoot(void);

    /// @brief Returns true if unit tests are being run
    bool runningUnitTests(void) const{ return _runningUnitTests; }

    /// @brief Returns true if Qt debug output should be logged to a file
    bool logOutput(void) const{ return _logOutput; }

    /// Used to report a missing Parameter. Warning will be displayed to user. Method may be called
    /// multiple times.
    void reportMissingParameter(int componentId, const QString& name);

    /// @return true: Fake ui into showing mobile interface
    bool fakeMobile(void) const { return _fakeMobile; }

    // Still working on getting rid of this and using dependency injection instead for everything
    QGCToolbox* toolbox(void) { return _toolbox; }

    void            setLanguage();
    QQuickWindow*   mainRootWindow();
    uint64_t        msecsSinceBoot(void) { return _msecsElapsedTime.elapsed(); }
    QString         numberToString(quint64 number);
    QString         bigSizeToString(quint64 size);
    QString         bigSizeMBToString(quint64 size_MB);

    /// Registers the signal such that only the last duplicate signal added is left in the queue.
    void addCompressedSignal(const QMetaMethod & method);

    void removeCompressedSignal(const QMetaMethod & method);

    bool event(QEvent *e) override;

    static QString cachedParameterMetaDataFile(void);
    static QString cachedAirframeMetaDataFile(void);

public slots:
    /// You can connect to this slot to show an information message box from a different thread.
    void informationMessageBoxOnMainThread(const QString& title, const QString& msg);

    /// You can connect to this slot to show a warning message box from a different thread.
    void warningMessageBoxOnMainThread(const QString& title, const QString& msg);

    /// You can connect to this slot to show a critical message box from a different thread.
    void criticalMessageBoxOnMainThread(const QString& title, const QString& msg);

    void showSetupView();

    void qmlAttemptWindowClose();

    /// Save the specified telemetry Log
    void saveTelemetryLogOnMainThread(const QString &tempLogfile);

    /// Check that the telemetry save path is set correctly
    void checkTelemetrySavePathOnMainThread();

    /// Get current language
    const QLocale getCurrentLanguage() { return _locale; }

    /// Show non-modal vehicle message to the user
    void showCriticalVehicleMessage(const QString& message);

    /// Show modal application message to the user
    void showAppMessage(const QString& message, const QString& title = QString());

    /// Show modal application message to the user about the need for a reboot. Multiple messages will be supressed if they occur
    /// one after the other.
    void showRebootAppMessage(const QString& message, const QString& title = QString());

    QGCImageProvider* qgcImageProvider();

signals:
    /// This is connected to MAVLinkProtocol::checkForLostLogFiles. We signal this to ourselves to call the slot
    /// on the MAVLinkProtocol thread;
    void checkForLostLogFiles   ();

    void languageChanged        (const QLocale locale);

public:
    /// @brief Perform initialize which is common to both normal application running and unit tests.
    void init();
    void shutdown();

    // Although public, these methods are internal and should only be called by UnitTest code
    QQmlApplicationEngine* qmlAppEngine() { return _qmlAppEngine; }

private slots:
    void _missingParamsDisplay                      (void);
    void _qgcCurrentStableVersionDownloadComplete   (QString remoteFile, QString localFile, QString errorMsg);
    bool _parseVersionText                          (const QString& versionString, int& majorVersion, int& minorVersion, int& buildVersion);
    void _showDelayedAppMessages                    (void);

private:
    /// @brief Initialize the application for normal application boot. Or in other words we are not going to run unit tests.
    void _initForNormalAppBoot();

    QObject* _rootQmlObject();
    void _checkForNewVersion();
    bool _checkTelemetrySavePath(bool useMessageBox);

    // Overrides from QApplication
    bool compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents) override;

    bool                        _runningUnitTests;                                  ///< true: running unit tests, false: normal app
    static const int            _missingParamsDelayedDisplayTimerTimeout = 1000;    ///< Timeout to wait for next missing fact to come in before display
    QTimer                      _missingParamsDelayedDisplayTimer;                  ///< Timer use to delay missing fact display
    QList<QPair<int,QString>>   _missingParams;                                     ///< List of missing parameter component id:name

    QQmlApplicationEngine* _qmlAppEngine        = nullptr;
    bool                _logOutput              = false;    ///< true: Log Qt debug output to file
    bool				_fakeMobile             = false;    ///< true: Fake ui into displaying mobile interface
    bool                _settingsUpgraded       = false;    ///< true: Settings format has been upgrade to new version
    int                 _majorVersion           = 0;
    int                 _minorVersion           = 0;
    int                 _buildVersion           = 0;
    QGCToolbox*         _toolbox                = nullptr;
    QQuickWindow*       _mainRootWindow         = nullptr;
    QTranslator         _qgcTranslatorSourceCode;           ///< translations for source code C++/Qml
    QTranslator         _qgcTranslatorQtLibs;               ///< tranlsations for Qt libraries
    QLocale             _locale;
    bool                _error                  = false;
    bool                _showErrorsInToolbar    = false;
    QElapsedTimer       _msecsElapsedTime;

    QList<QPair<QString /* title */, QString /* message */>> _delayedAppMessages;

    class CompressedSignalList {
        Q_DISABLE_COPY(CompressedSignalList)

    public:
        CompressedSignalList() {}

        void add        (const QMetaMethod & method);
        void remove     (const QMetaMethod & method);
        bool contains   (const QMetaObject * metaObject, int signalIndex);

    private:
        static int _signalIndex(const QMetaMethod & method);

        QMap<const QMetaObject*, QSet<int> > _signalMap;
    };

    CompressedSignalList _compressedSignals;

    const QString _settingsVersionKey = QStringLiteral("SettingsVersion"); ///< Settings key which hold settings version
    const QString _deleteAllSettingsKey = QStringLiteral("DeleteAllSettingsNextBoot"); ///< If this settings key is set on boot, all settings will be deleted

    const QString qgcImageProviderId = QStringLiteral("QGCImages");

    /// Unit Test have access to creating and destroying singletons
    friend class UnitTest;
};
