#include "MAVLinkLogManagerTest.h"
#include "TestHelpers.h"
#include "QtTestExtensions.h"
#include "MAVLinkLogManager.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// Setup and Helpers
// ============================================================================

void MAVLinkLogManagerTest::cleanup()
{
    _vehicle = nullptr;
    _disconnectMockLink();
    UnitTest::cleanup();
}

MAVLinkLogManager *MAVLinkLogManagerTest::_createManager()
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager *const vehicleMgr = MultiVehicleManager::instance();
    _vehicle = vehicleMgr->activeVehicle();
    if (!_vehicle) {
        return nullptr;
    }

    return new MAVLinkLogManager(_vehicle, this);
}

// ============================================================================
// Initialization Tests
// ============================================================================

void MAVLinkLogManagerTest::_testInitialization()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);
    VERIFY_NOT_NULL(manager->logFiles());
}

void MAVLinkLogManagerTest::_testDefaultPropertyValues()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // Default URL should be PX4 logs
    QCOMPARE_EQ(manager->uploadURL(), QStringLiteral("https://logs.px4.io/upload"));

    // Default description
    QCOMPARE_EQ(manager->description(), QStringLiteral("QGroundControl Session"));

    // Default boolean settings
    QCOMPARE_EQ(manager->enableAutoUpload(), true);
    QCOMPARE_EQ(manager->enableAutoStart(), false);
    QCOMPARE_EQ(manager->deleteAfterUpload(), false);
    QCOMPARE_EQ(manager->publicLog(), true);

    // Default states
    QCOMPARE_EQ(manager->uploading(), false);
    QCOMPARE_EQ(manager->logRunning(), false);

    // Default wind speed (-1 means not set)
    QCOMPARE_EQ(manager->windSpeed(), -1);

    // Default rating
    QCOMPARE_EQ(manager->rating(), QStringLiteral("notset"));
}

// ============================================================================
// Property Setter Tests
// ============================================================================

void MAVLinkLogManagerTest::_testSetEmailAddress()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    QSignalSpy spy(manager, &MAVLinkLogManager::emailAddressChanged);
    QGC_VERIFY_SPY_VALID(spy);

    const QString testEmail = QStringLiteral("test@example.com");
    manager->setEmailAddress(testEmail);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->emailAddress(), testEmail);

    // Setting same value should not emit signal
    manager->setEmailAddress(testEmail);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testSetDescription()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    QSignalSpy spy(manager, &MAVLinkLogManager::descriptionChanged);
    QGC_VERIFY_SPY_VALID(spy);

    const QString testDesc = QStringLiteral("Test Flight Session");
    manager->setDescription(testDesc);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->description(), testDesc);

    // Setting same value should not emit signal
    manager->setDescription(testDesc);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testSetUploadURL()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    QSignalSpy spy(manager, &MAVLinkLogManager::uploadURLChanged);
    QGC_VERIFY_SPY_VALID(spy);

    const QString testURL = QStringLiteral("https://custom.logs.server/upload");
    manager->setUploadURL(testURL);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->uploadURL(), testURL);

    // Setting same value should not emit signal
    manager->setUploadURL(testURL);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testSetUploadURLEmpty()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // First set to a custom URL
    manager->setUploadURL(QStringLiteral("https://custom.server/upload"));

    QSignalSpy spy(manager, &MAVLinkLogManager::uploadURLChanged);
    QGC_VERIFY_SPY_VALID(spy);

    // Setting empty URL should revert to default
    manager->setUploadURL(QString());

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->uploadURL(), QStringLiteral("https://logs.px4.io/upload"));
}

void MAVLinkLogManagerTest::_testSetFeedback()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    QSignalSpy spy(manager, &MAVLinkLogManager::feedbackChanged);
    QGC_VERIFY_SPY_VALID(spy);

    const QString testFeedback = QStringLiteral("Good flight, stable hover");
    manager->setFeedback(testFeedback);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->feedback(), testFeedback);

    // Setting same value should not emit signal
    manager->setFeedback(testFeedback);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testSetVideoURL()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    QSignalSpy spy(manager, &MAVLinkLogManager::videoURLChanged);
    QGC_VERIFY_SPY_VALID(spy);

    const QString testURL = QStringLiteral("https://youtube.com/watch?v=test123");
    manager->setVideoURL(testURL);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->videoURL(), testURL);

    // Setting same value should not emit signal
    manager->setVideoURL(testURL);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testSetEnableAutoUpload()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // Default is true
    QCOMPARE_EQ(manager->enableAutoUpload(), true);

    QSignalSpy spy(manager, &MAVLinkLogManager::enableAutoUploadChanged);
    QGC_VERIFY_SPY_VALID(spy);

    manager->setEnableAutoUpload(false);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->enableAutoUpload(), false);

    // Setting same value should not emit signal
    manager->setEnableAutoUpload(false);
    QCOMPARE_EQ(spy.count(), 1);

    // Toggle back
    manager->setEnableAutoUpload(true);
    QCOMPARE_EQ(spy.count(), 2);
    QCOMPARE_EQ(manager->enableAutoUpload(), true);
}

void MAVLinkLogManagerTest::_testSetEnableAutoStart()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // Default is false
    QCOMPARE_EQ(manager->enableAutoStart(), false);

    QSignalSpy spy(manager, &MAVLinkLogManager::enableAutoStartChanged);
    QGC_VERIFY_SPY_VALID(spy);

    manager->setEnableAutoStart(true);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->enableAutoStart(), true);

    // Setting same value should not emit signal
    manager->setEnableAutoStart(true);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testSetDeleteAfterUpload()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // Default is false
    QCOMPARE_EQ(manager->deleteAfterUpload(), false);

    QSignalSpy spy(manager, &MAVLinkLogManager::deleteAfterUploadChanged);
    QGC_VERIFY_SPY_VALID(spy);

    manager->setDeleteAfterUpload(true);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->deleteAfterUpload(), true);

    // Setting same value should not emit signal
    manager->setDeleteAfterUpload(true);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testSetPublicLog()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // Default is true
    QCOMPARE_EQ(manager->publicLog(), true);

    QSignalSpy spy(manager, &MAVLinkLogManager::publicLogChanged);
    QGC_VERIFY_SPY_VALID(spy);

    manager->setPublicLog(false);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->publicLog(), false);

    // Setting same value should not emit signal
    manager->setPublicLog(false);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testSetWindSpeed()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // Default is -1
    QCOMPARE_EQ(manager->windSpeed(), -1);

    QSignalSpy spy(manager, &MAVLinkLogManager::windSpeedChanged);
    QGC_VERIFY_SPY_VALID(spy);

    manager->setWindSpeed(15);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->windSpeed(), 15);

    // Setting same value should not emit signal
    manager->setWindSpeed(15);
    QCOMPARE_EQ(spy.count(), 1);

    // Set different value
    manager->setWindSpeed(25);
    QCOMPARE_EQ(spy.count(), 2);
    QCOMPARE_EQ(manager->windSpeed(), 25);
}

void MAVLinkLogManagerTest::_testSetRating()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // Default is "notset"
    QCOMPARE_EQ(manager->rating(), QStringLiteral("notset"));

    QSignalSpy spy(manager, &MAVLinkLogManager::ratingChanged);
    QGC_VERIFY_SPY_VALID(spy);

    manager->setRating(QStringLiteral("good"));

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(manager->rating(), QStringLiteral("good"));

    // Setting same value should not emit signal
    manager->setRating(QStringLiteral("good"));
    QCOMPARE_EQ(spy.count(), 1);
}

// ============================================================================
// State Tests
// ============================================================================

void MAVLinkLogManagerTest::_testLogRunningInitialState()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    QCOMPARE_EQ(manager->logRunning(), false);
    QCOMPARE_EQ(manager->uploading(), false);
}

void MAVLinkLogManagerTest::_testCanStartLog()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // canStartLog depends on vehicle type and whether logging was denied
    // For PX4 firmware, it should be true initially
    if (_vehicle && _vehicle->px4Firmware()) {
        QCOMPARE_EQ(manager->canStartLog(), true);
    }
}

void MAVLinkLogManagerTest::_testLogFilesModel()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    QmlObjectListModel *const logFiles = manager->logFiles();
    VERIFY_NOT_NULL(logFiles);

    // Initially may have existing log files from previous sessions
    // Just verify the model is valid and accessible
    QCOMPARE_GE(logFiles->count(), 0);
}

// ============================================================================
// MAVLinkLogFiles Tests
// ============================================================================

void MAVLinkLogManagerTest::_testLogFilesInitialState()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    // Create a log file object for testing
    MAVLinkLogFiles logFile(manager, QStringLiteral("/tmp/test-log.ulg"), true);

    // New file should have default state
    QCOMPARE_EQ(logFile.selected(), false);
    QCOMPARE_EQ(logFile.uploaded(), false);
    QCOMPARE_EQ(logFile.uploading(), false);
    QCOMPARE_EQ(logFile.writing(), false);
    QCOMPARE_EQ(logFile.progress(), 0.0);
    QCOMPARE_EQ(logFile.size(), static_cast<quint32>(0));
    QCOMPARE_EQ(logFile.name(), QStringLiteral("test-log"));
}

void MAVLinkLogManagerTest::_testLogFilesSetSelected()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    MAVLinkLogFiles logFile(manager, QStringLiteral("/tmp/test.ulg"), true);

    QSignalSpy spy(&logFile, &MAVLinkLogFiles::selectedChanged);
    QGC_VERIFY_SPY_VALID(spy);

    logFile.setSelected(true);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(logFile.selected(), true);

    // Setting same value should not emit signal
    logFile.setSelected(true);
    QCOMPARE_EQ(spy.count(), 1);

    // Toggle back
    logFile.setSelected(false);
    QCOMPARE_EQ(spy.count(), 2);
    QCOMPARE_EQ(logFile.selected(), false);
}

void MAVLinkLogManagerTest::_testLogFilesSetUploading()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    MAVLinkLogFiles logFile(manager, QStringLiteral("/tmp/test.ulg"), true);

    QSignalSpy spy(&logFile, &MAVLinkLogFiles::uploadingChanged);
    QGC_VERIFY_SPY_VALID(spy);

    logFile.setUploading(true);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(logFile.uploading(), true);

    // Setting same value should not emit signal
    logFile.setUploading(true);
    QCOMPARE_EQ(spy.count(), 1);

    // Toggle back
    logFile.setUploading(false);
    QCOMPARE_EQ(spy.count(), 2);
    QCOMPARE_EQ(logFile.uploading(), false);
}

void MAVLinkLogManagerTest::_testLogFilesSetUploaded()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    MAVLinkLogFiles logFile(manager, QStringLiteral("/tmp/test.ulg"), true);

    QSignalSpy spy(&logFile, &MAVLinkLogFiles::uploadedChanged);
    QGC_VERIFY_SPY_VALID(spy);

    logFile.setUploaded(true);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(logFile.uploaded(), true);

    // Setting same value should not emit signal
    logFile.setUploaded(true);
    QCOMPARE_EQ(spy.count(), 1);
}

void MAVLinkLogManagerTest::_testLogFilesSetWriting()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    MAVLinkLogFiles logFile(manager, QStringLiteral("/tmp/test.ulg"), true);

    QSignalSpy spy(&logFile, &MAVLinkLogFiles::writingChanged);
    QGC_VERIFY_SPY_VALID(spy);

    logFile.setWriting(true);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(logFile.writing(), true);

    // Setting same value should not emit signal
    logFile.setWriting(true);
    QCOMPARE_EQ(spy.count(), 1);

    // Toggle back
    logFile.setWriting(false);
    QCOMPARE_EQ(spy.count(), 2);
    QCOMPARE_EQ(logFile.writing(), false);
}

void MAVLinkLogManagerTest::_testLogFilesSetProgress()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    MAVLinkLogFiles logFile(manager, QStringLiteral("/tmp/test.ulg"), true);

    QSignalSpy spy(&logFile, &MAVLinkLogFiles::progressChanged);
    QGC_VERIFY_SPY_VALID(spy);

    logFile.setProgress(0.5);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(logFile.progress(), 0.5);

    // Setting same value should not emit signal
    logFile.setProgress(0.5);
    QCOMPARE_EQ(spy.count(), 1);

    // Set different value
    logFile.setProgress(1.0);
    QCOMPARE_EQ(spy.count(), 2);
    QCOMPARE_EQ(logFile.progress(), 1.0);
}

void MAVLinkLogManagerTest::_testLogFilesSetSize()
{
    MAVLinkLogManager *const manager = _createManager();
    VERIFY_NOT_NULL(manager);

    MAVLinkLogFiles logFile(manager, QStringLiteral("/tmp/test.ulg"), true);

    QSignalSpy spy(&logFile, &MAVLinkLogFiles::sizeChanged);
    QGC_VERIFY_SPY_VALID(spy);

    logFile.setSize(1024);

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(logFile.size(), static_cast<quint32>(1024));

    // Setting same value should not emit signal
    logFile.setSize(1024);
    QCOMPARE_EQ(spy.count(), 1);

    // Set different value
    logFile.setSize(2048);
    QCOMPARE_EQ(spy.count(), 2);
    QCOMPARE_EQ(logFile.size(), static_cast<quint32>(2048));
}
