#include "SerialGPSTransportTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>

#include <atomic>
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

    uint8_t buffer[16] = {};
    QCOMPARE(transport.read(buffer, static_cast<int>(sizeof(buffer)), 100), -1);
    QVERIFY(transport.isCancelled());
}

void SerialGPSTransportTest::_testWriteAbortsWhenStopRequested()
{
    std::atomic_bool stop{false};
    SerialGPSTransport transport(QStringLiteral("/dev/null"), stop);
    QVERIFY(!transport.isCancelled());
    stop = true;

    const uint8_t payload[4] = { 1, 2, 3, 4 };
    QCOMPARE(transport.write(payload, static_cast<int>(sizeof(payload))), -1);
}

void SerialGPSTransportTest::_testCancelPendingOperation_data()
{
    QTest::addColumn<bool>("write");
    QTest::newRow("read") << false;
    QTest::newRow("write") << true;
}

void SerialGPSTransportTest::_testCancelPendingOperation()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(bool, write);
    QFile master;
    const QString slave = openPseudoTerminal(master);
    QVERIFY2(!slave.isEmpty(), "Cannot create a pseudo-terminal for serial cancellation testing");
    std::atomic_bool stop = false;
    SerialGPSTransport transport(slave, stop);
    QVERIFY(transport.open());
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
        const auto result = transport.writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()),
                                                   payload.size(), QDeadlineTimer(TestTimeout::longMs()));
        QCOMPARE(result.status, GPSTransport::WriteStatus::Cancelled);
        QCOMPARE(result.acceptedBytes, payload.size());
        QVERIFY(result.uncertainBytes > 0);
        QCOMPARE(result.writtenBytes + result.uncertainBytes, result.acceptedBytes);
    } else {
        QCOMPARE(transport.read(&byte, 1, TestTimeout::longMs()), -1);
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
    QVERIFY(transport.open());
    const QByteArray payload(4 * 1024 * 1024, 'x');
    QElapsedTimer elapsed;
    elapsed.start();
    const auto result = transport.writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
                                               QDeadlineTimer(100));
    QCOMPARE(result.status, GPSTransport::WriteStatus::TimedOut);
    QCOMPARE(result.acceptedBytes, payload.size());
    QVERIFY(result.writtenBytes > 0);
    QVERIFY(result.uncertainBytes > 0);
    QCOMPARE(result.writtenBytes + result.uncertainBytes, result.acceptedBytes);
    QVERIFY(elapsed.elapsed() < 1000);
    QVERIFY(!stop.load());
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
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
    QVERIFY(transport.open());
    QVERIFY(transport.setBaudrate(baud));
    const QByteArray payload(1029, 'x');
    const auto allowance = transport.correctionWriteTimeout(payload.size());
    QVERIFY(allowance.count() >= minimumWireMs + 100);
    QVERIFY(allowance.count() <= 3000);
    const auto result = transport.writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
                                               QDeadlineTimer(allowance));
    QCOMPARE(result.status, GPSTransport::WriteStatus::Completed);
    QCOMPARE(result.writtenBytes, payload.size());
    QCOMPARE(result.uncertainBytes, 0);
#else
    QSKIP("Serial baud policy coverage requires a Linux pseudo-terminal");
#endif
}

UT_REGISTER_TEST(SerialGPSTransportTest, TestLabel::Unit)
