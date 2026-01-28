#include "FTPManagerTest.h"
#include "TestHelpers.h"
#include "FTPManager.h"
#include "MockLinkFTP.h"

#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryFile>
#include <QtCore/QFile>
#include <QtTest/QTest>

// ============================================================================
// Test Case Data
// ============================================================================

const FTPManagerTest::TestCase_t FTPManagerTest::_rgTestCases[] = {
    { "/general.json" },
};

// ============================================================================
// Download Tests
// ============================================================================

void FTPManagerTest::_testCaseWorker(const TestCase_t& testCase)
{
    VERIFY_NOT_NULL(ftpManager());

    TestHelpers::SignalCapture capture(ftpManager(), &FTPManager::downloadComplete, FTPManager::kTestOperationMaxWaitMs);

    ftpManager()->download(MAV_COMP_ID_AUTOPILOT1, testCase.file, QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    QVERIFY(capture.wait());
    QCOMPARE_EQ(capture.count(), 1);
    qDebug() << "Downloaded:" << capture.arg<QString>(0);
    QGC_VERIFY_EMPTY(capture.arg<QString>(1));  // No error
}

void FTPManagerTest::_sizeTestCaseWorker(int fileSize)
{
    VERIFY_NOT_NULL(ftpManager());
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);

    TestHelpers::SignalCapture capture(ftpManager(), &FTPManager::downloadComplete, FTPManager::kTestOperationMaxWaitMs);

    ftpManager()->download(MAV_COMP_ID_AUTOPILOT1, filename, QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    QVERIFY(capture.wait());
    QCOMPARE_EQ(capture.count(), 1);
    QGC_VERIFY_EMPTY(capture.arg<QString>(1));  // No error

    _verifyFileSizeAndDelete(capture.arg<QString>(0), fileSize);
}

void FTPManagerTest::_performSizeBasedTestCases()
{
    // Test various boundary conditions on file sizes with respect to buffer sizes
    const QList<int> rgSizeTestCases = {
        // File fits one Read Ack packet, partially filling data
        static_cast<int>(sizeof(((MavlinkFTP::Request*)0)->data) - 1),
        // File fits one Read Ack packet, exactly filling all data
        static_cast<int>(sizeof(((MavlinkFTP::Request*)0)->data)),
        // File is larger than a single Read Ack packets, requires multiple Reads
        static_cast<int>(sizeof(((MavlinkFTP::Request*)0)->data) + 1),
        // File is large enough to require multiple bursts
        3 * 1024,
    };

    for (int fileSize : rgSizeTestCases) {
        _sizeTestCaseWorker(fileSize);
    }
}

void FTPManagerTest::_performTestCases()
{
    TestHelpers::runTestCases(_rgTestCases, [this](const TestCase_t& testCase) {
        _testCaseWorker(testCase);
    });
}

void FTPManagerTest::_testLostPackets()
{
    VERIFY_NOT_NULL(ftpManager());
    constexpr int fileSize = 4 * 1024;
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);

    TestHelpers::SignalCapture capture(ftpManager(), &FTPManager::downloadComplete, FTPManager::kTestOperationMaxWaitMs);

    mockLink()->mockLinkFTP()->enableRandromDrops(true);
    ftpManager()->download(MAV_COMP_ID_AUTOPILOT1, filename, QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    QVERIFY(capture.wait());
    QCOMPARE_EQ(capture.count(), 1);
    QGC_VERIFY_EMPTY(capture.arg<QString>(1));  // No error

    _verifyFileSizeAndDelete(capture.arg<QString>(0), fileSize);
}

void FTPManagerTest::_verifyFileSizeAndDelete(const QString& filename, int expectedSize)
{
    VERIFY_FILE_EXISTS(filename);
    QFileInfo fileInfo(filename);
    QCOMPARE_EQ(fileInfo.size(), static_cast<qint64>(expectedSize));

    QFile file(filename);
    QVERIFY(file.open(QFile::ReadOnly));
    for (int i = 0; i < expectedSize; i++) {
        QByteArray bytes = file.read(1);
        QCOMPARE_EQ(bytes[0], static_cast<char>(i % 255));
    }
    file.close();
    file.remove();
}

// ============================================================================
// List Directory Tests
// ============================================================================

void FTPManagerTest::_testListDirectoryWithErrorMode(MockLinkFTP::ErrorMode_t errorMode, bool expectSuccess, int expectedCount)
{
    VERIFY_NOT_NULL(ftpManager());

    mockLink()->mockLinkFTP()->setErrorMode(errorMode);

    TestHelpers::SignalCapture capture(ftpManager(), &FTPManager::listDirectoryComplete, FTPManager::kTestOperationMaxWaitMs);

    ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");

    QVERIFY(capture.wait());
    QCOMPARE_EQ(capture.count(), 1);

    if (expectSuccess) {
        qDebug() << "Directory listing:" << capture.arg<QStringList>(0);
        QCOMPARE_EQ(capture.arg<QStringList>(0).count(), expectedCount);
    } else {
        QGC_VERIFY_EMPTY(capture.arg<QStringList>(0));
        QGC_VERIFY_NOT_EMPTY(capture.arg<QString>(1));  // Error message
    }
}

void FTPManagerTest::_testListDirectory()
{
    _testListDirectoryWithErrorMode(MockLinkFTP::errModeNoSecondResponseAllowRetry, true, 6);
}

void FTPManagerTest::_testListDirectoryNoResponse()
{
    _testListDirectoryWithErrorMode(MockLinkFTP::errModeNoResponse, false);
}

void FTPManagerTest::_testListDirectoryNakResponse()
{
    _testListDirectoryWithErrorMode(MockLinkFTP::errModeNakResponse, false);
}

void FTPManagerTest::_testListDirectoryNoSecondResponse()
{
    _testListDirectoryWithErrorMode(MockLinkFTP::errModeNoSecondResponse, false);
}

void FTPManagerTest::_testListDirectoryNoSecondResponseAllowRetry()
{
    _testListDirectoryWithErrorMode(MockLinkFTP::errModeNoSecondResponseAllowRetry, true, 6);
}

void FTPManagerTest::_testListDirectoryNakSecondResponse()
{
    _testListDirectoryWithErrorMode(MockLinkFTP::errModeNakSecondResponse, false);
}

void FTPManagerTest::_testListDirectoryBadSequence()
{
    _testListDirectoryWithErrorMode(MockLinkFTP::errModeBadSequence, false);
}

void FTPManagerTest::_testListDirectoryCancel()
{
    VERIFY_NOT_NULL(ftpManager());

    TestHelpers::SignalCapture capture(ftpManager(), &FTPManager::listDirectoryComplete);

    QVERIFY(ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
    ftpManager()->cancelListDirectory();

    // listDirectoryComplete is signalled immediately on calling cancelListDirectory
    QCOMPARE_EQ(capture.count(), 1);
    QGC_VERIFY_EMPTY(capture.arg<QStringList>(0));
    QCOMPARE_EQ(capture.arg<QString>(1), QStringLiteral("Aborted"));
}

// ============================================================================
// Upload Tests
// ============================================================================

void FTPManagerTest::_testUpload()
{
    mockLink()->mockLinkFTP()->clearUploadedFiles();

    VERIFY_NOT_NULL(ftpManager());
    const QString remotePath(QStringLiteral("/mock/upload/test.bin"));
    const int chunkSize = static_cast<int>(sizeof(((MavlinkFTP::Request*)nullptr)->data));
    const int payloadSize = (chunkSize * 2) + 7;

    // Create test payload with recognizable pattern
    QByteArray payload(payloadSize, 0);
    for (int i = 0; i < payloadSize; ++i) {
        payload[i] = static_cast<char>((i % 251) + 1);
    }

    // Write payload to temp file
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QCOMPARE_EQ(tempFile.write(payload), static_cast<qint64>(payload.size()));
    tempFile.close();

    // Upload
    TestHelpers::SignalCapture capture(ftpManager(), &FTPManager::uploadComplete, FTPManager::kTestOperationMaxWaitMs);

    QVERIFY(ftpManager()->upload(MAV_COMP_ID_AUTOPILOT1, remotePath, tempFile.fileName()));

    QVERIFY(capture.wait());
    QCOMPARE_EQ(capture.count(), 1);
    QCOMPARE_EQ(capture.arg<QString>(0), remotePath);
    QGC_VERIFY_EMPTY(capture.arg<QString>(1));  // No error

    // Verify uploaded content
    QGC_VERIFY_CONTAINS(mockLink()->mockLinkFTP()->uploadedFiles(), remotePath);
    const QByteArray uploadedPayload = mockLink()->mockLinkFTP()->uploadedFileContents(remotePath);
    QCOMPARE_EQ(uploadedPayload, payload);

    mockLink()->mockLinkFTP()->clearUploadedFiles();
}
