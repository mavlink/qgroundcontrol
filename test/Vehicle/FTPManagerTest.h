#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class FTPManagerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testLostPackets();
    void _testListDirectory();
    void _testListDirectoryNoResponse();
    void _testListDirectoryNakResponse();
    void _testListDirectoryNoSecondResponse();
    void _testListDirectoryNoSecondResponseAllowRetry();
    void _testListDirectoryNakSecondResponse();
    void _testListDirectoryBadSequence();
    void _testListDirectoryCancel();
    void _testUpload();

    // Overrides from UnitTest
    void cleanup() override;

private:
    void _performSizeBasedTestCases();
    void _performTestCases();

    typedef struct
    {
        const char* file;
    } TestCase_t;

    void _testCaseWorker(const TestCase_t& testCase);
    void _sizeTestCaseWorker(int fileSize);
    void _verifyFileSizeAndDelete(const QString& filename, int expectedSize);

    static const TestCase_t _rgTestCases[];
};
