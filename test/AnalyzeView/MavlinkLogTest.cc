#include "MavlinkLogTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QUuid>

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif

#include "AppSettings.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MavlinkSettings.h"
#include "MultiVehicleManager.h"
#include "QGCFileHelper.h"
#include "SettingsManager.h"
#include "Vehicle.h"

namespace {
int tempMavlinkLogCount(const char* extension)
{
    QDir tmpDir(QGCFileHelper::tempDirectory());
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

QStringList savedTelemetryLogNames()
{
    const QDir saveDir(SettingsManager::instance()->appSettings()->telemetrySavePath());
    const QString filter = QStringLiteral("*.%1").arg(AppSettings::telemetryFileExtension);
    return saveDir.entryList(QStringList(filter), QDir::Files, QDir::Name);
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
    QDir tmpDir(QGCFileHelper::tempDirectory());
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
    QDir tmpDir(QGCFileHelper::tempDirectory());
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
    removeSavedTelemetryLogs();
    UnitTest::cleanup();
}

void MavlinkLogTest::_createTempLogFile(bool zeroLength, const QString& baseName, QString* filePath)
{
    const QString tempDirPath = QGCFileHelper::tempDirectory();
    const QString uniqueBaseName =
        QStringLiteral("%1_%2").arg(_untimedTempLogPrefix).arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString fileName =
        QStringLiteral("%1.%2").arg(baseName.isEmpty() ? uniqueBaseName : baseName).arg(_logFileExtension);
    QFile tempLogFile(QDir(tempDirPath).filePath(fileName));
    QVERIFY(tempLogFile.open(QIODevice::WriteOnly));
    if (!zeroLength) {
        tempLogFile.write("foo");
    }
    tempLogFile.close();
    if (filePath) {
        *filePath = tempLogFile.fileName();
    }
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
#ifdef Q_OS_UNIX
    // QSaveFile otherwise creates files as 0666 & ~umask; with the common 022 umask that already matches
    // the expected mode, so a restrictive umask proves the explicit permissions are applied.
    const mode_t previousUmask = ::umask(0077);
    const auto restoreUmask = qScopeGuard([previousUmask] { (void) ::umask(previousUmask); });
#endif

    // Create a fake mavlink log
    _createTempLogFile(false);
    QCOMPARE(tempMavlinkLogCount(_logFileExtension), 1);
    const int initialSavedLogCount = savedTelemetryLogCount();
    MAVLinkProtocol::instance()->checkForLostLogFiles();
    QVERIFY_TRUE_WAIT(tempMavlinkLogCount(_logFileExtension) == 0, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(savedTelemetryLogCount() == (initialSavedLogCount + 1), TestTimeout::longMs());

#ifdef Q_OS_UNIX
    // Windows only emulates these bits, so the mode is checked on Unix.
    const QDir saveDir(SettingsManager::instance()->appSettings()->telemetrySavePath());
    const QStringList savedLogs =
        saveDir.entryList(QStringList(QStringLiteral("*.%1").arg(AppSettings::telemetryFileExtension)), QDir::Files);
    QCOMPARE(savedLogs.count(), 1);
    constexpr QFileDevice::Permissions userBits = QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser;
    const QFileDevice::Permissions saved = QFile::permissions(saveDir.filePath(savedLogs.first())) & ~userBits;
    QCOMPARE(saved, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
#endif

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

void MavlinkLogTest::_bootLogDetectionSaveDisabled_test_data()
{
    QTest::addColumn<bool>("telemetrySave");
    QTest::addColumn<bool>("disableAllPersistence");

    QTest::newRow("save telemetry off") << false << false;
    QTest::newRow("all persistence disabled") << true << true;
}

void MavlinkLogTest::_bootLogDetectionSaveDisabled_test()
{
    QFETCH(bool, telemetrySave);
    QFETCH(bool, disableAllPersistence);

    SettingsManager::instance()->mavlinkSettings()->telemetrySave()->setRawValue(telemetrySave);
    SettingsManager::instance()->appSettings()->disableAllPersistence()->setRawValue(disableAllPersistence);

    _createTempLogFile(false);
    MAVLinkProtocol::instance()->checkForLostLogFiles();
    QCOMPARE(tempMavlinkLogCount(_logFileExtension), 0);
    QCOMPARE(savedTelemetryLogCount(), 0);
}

void MavlinkLogTest::_bootLogDetectionNamedForStartTime_test()
{
    const QString startTimeString = QStringLiteral("2024-03-05 10-20-30");

    // Recovering the same start time twice exercises the collision suffix.
    for (int i = 0; i < 2; i++) {
        _createTempLogFile(false, startTimeString);
        MAVLinkProtocol::instance()->checkForLostLogFiles();
        QCOMPARE(tempMavlinkLogCount(_logFileExtension), 0);
    }
    QCOMPARE(savedTelemetryLogNames(),
             QStringList({startTimeString + QStringLiteral(".1.tlog"), startTimeString + QStringLiteral(".tlog")}));
}

void MavlinkLogTest::_bootLogDetectionNamedForModifiedTime_test()
{
    QString filePath;
    _createTempLogFile(false, QString(), &filePath);
    QFile tempLogFile(filePath);
    QVERIFY(tempLogFile.open(QIODevice::ReadWrite));
    QVERIFY(tempLogFile.setFileTime(QDateTime(QDate(2023, 7, 8), QTime(9, 10, 11)), QFileDevice::FileModificationTime));
    tempLogFile.close();

    MAVLinkProtocol::instance()->checkForLostLogFiles();
    QCOMPARE(savedTelemetryLogNames(), QStringList(QStringLiteral("2023-07-08 09-10-11.tlog")));
}

void MavlinkLogTest::_bootLogDetectionSavePathError_test_data()
{
    QTest::addColumn<bool>("savePathSet");
    QTest::addColumn<QString>("expectedError");

    QTest::newRow("save path not set") << false << QStringLiteral("Application save directory is not set");
    QTest::newRow("telemetry directory missing")
        << true << QStringLiteral("Telemetry save directory .* does not exist");
}

void MavlinkLogTest::_bootLogDetectionSavePathError_test()
{
    QFETCH(bool, savePathSet);
    QFETCH(QString, expectedError);

    {
        // The saved log helpers, including the one in cleanup(), resolve an empty save path to the working
        // directory, so the save path is restored before they run again.
        AppSettings* const appSettings = SettingsManager::instance()->appSettings();
        const QString originalSavePath = appSettings->savePath()->rawValue().toString();
        const auto restoreSavePath =
            qScopeGuard([appSettings, originalSavePath] { appSettings->savePath()->setRawValue(originalSavePath); });

        if (savePathSet) {
            QVERIFY(QDir(appSettings->telemetrySavePath()).removeRecursively());
        } else {
            appSettings->savePath()->setRawValue(QString());
        }

        _createTempLogFile(false);
        expectAppMessage(QRegularExpression(QStringLiteral("Unable to save telemetry log\\. ") + expectedError));
        MAVLinkProtocol::instance()->checkForLostLogFiles();
        verifyExpectedLogMessage();
    }

    QCOMPARE(tempMavlinkLogCount(_logFileExtension), 0);
    QCOMPARE(savedTelemetryLogCount(), 0);
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
    QSKIP("MAVLinkProtocol::_startLogging() is a no-op during unit tests, so no "
          "telemetry log is ever created. Testing the arm-triggers-save path "
          "requires either removing the runningUnitTests() guard or injecting a "
          "test-capable logging backend.");
    _connectLogWorker(true);
}

void MavlinkLogTest::_deleteTempLogFiles_test()
{
    // Verify that the MAVLinkProtocol::deleteTempLogFiles api works correctly
    _createTempLogFile(false);
    MAVLinkProtocol::deleteTempLogFiles();
    QDir tmpDir(QGCFileHelper::tempDirectory());
    QStringList logFiles(tmpDir.entryList(QStringList(QString("*.%1").arg(_logFileExtension)), QDir::Files));
    QCOMPARE(logFiles.count(), 0);
}

UT_REGISTER_TEST(MavlinkLogTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle,
                 TestLabel::Serial)
