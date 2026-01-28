#pragma once

#include "TestFixtures.h"
#include "MockLinkFTP.h"

/// Tests for FTPManager functionality.
/// Inherits from FTPTest which provides a vehicle connected without initial connect sequence.
class FTPManagerTest : public FTPTest
{
    Q_OBJECT

public:
    FTPManagerTest() = default;

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

private:
    void _performSizeBasedTestCases();
    void _performTestCases();

    struct TestCase_t {
        const char* file;
    };

    void _testCaseWorker(const TestCase_t& testCase);
    void _sizeTestCaseWorker(int fileSize);
    void _verifyFileSizeAndDelete(const QString& filename, int expectedSize);

    /// Helper for listDirectory error mode tests
    /// @param errorMode MockLinkFTP error mode to test
    /// @param expectSuccess Whether the operation should succeed
    /// @param expectedCount Expected number of entries (only if expectSuccess is true)
    void _testListDirectoryWithErrorMode(MockLinkFTP::ErrorMode_t errorMode, bool expectSuccess, int expectedCount = 0);

    static const TestCase_t _rgTestCases[];
};
