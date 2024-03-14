/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Test for mavlink log collection
///
///     @author Don Gagne <don@thegagnes.com>

#pragma once

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

