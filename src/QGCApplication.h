/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QApplication>
#include <QTimer>
#include <QQmlApplicationEngine>
#include <QElapsedTimer>

#include "LinkConfiguration.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "FlightMapSettings.h"
#include "FirmwarePluginManager.h"
#include "MultiVehicleManager.h"
#include "JoystickManager.h"
#include "AudioOutput.h"
#include "UASMessageHandler.h"
#include "FactSystem.h"
#include "GPSRTKFactGroup.h"

#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif

// Work around circular header includes
class QGCSingleton;
class QGCToolbox;
class QGCFileDownload;

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
class QGCApplication : public QApplication
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
    bool runningUnitTests(void) { return _runningUnitTests; }

    /// @brief Returns true if Qt debug output should be logged to a file
    bool logOutput(void) { return _logOutput; }

    /// Used to report a missing Parameter. Warning will be displayed to user. Method may be called
    /// multiple times.
    void reportMissingParameter(int componentId, const QString& name);

    /// Show a non-modal message to the user
    void showMessage(const QString& message);

    /// @return true: Fake ui into showing mobile interface
    bool fakeMobile(void) const { return _fakeMobile; }

    // Still working on getting rid of this and using dependency injection instead for everything
    QGCToolbox* toolbox(void) { return _toolbox; }

    /// Do we have Bluetooth Support?
    bool isBluetoothAvailable() { return _bluetoothAvailable; }

    /// Is Internet available?
    bool isInternetAvailable();

    FactGroup* gpsRtkFactGroup(void)  { return _gpsRtkFactGroup; }

    static QString cachedParameterMetaDataFile(void);
    static QString cachedAirframeMetaDataFile(void);

    void            setLanguage();
    QQuickItem*     mainRootWindow();

    uint64_t        msecsSinceBoot(void) { return _msecsElapsedTime.elapsed(); }

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
    void saveTelemetryLogOnMainThread(QString tempLogfile);

    /// Check that the telemetry save path is set correctly
    void checkTelemetrySavePathOnMainThread();

    /// Get current language
    const QLocale getCurrentLanguage() { return _locale; }

signals:
    /// This is connected to MAVLinkProtocol::checkForLostLogFiles. We signal this to ourselves to call the slot
    /// on the MAVLinkProtocol thread;
    void checkForLostLogFiles   ();

    void languageChanged        (const QLocale locale);

public:
    // Although public, these methods are internal and should only be called by UnitTest code

    /// @brief Perform initialize which is common to both normal application running and unit tests.
    ///         Although public should only be called by main.
    void _initCommon();

    /// @brief Initialize the application for normal application boot. Or in other words we are not going to run
    ///         unit tests. Although public should only be called by main.
    bool _initForNormalAppBoot();

    /// @brief Initialize the application for normal application boot. Or in other words we are not going to run
    ///         unit tests. Although public should only be called by main.
    bool _initForUnitTests();

    static QGCApplication*  _app;   ///< Our own singleton. Should be reference directly by qgcApp

    bool    isErrorState()  { return _error; }

public:
    // Although public, these methods are internal and should only be called by UnitTest code

    /// Shutdown the application object
    void _shutdown();

    bool _checkTelemetrySavePath(bool useMessageBox);

private slots:
    void _missingParamsDisplay(void);
    void _currentVersionDownloadFinished(QString remoteFile, QString localFile);
    void _currentVersionDownloadError(QString errorMsg);
    bool _parseVersionText(const QString& versionString, int& majorVersion, int& minorVersion, int& buildVersion);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _gpsSurveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active);
    void _gpsNumSatellites(int numSatellites);

private:
    QObject*    _rootQmlObject          ();
    void        _checkForNewVersion     ();
    void        _exitWithError          (QString errorMessage);


    bool                        _runningUnitTests;                                  ///< true: running unit tests, false: normal app
    static const int            _missingParamsDelayedDisplayTimerTimeout = 1000;    ///< Timeout to wait for next missing fact to come in before display
    QTimer                      _missingParamsDelayedDisplayTimer;                  ///< Timer use to delay missing fact display
    QList<QPair<int,QString>>   _missingParams;                                     ///< List of missing parameter component id:name

    QQmlApplicationEngine* _qmlAppEngine        = nullptr;
    bool                _logOutput              = false;                    ///< true: Log Qt debug output to file
    bool				_fakeMobile             = false;                    ///< true: Fake ui into displaying mobile interface
    bool                _settingsUpgraded       = false;                    ///< true: Settings format has been upgrade to new version
    int                 _majorVersion           = 0;
    int                 _minorVersion           = 0;
    int                 _buildVersion           = 0;
    QGCFileDownload*    _currentVersionDownload = nullptr;
    GPSRTKFactGroup*    _gpsRtkFactGroup        = nullptr;
    QGCToolbox*         _toolbox                = nullptr;
    QQuickItem*         _mainRootWindow         = nullptr;
    bool                _bluetoothAvailable     = false;
    QTranslator         _QGCTranslator;
    QTranslator         _QGCTranslatorQt;
    QLocale             _locale;
    bool                _error                  = false;
    QElapsedTimer       _msecsElapsedTime;

    static const char* _settingsVersionKey;             ///< Settings key which hold settings version
    static const char* _deleteAllSettingsKey;           ///< If this settings key is set on boot, all settings will be deleted

    /// Unit Test have access to creating and destroying singletons
    friend class UnitTest;

};

/// @brief Returns the QGCApplication object singleton.
QGCApplication* qgcApp(void);
