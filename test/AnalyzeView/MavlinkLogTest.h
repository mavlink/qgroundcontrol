#pragma once

#include "BaseClasses/VehicleTest.h"

class MavlinkLogTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void _bootLogDetectionCancel_test();
    void _bootLogDetectionSave_test();
    void _bootLogDetectionZeroLength_test();
    void _connectLogNoArm_test();
    void _connectLogArm_test();
    void _deleteTempLogFiles_test();

signals:
    void checkForLostLogFiles();

private:
    void _createTempLogFile(bool zeroLength);
    void _connectLogWorker(bool arm);

    static constexpr const char* _tempLogFileTemplate = "FlightDataXXXXXX";       ///< Template for temporary log file
    static constexpr const char* _logFileExtension = "mavlink";                   ///< Extension for log files
};
