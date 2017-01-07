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
 *   @brief Definition of main class
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef QGCAPPLICATION_H
#define QGCAPPLICATION_H

#include <QApplication>
#include <QTimer>
#include <QQmlApplicationEngine>

#include "LinkConfiguration.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "FlightMapSettings.h"
#include "FirmwarePluginManager.h"
#include "MultiVehicleManager.h"
#include "JoystickManager.h"
#include "GAudioOutput.h"
#include "UASMessageHandler.h"
#include "FactSystem.h"

#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif

// Work around circular header includes
class QGCSingleton;
class MainWindow;
class QGCToolbox;

/**
 * @brief The main application and management class.
 *
 * This class is started by the main method and provides
 * the central management unit of the groundstation application.
 *
 **/
class QGCApplication : public
#ifdef __mobile__
    QGuiApplication // Native Qml based application
#else
    QApplication    // QtWidget based application
#endif
{
    Q_OBJECT

public:
    QGCApplication(int &argc, char* argv[], bool unitTesting);
    ~QGCApplication();

    static const char* parameterFileExtension;
    static const char* missionFileExtension;
    static const char* fenceFileExtension;
    static const char* rallyPointFileExtension;
    static const char* telemetryFileExtension;

    /// @brief Sets the persistent flag to delete all settings the next time QGroundControl is started.
    void deleteAllSettingsNextBoot(void);

    /// @brief Clears the persistent flag to delete all settings the next time QGroundControl is started.
    void clearDeleteAllSettingsNextBoot(void);

    /// @return true: Prompt to save log file when vehicle goes away
    bool promptFlightDataSave(void);

    /// @return true: Prompt to save log file even if vehicle was not armed
    bool promptFlightDataSaveNotArmed(void);

    void setPromptFlightDataSave(bool promptForSave);
    void setPromptFlightDataSaveNotArmed(bool promptForSave);

    /// @brief Returns truee if unit test are being run
    bool runningUnitTests(void) { return _runningUnitTests; }

    /// @return true: dark ui style, false: light ui style
    bool styleIsDark(void) { return _styleIsDark; }

    /// Set the current UI style
    void setStyle(bool styleIsDark);

    /// Used to report a missing Parameter. Warning will be displayed to user. Method may be called
    /// multiple times.
    void reportMissingParameter(int componentId, const QString& name);

    /// Show a non-modal message to the user
    void showMessage(const QString& message);

    /// @return true: Fake ui into showing mobile interface
    bool fakeMobile(void) { return _fakeMobile; }

#ifdef QT_DEBUG
    bool testHighDPI(void) { return _testHighDPI; }
#endif

    // Still working on getting rid of this and using dependency injection instead for everything
    QGCToolbox* toolbox(void) { return _toolbox; }

    /// Do we have Bluetooth Support?
    bool isBluetoothAvailable() { return _bluetoothAvailable; }

    QGeoCoordinate lastKnownHomePosition(void) { return _lastKnownHomePosition; }
    void setLastKnownHomePosition(QGeoCoordinate& lastKnownHomePosition);

public slots:
    /// You can connect to this slot to show an information message box from a different thread.
    void informationMessageBoxOnMainThread(const QString& title, const QString& msg);

    /// You can connect to this slot to show a warning message box from a different thread.
    void warningMessageBoxOnMainThread(const QString& title, const QString& msg);

    /// You can connect to this slot to show a critical message box from a different thread.
    void criticalMessageBoxOnMainThread(const QString& title, const QString& msg);

    void showSetupView(void);

    void qmlAttemptWindowClose(void);

#ifndef __mobile__
    /// Save the specified Flight Data Log
    void saveTempFlightDataLogOnMainThread(QString tempLogfile);
#endif

signals:
    /// Signals that the style has changed
    ///     @param darkStyle true: dark style, false: light style
    void styleChanged(bool darkStyle);

    /// This is connected to MAVLinkProtocol::checkForLostLogFiles. We signal this to ourselves to call the slot
    /// on the MAVLinkProtocol thread;
    void checkForLostLogFiles(void);

public:
    // Although public, these methods are internal and should only be called by UnitTest code

    /// @brief Perform initialize which is common to both normal application running and unit tests.
    ///         Although public should only be called by main.
    void _initCommon(void);

    /// @brief Intialize the application for normal application boot. Or in other words we are not going to run
    ///         unit tests. Although public should only be called by main.
    bool _initForNormalAppBoot(void);

    /// @brief Intialize the application for normal application boot. Or in other words we are not going to run
    ///         unit tests. Although public should only be called by main.
    bool _initForUnitTests(void);

    static QGCApplication*  _app;   ///< Our own singleton. Should be reference directly by qgcApp

private slots:
    void _missingParamsDisplay(void);

private:
    void        _loadCurrentStyle   ();
    QObject*    _rootQmlObject      ();

#ifdef __mobile__
    QQmlApplicationEngine* _qmlAppEngine;
#endif

    bool _runningUnitTests; ///< true: running unit tests, false: normal app

    static const char*  _darkStyleFile;
    static const char*  _lightStyleFile;
    bool                _styleIsDark;                                       ///< true: dark style, false: light style
    static const int    _missingParamsDelayedDisplayTimerTimeout = 1000;    ///< Timeout to wait for next missing fact to come in before display
    QTimer              _missingParamsDelayedDisplayTimer;                  ///< Timer use to delay missing fact display
    QStringList         _missingParams;                                     ///< List of missing facts to be displayed
    bool				_fakeMobile;                                        ///< true: Fake ui into displaying mobile interface
    bool                _settingsUpgraded;                                  ///< true: Settings format has been upgrade to new version

#ifdef QT_DEBUG
    bool _testHighDPI;  ///< true: double fonts sizes for simulating high dpi devices
#endif

    QGCToolbox* _toolbox;

    bool _bluetoothAvailable;

    QGeoCoordinate _lastKnownHomePosition;    ///< Map position when all other sources fail

    static const char* _settingsVersionKey;             ///< Settings key which hold settings version
    static const char* _deleteAllSettingsKey;           ///< If this settings key is set on boot, all settings will be deleted
    static const char* _promptFlightDataSave;           ///< Settings key for promptFlightDataSave
    static const char* _promptFlightDataSaveNotArmed;   ///< Settings key for promptFlightDataSaveNotArmed
    static const char* _styleKey;                       ///< Settings key for UI style
    static const char* _lastKnownHomePositionLatKey;
    static const char* _lastKnownHomePositionLonKey;
    static const char* _lastKnownHomePositionAltKey;

    /// Unit Test have access to creating and destroying singletons
    friend class UnitTest;

};

/// @brief Returns the QGCApplication object singleton.
QGCApplication* qgcApp(void);

#endif
