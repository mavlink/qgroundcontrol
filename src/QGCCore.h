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

#include "MainWindow.h"
#include "UASManager.h"
#include "LinkManager.h"
#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#endif


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
    QGCApplication(int &argc, char* argv[]);
    ~QGCApplication();
    
    /// @brief Initialize the applicaation.
    /// @return false: init failed, app should exit
    bool init(void);
    
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
    
protected:
    void startLinkManager();
    
    /**
     * @brief Start the robot managing system
     *
     * The robot manager keeps track of the configured robots.
     **/
    void startUASManager();
    
private:
    MainWindow* _mainWindow;
    
    static const char* _settingsVersionKey;             ///< Settings key which hold settings version
    static const char* _deleteAllSettingsKey;           ///< If this settings key is set on boot, all settings will be deleted
    static const char* _savedFilesLocationKey;          ///< Settings key for user visible saved files location
    static const char* _promptFlightDataSave;           ///< Settings key to prompt for saving Flight Data Log for all flights
    
    static const char* _defaultSavedFileDirectoryName;      ///< Default name for user visible save file directory
    static const char* _savedFileMavlinkLogDirectoryName;   ///< Name of mavlink log subdirectory
    static const char* _savedFileParameterDirectoryName;    ///< Name of parameter subdirectory
};

/// @brief Returns the QGCApplication object singleton.
QGCApplication* qgcApp(void);

#endif
