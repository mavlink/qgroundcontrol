#include "LogReplayLinkTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QtEndian>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LogReplayLink.h"
#include "MAVLinkLib.h"

namespace {

/// Returns a valid tlog stream: cMessages HEARTBEATs, each preceded by a big-endian
/// microsecond timestamp, spaced one second apart starting at baseTimeUSecs.
QByteArray buildTlogBytes(int cMessages, quint64 baseTimeUSecs)
{
    QByteArray tlog;

    for (int i = 0; i < cMessages; i++) {
        const quint64 timestampUSecs = qToBigEndian<quint64>(baseTimeUSecs + (i * 1000000ULL));
        (void) tlog.append(reinterpret_cast<const char*>(&timestampUSecs), sizeof(timestampUSecs));

        mavlink_message_t msg{};
        (void) mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_PX4, 0, 0,
                                          MAV_STATE_ACTIVE);

        uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
        const int cBuffer = mavlink_msg_to_send_buffer(buffer, &msg);
        (void) tlog.append(reinterpret_cast<const char*>(buffer), cBuffer);
    }

    return tlog;
}

}  // namespace

QString LogReplayLinkTest::_writeLogFile(const QByteArray& contents)
{
    if (!_tempDir.isValid()) {
        qCritical() << "Temp dir invalid:" << _tempDir.errorString();
        return QString();
    }

    const QString filename = _tempDir.filePath(QStringLiteral("%1.tlog").arg(QTest::currentTestFunction()));

    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        qCritical() << "Failed to open" << filename << ":" << file.errorString();
        return QString();
    }
    if (file.write(contents) != contents.size()) {
        qCritical() << "Short write to" << filename << ":" << file.errorString();
        return QString();
    }

    return filename;
}

void LogReplayLinkTest::_testCleanLogLoads()
{
    const quint64 baseTimeUSecs = 1700000000000000ULL;
    const QString filename = _writeLogFile(buildTlogBytes(3, baseTimeUSecs));
    QVERIFY(!filename.isEmpty());

    LogReplayConfiguration config(QStringLiteral("LogReplayLinkTest"));
    config.setLogFilename(filename);

    LogReplayWorker worker(&config);
    worker.setup();

    QSignalSpy connectedSpy(&worker, &LogReplayWorker::connected);
    QSignalSpy errorSpy(&worker, &LogReplayWorker::errorOccurred);
    QSignalSpy statsSpy(&worker, &LogReplayWorker::logFileStats);

    worker.connectToLog();

    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(statsSpy.count(), 1);
    QCOMPARE(statsSpy.first().first().toUInt(), 2u);  // 3 messages, 1 second apart

    worker.pause();
    worker.disconnectFromLog();
}

void LogReplayLinkTest::_testTrailingBytesIgnored_data()
{
    QTest::addColumn<int>("cTrailingBytes");
    QTest::addColumn<bool>("expectWarning");

    // Trailing junk smaller than the 8 byte timestamp is never read (loop condition), so no warning.
    // Anything at least timestamp-sized is read as a candidate timestamp, detected as incomplete
    // and warned about.
    QTest::newRow("1 byte") << 1 << false;
    QTest::newRow("7 bytes") << 7 << false;
    QTest::newRow("8 bytes (timestamp size)") << 8 << true;
    QTest::newRow("9 bytes") << 9 << true;
    QTest::newRow("100 bytes") << 100 << true;
}

void LogReplayLinkTest::_testTrailingBytesIgnored()
{
    QFETCH(int, cTrailingBytes);
    QFETCH(bool, expectWarning);

    // Issue #14210: a log which was not closed cleanly (crash/power loss) can end with
    // trailing bytes which are not part of any MAVLink message. Those bytes must be
    // ignored rather than failing the load with a corrupt file error.
    const quint64 baseTimeUSecs = 1700000000000000ULL;
    QByteArray tlog = buildTlogBytes(3, baseTimeUSecs);
    (void) tlog.append(QByteArray(cTrailingBytes, '\0'));

    const QString filename = _writeLogFile(tlog);
    QVERIFY(!filename.isEmpty());

    LogReplayConfiguration config(QStringLiteral("LogReplayLinkTest"));
    config.setLogFilename(filename);

    LogReplayWorker worker(&config);
    worker.setup();

    QSignalSpy connectedSpy(&worker, &LogReplayWorker::connected);
    QSignalSpy errorSpy(&worker, &LogReplayWorker::errorOccurred);
    QSignalSpy statsSpy(&worker, &LogReplayWorker::logFileStats);

    if (expectWarning) {
        expectLogMessage("Comms.LogReplayLink", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Ignoring trailing bytes")));
    }
    worker.connectToLog();
    if (expectWarning) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(statsSpy.count(), 1);
    QCOMPARE(statsSpy.first().first().toUInt(), 2u);  // trailing bytes must not affect duration

    worker.pause();
    worker.disconnectFromLog();
}

void LogReplayLinkTest::_testGarbageOnlyLogFails()
{
    // A log without any complete MAVLink message must still be rejected as corrupt.
    const QString filename = _writeLogFile(QByteArray(100, '\0'));
    QVERIFY(!filename.isEmpty());

    LogReplayConfiguration config(QStringLiteral("LogReplayLinkTest"));
    config.setLogFilename(filename);

    LogReplayWorker worker(&config);
    worker.setup();

    QSignalSpy connectedSpy(&worker, &LogReplayWorker::connected);
    QSignalSpy errorSpy(&worker, &LogReplayWorker::errorOccurred);

    expectLogMessage("Comms.LogReplayLink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No complete MAVLink message found")));
    worker.connectToLog();
    verifyExpectedLogMessage();

    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.first().first().toString().contains(QStringLiteral("corrupt or empty")));
}

UT_REGISTER_TEST(LogReplayLinkTest, TestLabel::Unit, TestLabel::Comms)
