#pragma once

#include "UnitTest.h"

class FTPManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testLostPackets                               (void);
    void _testListDirectory                             (void);
    void _testListDirectoryNoResponse                   (void);
    void _testListDirectoryNakResponse                  (void);
    void _testListDirectoryNoSecondResponse             (void);
    void _testListDirectoryNoSecondResponseAllowRetry   (void);
    void _testListDirectoryNakSecondResponse            (void);
    void _testListDirectoryBadSequence                  (void);
    void _testListDirectoryCancel                       (void);
    void _testUpload                                     (void);

    // Overrides from UnitTest
    void cleanup(void) override;

private:
    void _performSizeBasedTestCases (void);
    void _performTestCases          (void);

    typedef struct {
        const char* file;
    } TestCase_t;

    void _testCaseWorker            (const TestCase_t& testCase);
    void _sizeTestCaseWorker        (int fileSize);
    void _verifyFileSizeAndDelete   (const QString& filename, int expectedSize);

    static const TestCase_t _rgTestCases[];
};
