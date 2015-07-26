/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @brief Test for mavlink log collection
///
///     @author Don Gagne <don@thegagnes.com>

#ifndef MAVLINKLOGTEST_H
#define MAVLINKLOGTEST_H

#include "UnitTest.h"

class MavlinkLogTest : public UnitTest
{
    Q_OBJECT
    
public:
    MavlinkLogTest(void);
    
private slots:
    void init(void);
    void cleanup(void);
    
    void _bootLogDetectionCancel_test(void);
    void _bootLogDetectionSave_test(void);
    void _bootLogDetectionZeroLength_test(void);
    void _connectLogNoArm_test(void);
    void _connectLogArm_test(void);
    void _deleteTempLogFiles_test(void);
    
signals:
    void checkForLostLogFiles(void);
    
private:
    void _createTempLogFile(bool zeroLength);
    void _connectLogWorker(bool arm);
    
    static const char* _tempLogFileTemplate;    ///< Template for temporary log file
    static const char* _logFileExtension;       ///< Extension for log files
    static const char* _saveLogFilename;        ///< Filename to save log files to
};

#endif
