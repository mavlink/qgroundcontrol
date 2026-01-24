#pragma once

#include "UnitTest.h"

/// Unit test for MAVLink log file management functionality.
/// Tests the temp log file detection and cleanup APIs in MAVLinkProtocol.
class MavlinkLogTest : public UnitTest
{
    Q_OBJECT

public:
    MavlinkLogTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    void _zeroLengthLogFilesDeleted_test();
    void _deleteTempLogFiles_test();
    void _nonZeroLengthLogFileProcessed_test();

private:
    QString _createTempLogFile(bool zeroLength);
    void _cleanupTempLogFiles();
    int _countTempLogFiles();

    static constexpr const char* _tempLogFileTemplate = "FlightDataXXXXXX";
    static constexpr const char* _logFileExtension = "mavlink";
};
