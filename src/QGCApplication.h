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

#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif

// Work around circular header includes
class QGCSingleton;
class MainWindow;

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

    /// @brief All global singletons must be registered such that QGCApplication::destorySingletonsForUnitTest
    ///         can work correctly.
    void registerSingleton(QGCSingleton* singleton);
    
    /// @brief Creates non-ui based singletons for unit testing
    void createSingletonsForUnitTest(void) { _createSingletons(); }
    
    /// @brief Destroys all singletons. Used by unit test code to reset global state.
    void destroySingletonsForUnitTest(void);
    
    /// @brief Returns truee if unit test are being run
    bool runningUnitTests(void) { return _runningUnitTests; }
    
public:
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
    
private:
    void _createSingletons(void);
    
    static const char* _settingsVersionKey;             ///< Settings key which hold settings version
    static const char* _deleteAllSettingsKey;           ///< If this settings key is set on boot, all settings will be deleted
    static const char* _savedFilesLocationKey;          ///< Settings key for user visible saved files location
    static const char* _promptFlightDataSave;           ///< Settings key to prompt for saving Flight Data Log for all flights
    
    static const char* _defaultSavedFileDirectoryName;      ///< Default name for user visible save file directory
    static const char* _savedFileMavlinkLogDirectoryName;   ///< Name of mavlink log subdirectory
    static const char* _savedFileParameterDirectoryName;    ///< Name of parameter subdirectory
    
    QList<QGCSingleton*> _singletons;    ///< List of registered global singletons
    
    bool _runningUnitTests; ///< true: running unit tests, false: normal app
};

/// @brief Returns the QGCApplication object singleton.
QGCApplication* qgcApp(void);

#endif
