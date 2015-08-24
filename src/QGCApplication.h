/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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

#include "LinkConfiguration.h"

#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif

// Work around circular header includes
class QGCSingleton;
class MainWindow;
class MavManager;

/**
 * @brief The main application and management class.
 *
 * This class is started by the main method and provides
 * the central management unit of the groundstation application.
 *
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
    
    /// @brief Returns the location of user visible saved file associated with QGroundControl
    QString savedFilesLocation(void);
    
    /// @brief Sets the location of user visible saved file associated with QGroundControl
    void setSavedFilesLocation(QString& location);
    
    /// @brief Location to save and load parameter files from.
    QString savedParameterFilesLocation(void);
    
    /// @brief Location to save and load mavlink log files from
    QString mavlinkLogFilesLocation(void);
    
    /// @brief Validates that the specified location will work for the saved files location.
    bool validatePossibleSavedFilesLocation(QString& location);
    
    /// @brief Returns true is all mavlink connections should be logged
    bool promptFlightDataSave(void);
    
    /// @brief Sets the flag to log all mavlink connections
    void setPromptFlightDataSave(bool promptForSave);

    /// @brief Returns truee if unit test are being run
    bool runningUnitTests(void) { return _runningUnitTests; }
    
    /// @return true: dark ui style, false: light ui style
    bool styleIsDark(void) { return _styleIsDark; }
    
    /// Set the current UI style
    void setStyle(bool styleIsDark);
    
    /// Used to report a missing Parameter. Warning will be displayed to user. Method may be called
    /// multiple times.
    void reportMissingParameter(int componentId, const QString& name);

    /// When the singleton is created, it sets a pointer for subsequent use
    void setMavManager(MavManager* pMgr);

    /// MavManager accessor
    MavManager* getMavManager();
    
    /// Show a non-modal message to the user
    void showToolBarMessage(const QString& message);

	/// @return true: Fake ui into showing mobile interface
	bool fakeMobile(void) { return _fakeMobile; }
    
public slots:
    /// You can connect to this slot to show an information message box from a different thread.
    void informationMessageBoxOnMainThread(const QString& title, const QString& msg);
    
    /// You can connect to this slot to show a warning message box from a different thread.
    void warningMessageBoxOnMainThread(const QString& title, const QString& msg);
    
    /// You can connect to this slot to show a critical message box from a different thread.
    void criticalMessageBoxOnMainThread(const QString& title, const QString& msg);
    
    /// Save the specified Flight Data Log
    void saveTempFlightDataLogOnMainThread(QString tempLogfile);
    
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
    void _createSingletons(void);
    void _destroySingletons(void);
    void _loadCurrentStyle(void);
    
    static const char* _settingsVersionKey;             ///< Settings key which hold settings version
    static const char* _deleteAllSettingsKey;           ///< If this settings key is set on boot, all settings will be deleted
    static const char* _savedFilesLocationKey;          ///< Settings key for user visible saved files location
    static const char* _promptFlightDataSave;           ///< Settings key to prompt for saving Flight Data Log for all flights
    static const char* _styleKey;                       ///< Settings key for UI style
    
    static const char* _defaultSavedFileDirectoryName;      ///< Default name for user visible save file directory
    static const char* _savedFileMavlinkLogDirectoryName;   ///< Name of mavlink log subdirectory
    static const char* _savedFileParameterDirectoryName;    ///< Name of parameter subdirectory
    
    bool _runningUnitTests; ///< true: running unit tests, false: normal app
    
    static const char*  _darkStyleFile;
    static const char*  _lightStyleFile;
    bool                _styleIsDark;      ///< true: dark style, false: light style
    
    static const int    _missingParamsDelayedDisplayTimerTimeout = 1000;  ///< Timeout to wait for next missing fact to come in before display
    QTimer              _missingParamsDelayedDisplayTimer;                ///< Timer use to delay missing fact display
    QStringList         _missingParams;                                  ///< List of missing facts to be displayed
    MavManager*         _pMavManager;

	bool				_fakeMobile;	///< true: Fake ui into displaying mobile interface

    /// Unit Test have access to creating and destroying singletons
    friend class UnitTest;
};

/// @brief Returns the QGCApplication object singleton.
QGCApplication* qgcApp(void);

#endif
