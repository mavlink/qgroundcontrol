#include "SerialGPSTransportTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <thread>

#include "SerialGPSTransport.h"

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

namespace {
QString openPseudoTerminal(QFile& master)
{
    const int descriptor = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (descriptor < 0) {
        return {};
    }
    if (!master.open(descriptor, QIODevice::ReadWrite, QFileDevice::AutoCloseHandle)) {
        close(descriptor);
        return {};
    }
    if (grantpt(descriptor) != 0 || unlockpt(descriptor) != 0) {
        return {};
    }
    const char* slave = ptsname(descriptor);
    return slave ? QString::fromLocal8Bit(slave) : QString();
}
}  // namespace
#endif

void SerialGPSTransportTest::_testReadAbortsWhenStopRequested()
{
    std::atomic_bool stop{false};
    SerialGPSTransport transport(QStringLiteral("/dev/null"), stop);
    QVERIFY(!transport.isCancelled());
    stop = true;
    QCOMPARE(transport.open().status, GPSOpenStatus::Cancelled);

    uint8_t buffer[16] = {};
    QVERIFY(transport.read(buffer, static_cast<int>(sizeof(buffer)), 100).status != GPSReadStatus::Data);
    QVERIFY(transport.isCancelled());
}

void SerialGPSTransportTest::_testWriteAbortsWhenStopRequested()
{
    std::atomic_bool stop{false};
    SerialGPSTransport transport(QStringLiteral("/dev/null"), stop);
    QVERIFY(!transport.isCancelled());
    stop = true;

    const uint8_t payload[4] = {1, 2, 3, 4};
    QCOMPARE(transport.writeConfiguration(payload, sizeof(payload), QDeadlineTimer(TestTimeout::shortMs())).status,
             GPSWriteStatus::Cancelled);
}

void SerialGPSTransportTest::_testCancelPendingOperation_data()
{
    QTest::addColumn<bool>("write");
    QTest::addColumn<bool>("infiniteDeadline");
    QTest::newRow("read") << false << false;
    QTest::newRow("write") << true << false;
    QTest::newRow("write-forever") << true << true;
}

void SerialGPSTransportTest::_testCancelPendingOperation()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(bool, write);
    QFETCH(bool, infiniteDeadline);
    QFile master;
    const QString slave = openPseudoTerminal(master);
    QVERIFY2(!slave.isEmpty(), "Cannot create a pseudo-terminal for serial cancellation testing");
    std::atomic_bool stop = false;
    SerialGPSTransport transport(slave, stop);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    // Keep the peer unread so writes fill the kernel buffer and remain pending.
    const QByteArray payload(4 * 1024 * 1024, 'x');
    std::jthread cancellation([&]() {
        // Trigger cancellation after the blocking operation has entered its wait.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stop = true;
    });
    QElapsedTimer elapsed;
    elapsed.start();
    uint8_t byte = 0;
    if (write) {
        const auto result = transport.writeBounded(
            reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
            infiniteDeadline ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(TestTimeout::longMs()));
        QCOMPARE(result.status, GPSWriteStatus::Cancelled);
        QVERIFY(result.acceptedBytes < payload.size());
        QVERIFY(result.acceptedBytes > 0);
        QVERIFY(result.uncertainBytes() > 0);
        QCOMPARE(result.writtenBytes + result.uncertainBytes(), result.acceptedBytes);
    } else {
        QVERIFY(transport.read(&byte, 1, TestTimeout::longMs()).status != GPSReadStatus::Data);
    }
    QVERIFY(stop.load());
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
#else
    QSKIP("In-flight serial cancellation requires a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_testPendingWriteDeadline()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFile master;
    const QString slave = openPseudoTerminal(master);
    QVERIFY2(!slave.isEmpty(), "Cannot create a pseudo-terminal for serial write deadline testing");
    std::atomic_bool stop = false;
    SerialGPSTransport transport(slave, stop);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    const QByteArray payload(4 * 1024 * 1024, 'x');
    QElapsedTimer elapsed;
    elapsed.start();
    const auto result = transport.writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
                                               QDeadlineTimer(100));
    QCOMPARE(result.status, GPSWriteStatus::TimedOut);
    QVERIFY(result.acceptedBytes < payload.size());
    QVERIFY(result.acceptedBytes > 0);
    QVERIFY(result.writtenBytes > 0);
    QVERIFY(result.uncertainBytes() > 0);
    QCOMPARE(result.writtenBytes + result.uncertainBytes(), result.acceptedBytes);
    QVERIFY(elapsed.elapsed() < 1000);
    QVERIFY(!stop.load());
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    QVERIFY(transport.fatalError());
    QCOMPARE(transport.writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), 1, QDeadlineTimer(100))
                 .acceptedBytes,
             0);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    const uint8_t next = 42;
    QCOMPARE(transport.writeConfiguration(&next, 1, QDeadlineTimer(TestTimeout::shortMs())).status,
             GPSWriteStatus::Completed);
#else
    QSKIP("A stalled serial write requires a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_testLowBaudCorrectionAllowance_data()
{
    QTest::addColumn<unsigned>("baud");
    QTest::addColumn<int>("minimumWireMs");
    QTest::newRow("9600-max-rtcm") << 9600u << 1072;
    QTest::newRow("38400-max-rtcm") << 38400u << 268;
}

void SerialGPSTransportTest::_testLowBaudCorrectionAllowance()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(unsigned, baud);
    QFETCH(int, minimumWireMs);
    QFile master;
    const QString slave = openPseudoTerminal(master);
    QVERIFY(!slave.isEmpty());
    std::atomic_bool stop = false;
    SerialGPSTransport transport(slave, stop);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QVERIFY(transport.setBaudrate(baud));
    const QByteArray payload(1029, 'x');
    const auto allowance = transport.correctionWriteTimeout(payload.size());
    QVERIFY(allowance.count() >= minimumWireMs + 100);
    QVERIFY(allowance.count() <= 3000);
    const auto result = transport.writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
                                               QDeadlineTimer(allowance));
    QCOMPARE(result.status, GPSWriteStatus::Completed);
    QCOMPARE(result.writtenBytes, payload.size());
    QCOMPARE(result.uncertainBytes(), 0);
#else
    QSKIP("Serial baud policy coverage requires a Linux pseudo-terminal");
#endif
}

UT_REGISTER_TEST(SerialGPSTransportTest, TestLabel::Unit)

void SerialGPSTransportTest::_inputBudgetEndsStream()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFile master;
    const QString slave = openPseudoTerminal(master);
    QVERIFY(!slave.isEmpty());
    const int descriptor = master.handle();
    QVERIFY(fcntl(descriptor, F_SETFL, fcntl(descriptor, F_GETFL) | O_NONBLOCK) >= 0);
    std::atomic_bool stop = false;
    SerialGPSTransport transport(slave, stop);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    std::jthread sender([descriptor](std::stop_token stopping) {
        const QByteArray payload(SerialGPSTransport::kReadBufferBytes + 4096, 'x');
        qsizetype sent = 0;
        while (sent < payload.size() && !stopping.stop_requested()) {
            const auto count = ::write(descriptor, payload.constData() + sent, payload.size() - sent);
            if (count > 0) {
                sent += count;
            } else if (errno != EAGAIN && errno != EINTR) {
                return;
            } else {
                std::this_thread::yield();
            }
        }
    });
    // Let the device's owner thread receive while its decoder is stalled.
    QTRY_VERIFY_WITH_TIMEOUT(transport.fatalError(), TestTimeout::shortMs());
    uint8_t bytes[16]{};
    const auto result = transport.read(bytes, sizeof(bytes), 0);
    QCOMPARE(result.status, GPSReadStatus::Overflow);
    QCOMPARE(result.bytesRead, 0);
    QVERIFY(!result.detail.isEmpty());
    QCOMPARE(transport.read(bytes, sizeof(bytes), 0).status, GPSReadStatus::Overflow);
    QCOMPARE(transport.writeConfiguration(bytes, sizeof(bytes), QDeadlineTimer(TestTimeout::shortMs())).acceptedBytes,
             0);
#else
    QSKIP("Serial ingress budget coverage requires a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_consecutiveWrites()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFile master;
    const QString slave = openPseudoTerminal(master);
    QVERIFY(!slave.isEmpty());
    std::atomic_bool stop = false;
    SerialGPSTransport transport(slave, stop);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QByteArray expected;
    for (int size : {1, 127, 3, 512, 17, 1029, 2}) {
        const QByteArray payload(size, char(size));
        const auto result = transport.writeConfiguration(reinterpret_cast<const uint8_t*>(payload.constData()), size,
                                                         QDeadlineTimer(TestTimeout::shortMs()));
        QCOMPARE(result.status, GPSWriteStatus::Completed);
        QCOMPARE(result.acceptedBytes, size);
        QCOMPARE(result.writtenBytes, size);
        QCOMPARE(result.uncertainBytes(), 0);
        expected.append(payload);
    }
    QVERIFY(fcntl(master.handle(), F_SETFL, fcntl(master.handle(), F_GETFL) | O_NONBLOCK) >= 0);
    QByteArray received;
    const auto complete = [&] {
        char bytes[2048];
        const auto count = ::read(master.handle(), bytes, sizeof(bytes));
        if (count > 0) {
            received.append(bytes, count);
        }
        return received == expected;
    };
    QTRY_VERIFY_WITH_TIMEOUT(complete(), TestTimeout::shortMs());
#else
    QSKIP("Serial write integration requires a Linux pseudo-terminal");
#endif
}
