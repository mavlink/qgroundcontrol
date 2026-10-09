#include "GPSStreamTransportTest.h"

#include <array>
#include <chrono>
#include <functional>
#include <limits>

#include "GPSCancellation.h"
#include "GPSDeviceTransport.h"
#include "Protocols/Support/ScriptedReceiver.h"

using namespace GPSTest;

namespace {
class StreamWriteDevice : public QIODevice
{
public:
    bool rejectWrite = false;
    qint64 pendingBytes = 0;

    bool isSequential() const override { return true; }

    qint64 bytesToWrite() const override { return pendingBytes; }

protected:
    qint64 readData(char*, qint64) override { return -1; }

    qint64 writeData(const char*, qint64 length) override
    {
        if (rejectWrite) {
            return -1;
        }
        pendingBytes += length;
        return length;
    }
};

/// Drives GPSDeviceTransport's shared writer over a scripted device; onWait runs at each drain wait.
class StreamWriteTransport : public GPSDeviceTransport
{
public:
    using GPSDeviceTransport::GPSDeviceTransport;

    StreamWriteDevice scripted;
    bool fatal = false;
    qint64 confirmed = 0;
    QString detail;
    int retirements = 0;
    std::function<void(QDeadlineTimer&)> onWait;

    GPSOpenResult open() override { return {GPSOpenStatus::Unsupported}; }

    bool fatalError() const override { return fatal; }

    using GPSDeviceTransport::writeData;

protected:
    QIODevice* device() const override { return const_cast<StreamWriteDevice*>(&scripted); }

    GPSReadResult readFailure() const override { return {GPSReadStatus::Closed, 0, detail}; }

    void waitWritten(QDeadlineTimer& deadline) override { onWait(deadline); }

    qint64 confirmedWritten(int, qint64) override { return confirmed; }

    void retire() override
    {
        ++retirements;
        fatal = true;
        scripted.pendingBytes = 0;
        scripted.close();
        detail = QStringLiteral("connection retired");
    }
};

}  // namespace

void GPSStreamTransportTest::_writePreconditions_data()
{
    QTest::addColumn<int>("length");
    QTest::addColumn<bool>("expired");
    QTest::addColumn<bool>("cancelled");
    QTest::addColumn<GPSWriteStatus>("status");
    QTest::addColumn<bool>("reachesLink");
    QTest::newRow("valid") << 1 << false << false << GPSWriteStatus::Completed << true;
    QTest::newRow("empty") << 0 << false << false << GPSWriteStatus::Completed << false;
    QTest::newRow("expired") << 1 << true << false << GPSWriteStatus::TimedOut << false;
    QTest::newRow("cancelled") << 1 << false << true << GPSWriteStatus::Cancelled << false;
}

void GPSStreamTransportTest::_writePreconditions()
{
    QFETCH(int, length);
    QFETCH(bool, expired);
    QFETCH(bool, cancelled);
    QFETCH(GPSWriteStatus, status);
    QFETCH(bool, reachesLink);
    GPSCancelSource stop;
    ScriptedReceiver transport(stop.token());
    int submissions = 0;
    transport.setWriteHandler(
        [&](const QByteArray& bytes, const ScriptedReceiver::WriteContext&) -> std::optional<GPSWriteResult> {
            ++submissions;
            const int size = bytes.size();
            return GPSWriteResult{GPSWriteStatus::Completed, size, size};
        });
    if (cancelled) {
        stop.cancel();
    }
    const auto result =
        transport.write(QByteArray(length, '*'), expired ? QDeadlineTimer(0) : QDeadlineTimer(TestTimeout::shortMs()));
    QCOMPARE(result.status, status);
    // A rejected request never reaches the link, so it reports no progress and cannot retire the connection.
    QCOMPARE(submissions, reachesLink ? 1 : 0);
    QCOMPARE(result.acceptedBytes, reachesLink ? length : 0);
    QCOMPARE(result.writtenBytes, reachesLink ? length : 0);
}

void GPSStreamTransportTest::_writeResultCounts_data()
{
    QTest::addColumn<int>("accepted");
    QTest::addColumn<int>("written");
    QTest::addColumn<int>("uncertain");
    QTest::newRow("empty") << 0 << 0 << 0;
    QTest::newRow("complete") << 12 << 12 << 0;
    QTest::newRow("partial") << 12 << 7 << 5;
    QTest::newRow("unconfirmed") << 12 << 0 << 12;
    QTest::newRow("maximum") << (std::numeric_limits<int>::max)() << 0 << (std::numeric_limits<int>::max)();
    QTest::newRow("negative-accepted") << -1 << 0 << -1;
    QTest::newRow("negative-written") << 0 << -1 << -1;
    QTest::newRow("written-exceeds-accepted") << 12 << 13 << -1;
    QTest::newRow("invalid-negative-complete") << -1 << -1 << -1;
    QTest::newRow("invalid-subtraction-overflow")
        << (std::numeric_limits<int>::max)() << (std::numeric_limits<int>::min)() << -1;
}

void GPSStreamTransportTest::_writeResultCounts()
{
    QFETCH(int, accepted);
    QFETCH(int, written);
    QFETCH(int, uncertain);
    for (const auto status : {GPSWriteStatus::Completed, GPSWriteStatus::TimedOut, GPSWriteStatus::Cancelled,
                              GPSWriteStatus::Error, GPSWriteStatus::Unsupported}) {
        const GPSWriteResult result{status, accepted, written};
        QCOMPARE(result.uncertainBytes(), uncertain);
    }
}

void GPSStreamTransportTest::_sharedWriterEvidence_data()
{
    QTest::addColumn<bool>("rejectWrite");
    QTest::addColumn<GPSWriteStatus>("event");
    QTest::addColumn<bool>("drainAll");
    QTest::addColumn<GPSWriteStatus>("status");
    QTest::addColumn<int>("accepted");
    QTest::addColumn<int>("written");
    QTest::addColumn<int>("uncertain");
    QTest::addColumn<int>("retirements");
    using Status = GPSWriteStatus;
    QTest::newRow("complete") << false << Status::Completed << true << Status::Completed << 4 << 4 << 0 << 0;
    QTest::newRow("fatal-wait") << false << Status::Error << false << Status::Error << 4 << 2 << 2 << 1;
    QTest::newRow("first-write-error") << true << Status::Error << false << Status::Error << 0 << 0 << 0 << 0;
    QTest::newRow("timeout") << false << Status::TimedOut << false << Status::TimedOut << 4 << 2 << 2 << 1;
    QTest::newRow("cancelled") << false << Status::Cancelled << false << Status::Cancelled << 4 << 2 << 2 << 1;
    QTest::newRow("drained-at-deadline") << false << Status::TimedOut << true << Status::Completed << 4 << 4 << 0 << 0;
    QTest::newRow("drained-then-cancelled")
        << false << Status::Cancelled << true << Status::Completed << 4 << 4 << 0 << 0;
}

void GPSStreamTransportTest::_sharedWriterEvidence()
{
    QFETCH(bool, rejectWrite);
    QFETCH(GPSWriteStatus, event);
    QFETCH(bool, drainAll);
    QFETCH(GPSWriteStatus, status);
    QFETCH(int, accepted);
    QFETCH(int, written);
    QFETCH(int, uncertain);
    QFETCH(int, retirements);

    GPSCancelSource stop;
    StreamWriteTransport transport(stop.token());
    StreamWriteDevice& device = transport.scripted;
    device.rejectWrite = rejectWrite;
    QVERIFY(device.open(QIODevice::WriteOnly));
    const std::array<char, 4> payload{};
    const QString errorDetail = QStringLiteral("stream write failed");
    transport.detail = errorDetail;
    transport.onWait = [&](QDeadlineTimer& remaining) {
        const qint64 drained = drainAll ? device.pendingBytes : 2;
        device.pendingBytes -= drained;
        transport.confirmed += drained;
        if (event == GPSWriteStatus::Error) {
            transport.fatal = true;
        } else if (event == GPSWriteStatus::Cancelled) {
            stop.cancel();
        } else if (event == GPSWriteStatus::TimedOut) {
            // Expire the writer's deadline without a wall-clock delay.
            remaining.setRemainingTime(0);
        }
    };
    const auto result =
        transport.writeData(QByteArrayView(payload.data(), payload.size()), QDeadlineTimer(QDeadlineTimer::Forever));
    QCOMPARE(result.status, status);
    QCOMPARE(result.acceptedBytes, accepted);
    QCOMPARE(result.writtenBytes, written);
    QCOMPARE(result.uncertainBytes(), uncertain);
    QCOMPARE(result.detail, status == GPSWriteStatus::Error ? errorDetail : QString());
    QCOMPARE(transport.retirements, retirements);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSStreamTransportTest, TestLabel::Unit)
