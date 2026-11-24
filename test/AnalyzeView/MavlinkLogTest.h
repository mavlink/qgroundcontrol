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
    void _bootLogDetectionSaveDisabled_test_data();
    void _bootLogDetectionSaveDisabled_test();
    void _bootLogDetectionNamedForStartTime_test();
    void _bootLogDetectionNamedForModifiedTime_test();
    void _bootLogDetectionSavePathError_test_data();
    void _bootLogDetectionSavePathError_test();
    void _connectLogNoArm_test();
    void _connectLogArm_test();
    void _deleteTempLogFiles_test();

signals:
    void checkForLostLogFiles();

private:
    /// @param baseName Temp file name without extension. If empty, a unique name that holds no start time is generated.
    /// @param filePath If not null, receives the full path of the created file.
    void _createTempLogFile(bool zeroLength, const QString& baseName = QString(), QString* filePath = nullptr);
    void _connectLogWorker(bool arm);

    static constexpr const char* _untimedTempLogPrefix = "FlightData";            ///< Temp log name with no start time
    static constexpr const char* _logFileExtension = "mavlink";                   ///< Extension for log files
};
