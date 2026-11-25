#include "MavlinkLogTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QtEndian>
#include <QtTest/QSignalSpy>

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif

#include "AppSettings.h"
#include "LinkManager.h"
#include "LogReplayLink.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MavlinkSettings.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

namespace {
constexpr const char* inProgressTelemetryLogFilter = "*.tlog.inprogress";
constexpr const char* savedTelemetryLogFilter = "*.tlog";

QDir telemetrySaveDir()
{
    return QDir(SettingsManager::instance()->appSettings()->telemetrySavePath());
}

QStringList telemetryLogNames(const char* filter)
{
    return telemetrySaveDir().entryList(QStringList(QLatin1String(filter)), QDir::Files, QDir::Name);
}

void removeFiles(QDir dir, const char* filter)
{
    const QStringList files = dir.entryList(QStringList(QLatin1String(filter)), QDir::Files);
    for (const QString& file : files) {
        QVERIFY(dir.remove(file));
    }
}

void createFile(const QString& filePath)
{
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("foo"), 3);
}
}  // namespace

void MavlinkLogTest::init()
{
    UnitTest::init();
    MultiVehicleManager::instance()->init();
    // Connects the vehicle removal that stops logging
    MAVLinkProtocol::instance()->init();
    LinkManager::instance()->setConnectionsAllowed();

    const QString testRoot = QDir::tempPath() + QStringLiteral("/QGroundControl_unittest_MavlinkLogTest");
    QVERIFY(QDir().mkpath(testRoot));
    SettingsManager::instance()->appSettings()->savePath()->setRawValue(testRoot);
    SettingsManager::instance()->appSettings()->disableAllPersistence()->setRawValue(false);
    SettingsManager::instance()->mavlinkSettings()->telemetrySave()->setRawValue(true);
    SettingsManager::instance()->mavlinkSettings()->telemetrySaveNotArmed()->setRawValue(false);

    QVERIFY(!telemetrySaveDir().path().isEmpty());
    QVERIFY(QDir().mkpath(telemetrySaveDir().path()));
    removeFiles(telemetrySaveDir(), inProgressTelemetryLogFilter);
    removeFiles(telemetrySaveDir(), savedTelemetryLogFilter);

    MAVLinkProtocol::instance()->setLoggingEnabledForTest(true);
}

void MavlinkLogTest::cleanup()
{
    _disconnectMockLink();
    MAVLinkProtocol::instance()->setLoggingEnabledForTest(false);
    removeFiles(telemetrySaveDir(), inProgressTelemetryLogFilter);
    removeFiles(telemetrySaveDir(), savedTelemetryLogFilter);
    UnitTest::cleanup();
}

void MavlinkLogTest::_connectAndArm()
{
    _connectMockLink();
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    vehicle->setArmedShowError(true);
    QVERIFY_TRUE_WAIT(vehicle->armed(), TestTimeout::mediumMs());
}

void MavlinkLogTest::_disconnectNotArmed_test_data()
{
    QTest::addColumn<bool>("saveNotArmed");

    QTest::newRow("discarded") << false;
    QTest::newRow("saved when unarmed logs are kept") << true;
}

void MavlinkLogTest::_disconnectNotArmed_test()
{
    QFETCH(bool, saveNotArmed);

    SettingsManager::instance()->mavlinkSettings()->telemetrySaveNotArmed()->setRawValue(saveNotArmed);

    _connectMockLink();
    QVERIFY_TRUE_WAIT((telemetryLogNames(inProgressTelemetryLogFilter).count() +
                       telemetryLogNames(savedTelemetryLogFilter).count()) == 1,
                      TestTimeout::mediumMs());
    _disconnectMockLink();

    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 0);
    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter).count(), saveNotArmed ? 1 : 0);
}

void MavlinkLogTest::_armedLogSavedWhileConnected_test()
{
#ifdef Q_OS_UNIX
    // Files are otherwise created as 0666 & ~umask; with the common 022 umask that already matches the
    // expected mode, so a restrictive umask proves the explicit permissions are applied.
    const mode_t previousUmask = ::umask(0077);
    const auto restoreUmask = qScopeGuard([previousUmask] { (void) ::umask(previousUmask); });
#endif

    _connectAndArm();
    if (QTest::currentTestFailed()) {
        return;
    }

    // The log already has its final name, so a crash from here on needs no recovery.
    QVERIFY_TRUE_WAIT(telemetryLogNames(savedTelemetryLogFilter).count() == 1, TestTimeout::mediumMs());
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 0);
    const QString savedLogName = telemetryLogNames(savedTelemetryLogFilter).first();
    const QString savedLogPath = telemetrySaveDir().filePath(savedLogName);
    static const QRegularExpression startTimeName(QStringLiteral("^\\d{4}-\\d{2}-\\d{2} \\d{2}-\\d{2}-\\d{2}\\.tlog$"));
    QVERIFY2(startTimeName.match(savedLogName).hasMatch(), qPrintable(savedLogName));
    const qint64 sizeWhenRenamed = QFileInfo(savedLogPath).size();
    QVERIFY(sizeWhenRenamed > 0);

#ifdef Q_OS_UNIX
    // Windows only emulates these bits, so the mode is checked on Unix.
    constexpr QFileDevice::Permissions userBits = QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser;
    QCOMPARE(QFile::permissions(savedLogPath) & ~userBits,
             QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
#endif

    // Traffic after the rename must still land in the same file.
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    vehicle->setArmedShowError(false);
    QVERIFY_TRUE_WAIT(!vehicle->armed(), TestTimeout::mediumMs());
    _disconnectMockLink();

    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter), QStringList(savedLogName));
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 0);
    QVERIFY(QFileInfo(savedLogPath).size() > sizeWhenRenamed);
}

void MavlinkLogTest::_saveTurnedOnWhileConnected_test()
{
    SettingsManager::instance()->mavlinkSettings()->telemetrySave()->setRawValue(false);

    _connectAndArm();
    if (QTest::currentTestFailed()) {
        return;
    }
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 1);
    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter).count(), 0);

    SettingsManager::instance()->mavlinkSettings()->telemetrySave()->setRawValue(true);
    QVERIFY_TRUE_WAIT(telemetryLogNames(savedTelemetryLogFilter).count() == 1, TestTimeout::mediumMs());
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 0);

    _disconnectMockLink();
    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter).count(), 1);
}

void MavlinkLogTest::_saveTurnedOffWhileConnected_test()
{
    _connectAndArm();
    if (QTest::currentTestFailed()) {
        return;
    }
    QVERIFY_TRUE_WAIT(telemetryLogNames(savedTelemetryLogFilter).count() == 1, TestTimeout::mediumMs());

    SettingsManager::instance()->mavlinkSettings()->telemetrySave()->setRawValue(false);
    _disconnectMockLink();

    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter).count(), 0);
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 0);
}

void MavlinkLogTest::_replayArmedStateNotInherited_test()
{
    // A component heartbeat creates no vehicle, so the live vehicle below is the only one.
    QByteArray tlog;
    for (quint64 timestampUSecs : {1700000000000000ULL, 1700000000001000ULL}) {
        const quint64 bigEndianTimestamp = qToBigEndian(timestampUSecs);
        (void) tlog.append(reinterpret_cast<const char*>(&bigEndianTimestamp), sizeof(bigEndianTimestamp));

        mavlink_message_t message{};
        (void) mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_CAMERA, &message, MAV_TYPE_CAMERA, MAV_AUTOPILOT_INVALID,
                                          MAV_MODE_FLAG_SAFETY_ARMED, 0, MAV_STATE_ACTIVE);
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
        (void) tlog.append(reinterpret_cast<const char*>(buffer), mavlink_msg_to_send_buffer(buffer, &message));
    }
    QTemporaryDir replayDir;
    QVERIFY(replayDir.isValid());
    const QString replayLogPath = replayDir.filePath(QStringLiteral("replay.tlog"));
    QFile replayLog(replayLogPath);
    QVERIFY(replayLog.open(QIODevice::WriteOnly));
    QCOMPARE(replayLog.write(tlog), tlog.size());
    replayLog.close();

    auto* const replayConfig = new LogReplayConfiguration(QStringLiteral("MavlinkLogTestReplay"));
    replayConfig->setLogFilename(replayLogPath);
    replayConfig->setDynamic(true);
    SharedLinkConfigurationPtr replayLinkConfig = LinkManager::instance()->addConfiguration(replayConfig);
    QSignalSpy heartbeatSpy(MAVLinkProtocol::instance(), &MAVLinkProtocol::vehicleHeartbeatInfo);
    QVERIFY(LinkManager::instance()->createConnectedLink(replayLinkConfig));
    auto* const replayLink = qobject_cast<LogReplayLink*>(replayLinkConfig->link());
    QVERIFY(replayLink);
    // The worker closes the replay log on its own thread, which must happen before replayDir is removed.
    const SharedLinkInterfacePtr sharedReplayLink =
        LinkManager::instance()->sharedLinkInterfacePointerForLink(replayLink);
    QVERIFY(sharedReplayLink);
    const auto disconnectReplay = qScopeGuard([sharedReplayLink] {
        if (!sharedReplayLink->isConnected()) {
            return;
        }
        QSignalSpy disconnectedSpy(sharedReplayLink.get(), &LinkInterface::disconnected);
        LinkManager::instance()->disconnectLink(sharedReplayLink.get());
        (void) disconnectedSpy.wait(TestTimeout::mediumMs());
    });

    // The replay pauses itself at the end of the log, which allows connections again.
    QSignalSpy pausedSpy(replayLink, &LogReplayLink::playbackPaused);
    QVERIFY(pausedSpy.wait(TestTimeout::mediumMs()));
    QCOMPARE(heartbeatSpy.count(), 2);

    _connectMockLink();
    if (QTest::currentTestFailed()) {
        return;
    }
    QVERIFY(!_vehicle->armed());
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 1);
    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter).count(), 0);
}

void MavlinkLogTest::_foreignArmedHeartbeatIgnored_test()
{
    _connectMockLink();
    if (QTest::currentTestFailed()) {
        return;
    }
    QVERIFY(!_vehicle->armed());
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 1);

    // A target system above 255 is carried in the TARGET32 header extension.
    constexpr uint32_t foreignTargetSystem = 300;
    mavlink_message_t message{};
    (void) mavlink_msg_heartbeat_pack(2, MAV_COMP_ID_AUTOPILOT1, &message, MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_PX4,
                                      MAV_MODE_FLAG_SAFETY_ARMED, 0, MAV_STATE_ACTIVE);
    mavlink_status_t status{};
    (void) mavlink_finalize_message_buffer_target(&message, 2, MAV_COMP_ID_AUTOPILOT1, &status,
                                                  MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN, MAVLINK_MSG_ID_HEARTBEAT_LEN,
                                                  MAVLINK_MSG_ID_HEARTBEAT_CRC, foreignTargetSystem);
    QVERIFY(message.incompat_flags & MAVLINK_IFLAG_TARGET32);

    QSignalSpy heartbeatSpy(MAVLinkProtocol::instance(), &MAVLinkProtocol::vehicleHeartbeatInfo);
    _mockLink->respondWithMavlinkMessage(message);
    QTRY_VERIFY_WITH_TIMEOUT(heartbeatSpy.count() >= 2, TestTimeout::mediumMs());

    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 1);
    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter).count(), 0);
}

void MavlinkLogTest::_saveDirectoryMissing_test()
{
    const QString saveDirPath = telemetrySaveDir().path();
    QVERIFY(QDir(saveDirPath).removeRecursively());

    expectAppMessage(QRegularExpression(
        QStringLiteral("Telemetry log save path is not valid\\. Telemetry save directory .* does not exist")));
    _connectMockLink();
    verifyExpectedLogMessage();

    // Recording starts once the directory is back, without reconnecting.
    QVERIFY(QDir().mkpath(saveDirPath));
    QVERIFY_TRUE_WAIT(telemetryLogNames(inProgressTelemetryLogFilter).count() == 1, TestTimeout::mediumMs());
}

void MavlinkLogTest::_savedNameCollision_test()
{
    // With saving off the log keeps its in-progress name, which gives the start time to collide with.
    SettingsManager::instance()->mavlinkSettings()->telemetrySave()->setRawValue(false);
    _connectMockLink();
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 1);
    const QString startTimeString = QFileInfo(telemetryLogNames(inProgressTelemetryLogFilter).first()).baseName();
    const QString existingLogName = startTimeString + QStringLiteral(".tlog");
    createFile(telemetrySaveDir().filePath(existingLogName));

    SettingsManager::instance()->mavlinkSettings()->telemetrySaveNotArmed()->setRawValue(true);
    SettingsManager::instance()->mavlinkSettings()->telemetrySave()->setRawValue(true);
    QVERIFY_TRUE_WAIT(telemetryLogNames(inProgressTelemetryLogFilter).count() == 0, TestTimeout::mediumMs());
    _disconnectMockLink();

    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter),
             QStringList({startTimeString + QStringLiteral(".1.tlog"), existingLogName}));
    QCOMPARE(QFileInfo(telemetrySaveDir().filePath(existingLogName)).size(), 3);
}

void MavlinkLogTest::_removeOrphanedLogFiles_test()
{
    const QString savedLogName = QStringLiteral("2024-03-05 10-20-30.tlog");
    createFile(telemetrySaveDir().filePath(savedLogName));
    createFile(telemetrySaveDir().filePath(QStringLiteral("2024-03-05 11-20-30.tlog.inprogress")));

    MAVLinkProtocol::removeOrphanedLogFiles();

    QCOMPARE(telemetryLogNames(savedTelemetryLogFilter), QStringList(savedLogName));
    QCOMPARE(telemetryLogNames(inProgressTelemetryLogFilter).count(), 0);
}

UT_REGISTER_TEST(MavlinkLogTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle,
                 TestLabel::Serial)
