#include "MavlinkLogTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>

#include "AppSettings.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MavlinkSettings.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

namespace {
int tempMavlinkLogCount(const char* extension)
{
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    const QStringList logFiles(tmpDir.entryList(QStringList(QStringLiteral("*.%1").arg(extension)), QDir::Files));
    return logFiles.count();
}

int savedTelemetryLogCount()
{
    const QString telemetrySavePath = SettingsManager::instance()->appSettings()->telemetrySavePath();
    const QDir saveDir(telemetrySavePath);
    const QString filter = QStringLiteral("*.%1").arg(AppSettings::telemetryFileExtension);
    return saveDir.entryList(QStringList(filter), QDir::Files).count();
}

void removeSavedTelemetryLogs()
{
    const QString telemetrySavePath = SettingsManager::instance()->appSettings()->telemetrySavePath();
    QDir saveDir(telemetrySavePath);
    const QString filter = QStringLiteral("*.%1").arg(AppSettings::telemetryFileExtension);
    const QStringList logFiles = saveDir.entryList(QStringList(filter), QDir::Files);
    for (const QString& file : logFiles) {
        QVERIFY(saveDir.remove(file));
    }
}
}  // namespace

void MavlinkLogTest::init()
{
    UnitTest::init();
    MultiVehicleManager::instance()->init();
    LinkManager::instance()->setConnectionsAllowed();
    // Make sure temp directory is clear of mavlink logs
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    for (const QString& logFile : logFiles) {
        QVERIFY(tmpDir.remove(logFile));
    }

    const QString testRoot =
        QDir::tempPath() + QStringLiteral("/QGroundControl_unittest_MavlinkLogTest");
    QVERIFY(QDir().mkpath(testRoot));
    SettingsManager::instance()->appSettings()->savePath()->setRawValue(testRoot);
    SettingsManager::instance()->appSettings()->disableAllPersistence()->setRawValue(false);
    SettingsManager::instance()->mavlinkSettings()->telemetrySave()->setRawValue(true);
    SettingsManager::instance()->mavlinkSettings()->telemetrySaveNotArmed()->setRawValue(false);

    const QString telemetrySavePath = SettingsManager::instance()->appSettings()->telemetrySavePath();
    QVERIFY(!telemetrySavePath.isEmpty());
    QVERIFY(QDir().mkpath(telemetrySavePath));
    removeSavedTelemetryLogs();
}

void MavlinkLogTest::cleanup()
{
    _disconnectMockLink();
    // Make sure no left over logs in temp directory
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
    removeSavedTelemetryLogs();
    UnitTest::cleanup();
}

void MavlinkLogTest::_createTempLogFile(bool zeroLength)
{
    const QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString fileName = QStringLiteral("%1_%2.%3")
                                 .arg(_tempLogFileTemplate)
                                 .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))
                                 .arg(_logFileExtension);
    QFile tempLogFile(QDir(tempDirPath).filePath(fileName));
    QVERIFY(tempLogFile.open(QIODevice::WriteOnly));
    if (!zeroLength) {
        tempLogFile.write("foo");
    }
    tempLogFile.close();
}

void MavlinkLogTest::_bootLogDetectionCancel_test()
{
    // Create a fake mavlink log
    _createTempLogFile(false);
    QCOMPARE(tempMavlinkLogCount(_logFileExtension), 1);
    MAVLinkProtocol::instance()->checkForLostLogFiles();
    QVERIFY_TRUE_WAIT(tempMavlinkLogCount(_logFileExtension) == 0, TestTimeout::mediumMs());
}

void MavlinkLogTest::_bootLogDetectionSave_test()
{
    // Create a fake mavlink log
    _createTempLogFile(false);
    QCOMPARE(tempMavlinkLogCount(_logFileExtension), 1);
    const int initialSavedLogCount = savedTelemetryLogCount();
    MAVLinkProtocol::instance()->checkForLostLogFiles();
    QVERIFY_TRUE_WAIT(tempMavlinkLogCount(_logFileExtension) == 0, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(savedTelemetryLogCount() == (initialSavedLogCount + 1), TestTimeout::longMs());
    removeSavedTelemetryLogs();
}

void MavlinkLogTest::_bootLogDetectionZeroLength_test()
{
    // Create a fake empty mavlink log
    _createTempLogFile(true);
    QCOMPARE(tempMavlinkLogCount(_logFileExtension), 1);
    MAVLinkProtocol::instance()->checkForLostLogFiles();
    QVERIFY_TRUE_WAIT(tempMavlinkLogCount(_logFileExtension) == 0, TestTimeout::mediumMs());
    // Zero length log files should not generate any additional UI pop-ups. It should just be deleted silently.
}

void MavlinkLogTest::_connectLogWorker(bool arm)
{
    const int initialSavedLogCount = savedTelemetryLogCount();
    const bool originalSaveNotArmed =
        SettingsManager::instance()->mavlinkSettings()->telemetrySaveNotArmed()->rawValue().toBool();
    if (arm) {
        SettingsManager::instance()->mavlinkSettings()->telemetrySaveNotArmed()->setRawValue(true);
    }
    _connectMockLink();
    if (arm) {
        MultiVehicleManager::instance()->activeVehicle()->setArmedShowError(true);
    }
    _disconnectMockLink();
    SettingsManager::instance()->mavlinkSettings()->telemetrySaveNotArmed()->setRawValue(originalSaveNotArmed);
    if (arm) {
        const bool savedLog = UnitTest::waitForCondition(
            [&]() { return savedTelemetryLogCount() == (initialSavedLogCount + 1); },
            TestTimeout::mediumMs(),
            QStringLiteral("savedTelemetryLogCount() == (initialSavedLogCount + 1)"));
        if (savedLog) {
            removeSavedTelemetryLogs();
        } else {
            QCOMPARE(savedTelemetryLogCount(), initialSavedLogCount);
            QCOMPARE(tempMavlinkLogCount(_logFileExtension), 0);
        }
    } else {
        QCOMPARE(savedTelemetryLogCount(), initialSavedLogCount);
    }
}

void MavlinkLogTest::_connectLogNoArm_test()
{
    _connectLogWorker(false);
}

void MavlinkLogTest::_connectLogArm_test()
{
    _connectLogWorker(true);
}

void MavlinkLogTest::_deleteTempLogFiles_test()
{
    // Verify that the MAVLinkProtocol::deleteTempLogFiles api works correctly
    _createTempLogFile(false);
    MAVLinkProtocol::deleteTempLogFiles();
    QDir tmpDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
}

UT_REGISTER_TEST(MavlinkLogTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle,
                 TestLabel::Serial)
