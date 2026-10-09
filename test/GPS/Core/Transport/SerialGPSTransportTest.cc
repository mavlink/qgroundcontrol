#include "SerialGPSTransportTest.h"

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <semaphore>
#include <thread>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>

#include "GPSCancellation.h"
#include "SerialGPSTransport.h"
#include "Transport/Support/PseudoTerminal.h"

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

namespace {
/// A write deadline a full terminal cannot meet; short, so the test does not wait long for it.
constexpr int STALLED_WRITE_DEADLINE_MS = 100;
}  // namespace

using namespace GPSTest;
#endif

using namespace std::chrono_literals;

void SerialGPSTransportTest::_testOperationsAbortWhenStopRequested()
{
    GPSCancelSource stop;
    SerialGPSTransport transport(QStringLiteral("/dev/null"), stop.token());
    QVERIFY(!transport.isCancelled());
    stop.cancel();
    QVERIFY(transport.isCancelled());
    QCOMPARE(transport.open().status, GPSOpenStatus::Cancelled);

    std::array<uint8_t, 16> buffer{};
    // Cancelled before it waits, so the timeout is never spent.
    QCOMPARE(transport.read(buffer, 100ms).status, GPSReadStatus::Cancelled);
    QCOMPARE(transport.write(buffer, QDeadlineTimer(TestTimeout::shortMs())).status, GPSWriteStatus::Cancelled);
}

void SerialGPSTransportTest::_openFailureNamesCause()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    const QString device = QStringLiteral("/dev/qgc-missing-gps-receiver");
    SerialGPSTransport transport(device, GPSCancelToken{});
    expectLogMessage("GPS.Transport.SerialGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open Serial Device")));
    const GPSOpenResult result = transport.open();
    verifyExpectedLogMessage();
    QCOMPARE(result.status, GPSOpenStatus::Error);
    QCOMPARE(result.detail, QStringLiteral("%1 was not found").arg(device));
#else
    QSKIP("Missing serial device names are Linux paths here");
#endif
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
    PseudoTerminal terminal;
    QVERIFY2(terminal.isValid(), "Cannot create a pseudo-terminal for serial cancellation testing");
    GPSCancelSource stop;
    SerialGPSTransport transport(terminal.slavePath(), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    // Keep the peer unread so writes fill the terminal and remain pending.
    const QByteArray payload(4 * 1024 * 1024, 'x');
    std::binary_semaphore reading{0};
    std::thread cancellation([&]() {
        // Cancel a write once the terminal takes no more output, so bytes are still pending; cancel a read once it
        // has been called. Both waits are bounded so a failed operation cannot hang the test.
        if (write) {
            (void) terminal.waitForBlockedOutput(TestTimeout::mediumDuration());
        } else {
            (void) reading.try_acquire_for(TestTimeout::mediumDuration());
        }
        stop.cancel();
    });
    const auto joinCancellation = qScopeGuard([&cancellation] { cancellation.join(); });
    if (write) {
        const auto result = transport.write(payload, infiniteDeadline ? QDeadlineTimer(QDeadlineTimer::Forever)
                                                                      : QDeadlineTimer(TestTimeout::longMs()));
        QCOMPARE(result.status, GPSWriteStatus::Cancelled);
        QVERIFY(result.acceptedBytes < payload.size());
        QVERIFY(result.acceptedBytes > 0);
        QVERIFY(result.uncertainBytes() > 0);
        QVERIFY(result.uncertainBytes() <= SerialGPSTransport::WRITE_BUFFER_BYTES);
        QCOMPARE(result.writtenBytes + result.uncertainBytes(), result.acceptedBytes);
        // A write that accepted bytes and did not complete retires the connection.
        QVERIFY(transport.fatalError());
    } else {
        reading.release();
        std::array<uint8_t, 1> byte{};
        QCOMPARE(transport.read(byte, TestTimeout::longDuration()).status, GPSReadStatus::Cancelled);
    }
    QVERIFY(stop.isCancelled());
#else
    QSKIP("In-flight serial cancellation requires a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_testPendingWriteDeadline()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    PseudoTerminal terminal;
    QVERIFY2(terminal.isValid(), "Cannot create a pseudo-terminal for serial write deadline testing");
    GPSCancelSource stop;
    SerialGPSTransport transport(terminal.slavePath(), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    const QByteArray payload(4 * 1024 * 1024, 'x');
    const auto result = transport.write(payload, QDeadlineTimer(STALLED_WRITE_DEADLINE_MS));
    QCOMPARE(result.status, GPSWriteStatus::TimedOut);
    QVERIFY(result.acceptedBytes < payload.size());
    QVERIFY(result.acceptedBytes > 0);
    QVERIFY(result.writtenBytes > 0);
    QVERIFY(result.uncertainBytes() > 0);
    QVERIFY(result.uncertainBytes() <= SerialGPSTransport::WRITE_BUFFER_BYTES);
    QCOMPARE(result.writtenBytes + result.uncertainBytes(), result.acceptedBytes);
    QVERIFY(!stop.isCancelled());
    QVERIFY(transport.fatalError());
    QCOMPARE(transport.write(payload.first(1), QDeadlineTimer(STALLED_WRITE_DEADLINE_MS)).acceptedBytes, 0);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QCOMPARE(transport.write("*", QDeadlineTimer(TestTimeout::shortMs())).status, GPSWriteStatus::Completed);
#else
    QSKIP("A stalled serial write requires a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_inputBudgetEndsStream()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    const int descriptor = terminal.masterHandle();
    GPSCancelSource stop;
    SerialGPSTransport transport(terminal.slavePath(), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    std::atomic_bool stopSending = false;
    std::thread sender([descriptor, &stopSending] {
        const QByteArray payload(SerialGPSTransport::READ_BUFFER_BYTES + 4096, 'x');
        qsizetype sent = 0;
        while (sent < payload.size() && !stopSending.load()) {
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
    const auto joinSender = qScopeGuard([&] {
        stopSending.store(true);
        sender.join();
    });
    // Let the device's owner thread receive while its decoder is stalled.
    QTRY_VERIFY_WITH_TIMEOUT(transport.fatalError(), TestTimeout::shortMs());
    std::array<uint8_t, 16> bytes{};
    const auto result = transport.read(bytes, 0ms);
    QCOMPARE(result.status, GPSReadStatus::Overflow);
    QCOMPARE(result.bytesRead, 0);
    QVERIFY(!result.detail.isEmpty());
    QCOMPARE(transport.read(bytes, 0ms).status, GPSReadStatus::Overflow);
    QCOMPARE(transport.write(bytes, QDeadlineTimer(TestTimeout::shortMs())).acceptedBytes, 0);
#else
    QSKIP("Serial ingress budget coverage requires a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_consecutiveWrites()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    GPSCancelSource stop;
    SerialGPSTransport transport(terminal.slavePath(), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QByteArray expected;
    // Consecutive writes must not credit a previous operation's progress.
    for (int size : {1, 127, 3, 512, 17, 1029, 2}) {
        const QByteArray payload(size, char(size));
        const auto result = transport.write(payload, QDeadlineTimer(TestTimeout::shortMs()));
        QCOMPARE(result.status, GPSWriteStatus::Completed);
        QCOMPARE(result.acceptedBytes, size);
        QCOMPARE(result.writtenBytes, size);
        QCOMPARE(result.uncertainBytes(), 0);
        expected.append(payload);
    }
    QByteArray received;
    QTRY_VERIFY_WITH_TIMEOUT((received += terminal.readAvailable()) == expected, TestTimeout::shortMs());
    QVERIFY(!transport.fatalError());
#else
    QSKIP("Serial write integration requires a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_refusedBaudrateKeepsLink()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    GPSCancelSource stop;
    SerialGPSTransport transport(terminal.slavePath(), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    // QSerialPort refuses a rate it cannot represent without touching the device.
    QVERIFY(!transport.setBaudrate(0));
    QVERIFY(!transport.fatalError());
    QVERIFY(transport.setBaudrate(115200));
    const auto result = transport.write(QByteArrayLiteral("ok"), QDeadlineTimer(TestTimeout::shortMs()));
    QCOMPARE(result.status, GPSWriteStatus::Completed);
    QByteArray received;
    QTRY_VERIFY_WITH_TIMEOUT((received += terminal.readAvailable()) == QByteArrayLiteral("ok"), TestTimeout::shortMs());
#else
    QSKIP("Serial baud-rate integration requires a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_openRetriesUntilTimeout_data()
{
    QTest::addColumn<bool>("busy");
    QTest::newRow("busy") << true;
    QTest::newRow("permission-denied") << false;
}

void SerialGPSTransportTest::_openRetriesUntilTimeout()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(bool, busy);
    if (::geteuid() == 0) {
        QSKIP("Root opens exclusive and unreadable devices");
    }
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    const QByteArray path = terminal.slavePath().toLocal8Bit();
    const int holder = ::open(path.constData(), O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK);
    QVERIFY(holder >= 0);
    const auto release = qScopeGuard([holder] {
        (void) ::ioctl(holder, TIOCNXCL);
        ::close(holder);
    });
    if (busy) {
        QCOMPARE(::ioctl(holder, TIOCEXCL), 0);
    } else {
        QCOMPARE(::chmod(path.constData(), 0), 0);
    }
    // Both refusals read as a device still settling after startup, so open() retries them until its timeout.
    SerialGPSTransport transport(terminal.slavePath(), GPSCancelToken{}, TestTimeout::shortDuration());
    expectLogMessage("GPS.Transport.SerialGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open Serial Device")));
    const GPSOpenResult result = transport.open();
    verifyExpectedLogMessage();
    QCOMPARE(result.status, GPSOpenStatus::TimedOut);
    QCOMPARE(result.detail, busy ? QStringLiteral("%1 is in use by another program").arg(terminal.slavePath())
                                 : QStringLiteral("permission denied for %1").arg(terminal.slavePath()));
#else
    QSKIP("Refused serial opens require a Linux pseudo-terminal");
#endif
}

void SerialGPSTransportTest::_openRetriesUntilReleased()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (::geteuid() == 0) {
        QSKIP("Root opens exclusive devices");
    }
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    const int holder = ::open(terminal.slavePath().toLocal8Bit().constData(), O_RDWR | O_NOCTTY | O_CLOEXEC);
    QVERIFY(holder >= 0);
    QCOMPARE(::ioctl(holder, TIOCEXCL), 0);
    bool released = false;
    QObject context;
    // open() dispatches this thread's events only while waiting to retry, so the release follows a refusal.
    QTimer::singleShot(0, &context, [holder, &released] {
        // Exclusive mode outlives the descriptor while the terminal stays open, so it is cleared first.
        (void) ::ioctl(holder, TIOCNXCL);
        ::close(holder);
        released = true;
    });
    SerialGPSTransport transport(terminal.slavePath(), GPSCancelToken{}, TestTimeout::mediumDuration());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QVERIFY(released);
    QVERIFY(!transport.fatalError());
#else
    QSKIP("Refused serial opens require a Linux pseudo-terminal");
#endif
}

UT_REGISTER_TEST_LIGHTWEIGHT(SerialGPSTransportTest, TestLabel::Unit)
