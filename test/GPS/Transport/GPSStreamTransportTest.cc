#include <chrono>
#include <limits>
#include <memory>
#include <thread>
#include <type_traits>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QTest>

#include "GPSStreamWrite_p.h"
#include "TCPGPSTransport.h"
#include "UnitTest.h"

static_assert(std::is_enum_v<GPSOpenStatus>);
static_assert(std::is_enum_v<GPSReadStatus>);
static_assert(std::is_enum_v<GPSWriteStatus>);
static_assert(std::is_enum_v<GPSBaudStatus>);
static_assert(GPSReadStatus::Data != GPSReadStatus::TimedOut);
static_assert(GPSWriteStatus::Completed != GPSWriteStatus::Unsupported);

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

#include "SerialGPSTransport.h"
#endif

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

class StreamWriteTransport : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    bool failed = false;

    GPSOpenResult open() override { return {GPSOpenStatus::Unsupported}; }

    bool fatalError() const override { return failed; }

    GPSReadResult read(uint8_t*, int, int) override { return {GPSReadStatus::Closed}; }

    bool setBaudrate(unsigned) override { return false; }
};
}  // namespace

class GPSStreamTransportTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _defaultWriteContract()
    {
        std::atomic_bool stop = false;
        StreamWriteTransport transport(stop);
        const uint8_t byte = 1;
        QCOMPARE(transport.writeConfiguration(&byte, 1, QDeadlineTimer(transport.configurationWriteTimeout())).status,
                 GPSWriteStatus::Unsupported);
        stop = true;
        QCOMPARE(transport.writeConfiguration(&byte, 1, QDeadlineTimer(transport.configurationWriteTimeout())).status,
                 GPSWriteStatus::Cancelled);
    }

    void _serialCorrectionAllowance_data()
    {
        QTest::addColumn<int>("length");
        QTest::addColumn<qint64>("baud");
        QTest::addColumn<qint64>("expectedMs");
        QTest::newRow("low-baud-frame") << 1029 << qint64(9600) << qint64(1172);
        QTest::newRow("high-baud-frame") << 1029 << qint64(38400) << qint64(368);
        QTest::newRow("empty") << 0 << qint64(9600) << qint64(200);
        QTest::newRow("negative-length") << -1 << qint64(9600) << qint64(200);
        QTest::newRow("zero-baud") << 1029 << qint64(0) << qint64(200);
        QTest::newRow("negative-baud") << 1029 << qint64(-1) << qint64(200);
        QTest::newRow("round-up") << 1 << qint64(115200) << qint64(101);
        QTest::newRow("bounded-allowance") << 1000000 << qint64(9600) << qint64(3000);
    }

    void _serialCorrectionAllowance()
    {
        QFETCH(int, length);
        QFETCH(qint64, baud);
        QFETCH(qint64, expectedMs);
        QCOMPARE(GPSTransport::serialCorrectionWriteTimeout(length, baud), std::chrono::milliseconds(expectedMs));
    }

    void _writeResultCounts_data()
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

    void _writeResultCounts()
    {
        QFETCH(int, accepted);
        QFETCH(int, written);
        QFETCH(int, uncertain);
        for (const auto status : {GPSWriteStatus::Completed, GPSWriteStatus::TimedOut, GPSWriteStatus::Cancelled,
                                  GPSWriteStatus::Error, GPSWriteStatus::Unsupported, GPSWriteStatus::InvalidData}) {
            const GPSWriteResult result{status, accepted, written};
            QCOMPARE(result.uncertainBytes(), uncertain);
        }
    }

    void _sharedWriterEvidence_data()
    {
        QTest::addColumn<bool>("rejectWrite");
        QTest::addColumn<GPSWriteStatus>("status");
        QTest::addColumn<int>("accepted");
        QTest::addColumn<int>("written");
        QTest::addColumn<int>("uncertain");
        QTest::addColumn<int>("retirements");
        QTest::newRow("complete") << false << GPSWriteStatus::Completed << 4 << 4 << 0 << 0;
        QTest::newRow("fatal-wait") << false << GPSWriteStatus::Error << 4 << 2 << 2 << 1;
        QTest::newRow("first-write-error") << true << GPSWriteStatus::Error << 0 << 0 << 0 << 0;
        QTest::newRow("timeout") << false << GPSWriteStatus::TimedOut << 4 << 2 << 2 << 1;
        QTest::newRow("cancelled") << false << GPSWriteStatus::Cancelled << 4 << 2 << 2 << 1;
    }

    void _sharedWriterEvidence()
    {
        QFETCH(bool, rejectWrite);
        QFETCH(GPSWriteStatus, status);
        QFETCH(int, accepted);
        QFETCH(int, written);
        QFETCH(int, uncertain);
        QFETCH(int, retirements);

        std::atomic_bool stop = false;
        StreamWriteTransport transport(stop);
        StreamWriteDevice device;
        device.rejectWrite = rejectWrite;
        QVERIFY(device.open(QIODevice::WriteOnly));
        const uint8_t payload[4] = {};
        const QString errorDetail = QStringLiteral("stream write failed");
        QString detail = errorDetail;
        qint64 confirmed = 0;
        int retirementCount = 0;
        const auto result = GPSStreamWrite::writeBounded(
            transport, &device, payload, sizeof(payload), QDeadlineTimer(QDeadlineTimer::Forever), 4,
            [&](QDeadlineTimer& remaining) {
                const qint64 drained = status == GPSWriteStatus::Completed ? device.pendingBytes : 2;
                device.pendingBytes -= drained;
                confirmed += drained;
                if (status == GPSWriteStatus::Error) {
                    transport.failed = true;
                } else if (status == GPSWriteStatus::Cancelled) {
                    stop = true;
                } else if (status == GPSWriteStatus::TimedOut) {
                    // Expire the helper's deadline without a wall-clock delay.
                    remaining.setRemainingTime(0);
                }
            },
            [&](int) { return confirmed; }, [&]() { return detail; },
            [&]() {
                ++retirementCount;
                transport.failed = true;
                device.pendingBytes = 0;
                device.close();
                detail = QStringLiteral("connection retired");
            });
        QCOMPARE(result.status, status);
        QCOMPARE(result.acceptedBytes, accepted);
        QCOMPARE(result.writtenBytes, written);
        QCOMPARE(result.uncertainBytes(), uncertain);
        QCOMPARE(result.detail, status == GPSWriteStatus::Error ? errorDetail : QString());
        QCOMPARE(retirementCount, retirements);
    }

    void writes_data()
    {
        QTest::addColumn<bool>("serial");
        QTest::addColumn<QByteArray>("outcome");
        for (bool serial : {false, true}) {
            for (const QByteArray outcome : {"complete", "expired", "cancelled", "cancelled-forever", "timeout"}) {
                QTest::newRow(((serial ? "serial-" : "tcp-") + outcome).constData()) << serial << outcome;
            }
        }
    }

    void writes()
    {
        QFETCH(bool, serial);
        QFETCH(QByteArray, outcome);
        std::atomic_bool stop = false;
        QFile master;
        QTcpServer server;
        QTcpSocket* peer = nullptr;
        std::unique_ptr<GPSTransport> transport;
        if (serial) {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
            const int descriptor = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK);
            QVERIFY(descriptor >= 0);
            if (!master.open(descriptor, QIODevice::ReadWrite, QFileDevice::AutoCloseHandle)) {
                close(descriptor);
                QFAIL("Cannot open pseudo-terminal");
            }
            QVERIFY(grantpt(descriptor) == 0);
            QVERIFY(unlockpt(descriptor) == 0);
            const char* slave = ptsname(descriptor);
            QVERIFY(slave);
            transport = std::make_unique<SerialGPSTransport>(QString::fromLocal8Bit(slave), stop);
#else
            QSKIP("Desktop serial integration requires a Linux pseudo-terminal");
#endif
        } else {
            QVERIFY(server.listen(QHostAddress::LocalHost));
            transport = std::make_unique<TCPGPSTransport>(QStringLiteral("127.0.0.1"), server.serverPort(), stop);
        }
        QCOMPARE(transport->open().status, GPSOpenStatus::Opened);
        if (!serial) {
            QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 1000);
            peer = server.nextPendingConnection();
            QVERIFY(peer);
            if (outcome != "complete") {
                peer->setReadBufferSize(1);
            }
        }
        const uint8_t byte = 42;
        QCOMPARE(transport->writeBounded(nullptr, 1, QDeadlineTimer(100)).status, GPSWriteStatus::InvalidData);
        QCOMPARE(transport->writeBounded(&byte, -1, QDeadlineTimer(100)).status, GPSWriteStatus::InvalidData);
        QCOMPARE(transport->writeBounded(&byte, 0, QDeadlineTimer(0)).status, GPSWriteStatus::Completed);
        if (outcome == "expired") {
            const auto result = transport->writeBounded(&byte, 1, QDeadlineTimer(0));
            QCOMPARE(result.status, GPSWriteStatus::TimedOut);
            QCOMPARE(result.acceptedBytes, 0);
            QCOMPARE(result.writtenBytes, 0);
            QCOMPARE(result.uncertainBytes(), 0);
            QVERIFY(!transport->fatalError());
            return;
        }
        if (outcome == "complete") {
            QByteArray expected;
            // Consecutive writes must not credit a previous operation's signals.
            for (int size : {1, 127, 3, 512, 17, 1029, 2}) {
                const QByteArray payload(size, static_cast<char>(size));
                const auto result = transport->writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), size,
                                                            QDeadlineTimer(1000));
                QCOMPARE(result.status, GPSWriteStatus::Completed);
                QCOMPARE(result.acceptedBytes, size);
                QCOMPARE(result.writtenBytes, size);
                QCOMPARE(result.uncertainBytes(), 0);
                expected.append(payload);
            }
            QByteArray received;
            const auto readPeer = [&]() {
                if (peer) {
                    received.append(peer->readAll());
                } else {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
                    char bytes[2048];
                    const auto count = ::read(master.handle(), bytes, sizeof(bytes));
                    if (count > 0) {
                        received.append(bytes, count);
                    }
#endif
                }
                return received == expected;
            };
            QTRY_VERIFY_WITH_TIMEOUT(readPeer(), 1000);
            QVERIFY(!transport->fatalError());
            return;
        }
        QCOMPARE(transport->writeConfiguration(&byte, 1, QDeadlineTimer(transport->configurationWriteTimeout()))
                     .writtenBytes,
                 1);
        const QByteArray payload(16 * 1024 * 1024, 'x');
        std::jthread cancellation;
        const bool cancelled = outcome.startsWith("cancelled");
        if (cancelled) {
            cancellation = std::jthread([&]() {
                // Cancel after submission, before the bounded deadline expires.
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                stop = true;
            });
        }
        QElapsedTimer elapsed;
        elapsed.start();
        const auto result = transport->writeBounded(
            reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
            outcome == "cancelled-forever" ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(100));
        QCOMPARE(result.status, cancelled ? GPSWriteStatus::Cancelled : GPSWriteStatus::TimedOut);
        QVERIFY(elapsed.elapsed() < 1000);
        QVERIFY(result.acceptedBytes > 0);
        QVERIFY(result.acceptedBytes < payload.size());
        QVERIFY(result.writtenBytes >= 0);
        QVERIFY(result.uncertainBytes() >= 0);
        QVERIFY(result.uncertainBytes() <= 4096);
        QVERIFY(transport->fatalError());
        stop = false;
        QCOMPARE(transport->writeBounded(&byte, 1, QDeadlineTimer(100)).acceptedBytes, 0);
    }
};

UT_REGISTER_TEST(GPSStreamTransportTest, TestLabel::Unit)
#include "GPSStreamTransportTest.moc"
