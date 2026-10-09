#include "AndroidGPSCompatibilityTest.h"

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <semaphore>
#include <span>
#include <thread>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AndroidSerial.h"
#include "GPSCancellation.h"
#include "GPSTransport.h"
#include "SerialGPSTransport.h"
#include "Transport/Support/PseudoTerminal.h"
#include "qserialport_p.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
bool posixBackend = false;
std::function<int(int)> writeStep;
/// Writes stall for their whole timeout, as a USB device that stopped draining.
bool stallWrites = false;
int writeCalls = 0;
int dtrSupport = 1;
bool dtrSuccess = true;
int rtsSupport = 1;
bool rtsSuccess = true;
QStringList warnings;
QSerialPortPrivate* activePort = nullptr;

/// A write deadline the simulated backend meets at once, unless the test scripts a stall.
constexpr int WRITE_DEADLINE_MS = 100;
/// A wait the simulated backend answers at once; short, so a wait that has to expire keeps the test fast.
constexpr int SHORT_WAIT_MS = 10;
/// A wait a stalled write spends whole; long enough to tell one stall from two.
constexpr int STALL_MS = 200;

void captureWarnings(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        warnings.append(message);
    }
}
}  // namespace

namespace AndroidSerial {
bool usePosixSerial()
{
    return posixBackend;
}

QList<QSerialPortInfo> availableDevices()
{
    return {};
}

QList<QSerialPortInfo> availablePosixPorts()
{
    return {};
}

int open(const QString&, QSerialPortPrivate*)
{
    return 1;
}

bool close(int)
{
    return true;
}

int getDeviceHandle(int)
{
    return 1;
}

void registerPointer(QSerialPortPrivate* port)
{
    activePort = port;
}

void unregisterPointer(QSerialPortPrivate* port)
{
    if (activePort == port) {
        activePort = nullptr;
    }
}

bool setParameters(int, int, int, int, int)
{
    return true;
}

bool setFlowControl(int, int)
{
    return true;
}

bool purgeBuffers(int, bool, bool)
{
    return true;
}

bool startReadThread(int)
{
    return true;
}

bool stopReadThread(int)
{
    return true;
}

bool readThreadRunning(int)
{
    return false;
}

bool setDataTerminalReady(int, bool)
{
    return dtrSuccess;
}

int dataTerminalReadySupport(int)
{
    return dtrSupport;
}

bool setRequestToSend(int, bool)
{
    return rtsSuccess;
}

int requestToSendSupport(int)
{
    return rtsSupport;
}

bool setBreak(int, bool)
{
    return true;
}

QSerialPort::PinoutSignals getControlLines(int)
{
    return {};
}

int write(int, const char*, int length, int, bool)
{
    ++writeCalls;
    return writeStep(length);
}

int writeWithProgress(int, const char*, int length, int timeout)
{
    ++writeCalls;
    if (stallWrites) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        return 0;
    }
    return writeStep(length);
}
}  // namespace AndroidSerial

void AndroidGPSCompatibilityTest::init()
{
    UnitTest::init();
    posixBackend = false;
    writeCalls = 0;
    stallWrites = false;
    dtrSupport = 1;
    dtrSuccess = true;
    rtsSupport = 1;
    rtsSuccess = true;
    writeStep = [](int length) { return length; };
    warnings.clear();
    activePort = nullptr;
}

void AndroidGPSCompatibilityTest::_backendWriteResults_data()
{
    QTest::addColumn<int>("step");
    QTest::addColumn<int>("status");
    QTest::addColumn<int>("written");
    QTest::addColumn<bool>("fatal");
    QTest::newRow("complete") << 4 << int(GPSWriteStatus::Completed) << 4 << false;
    QTest::newRow("partial-then-complete") << 2 << int(GPSWriteStatus::Completed) << 4 << false;
    QTest::newRow("failure") << -1 << int(GPSWriteStatus::Error) << 0 << true;
    QTest::newRow("no-progress") << 0 << int(GPSWriteStatus::TimedOut) << 0 << true;
}

void AndroidGPSCompatibilityTest::_backendWriteResults()
{
    QFETCH(int, step);
    QFETCH(int, status);
    QFETCH(int, written);
    QFETCH(bool, fatal);
    GPSCancelSource stop;
    SerialGPSTransport transport(QStringLiteral("test"), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    writeStep = [step](int length) { return step < 0 ? -1 : (std::min) (step, length); };
    const std::array<uint8_t, 4> payload{};
    if (step < 0) {
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to write to port")));
    }
    const auto result = transport.write(payload, QDeadlineTimer(WRITE_DEADLINE_MS));
    if (step < 0) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(int(result.status), status);
    QCOMPARE(result.acceptedBytes, 4);
    QCOMPARE(result.writtenBytes, written);
    QCOMPARE(result.uncertainBytes(), 4 - written);
    QCOMPARE(transport.fatalError(), fatal);
    if (step == 2) {
        QCOMPARE(writeCalls, 2);
    }
    if (fatal) {
        const int calls = writeCalls;
        QCOMPARE(transport.write(payload, QDeadlineTimer(WRITE_DEADLINE_MS)).acceptedBytes, 0);
        QCOMPARE(writeCalls, calls);
    }
}

void AndroidGPSCompatibilityTest::_writesAreBufferedAndReported()
{
    QSerialPort port(QStringLiteral("test"));
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy written(&port, &QIODevice::bytesWritten);
    QCOMPARE(port.write("abc", 3), 3);
    QCOMPARE(writeCalls, 0);
    QCOMPARE(port.bytesToWrite(), 3);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesToWrite(), 0, TestTimeout::shortMs());
    QCOMPARE(writeCalls, 1);
    QCOMPARE(written.size(), 1);
    QCOMPARE(written.first().first().toLongLong(), 3);
    QVERIFY(!port.waitForBytesWritten(SHORT_WAIT_MS));
}

void AndroidGPSCompatibilityTest::_writeWaitTimeoutKeepsPendingBytes()
{
    QSerialPort port(QStringLiteral("test"));
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy written(&port, &QIODevice::bytesWritten);
    writeStep = [](int length) { return (std::min) (length, 1); };
    QCOMPARE(port.write("abc", 3), 3);
    writeStep = [](int) { return 0; };
    QVERIFY(!port.waitForBytesWritten(SHORT_WAIT_MS));
    QCOMPARE(port.error(), QSerialPort::TimeoutError);
    QCOMPARE(port.bytesToWrite(), 3);
    port.clearError();
    writeStep = [](int length) { return (std::min) (length, 1); };
    QVERIFY(!port.waitForBytesWritten(SHORT_WAIT_MS));
    QCOMPARE(port.bytesToWrite(), 2);
    QCOMPARE(port.error(), QSerialPort::TimeoutError);
    QCOMPARE(written.size(), 1);
    writeStep = [](int length) { return length; };
    QVERIFY(port.waitForBytesWritten(SHORT_WAIT_MS));
    QCOMPARE(port.bytesToWrite(), 0);
    QCOMPARE(written.size(), 2);
}

void AndroidGPSCompatibilityTest::_closeSendsPendingWrites()
{
    QSerialPort port(QStringLiteral("test"));
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(port.write("abc", 3), 3);
    QCOMPARE(writeCalls, 0);
    port.close();
    QCOMPARE(writeCalls, 1);
}

void AndroidGPSCompatibilityTest::_readWaitSharesWriteDeadline()
{
    QSerialPort port(QStringLiteral("test"));
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(port.write("abc", 3), 3);
    stallWrites = true;
    const int writesBefore = writeCalls;
    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(!port.waitForReadyRead(STALL_MS));
    // The stalled write spends the whole wait; the read wait must not start another one.
    QCOMPARE(writeCalls - writesBefore, 1);
    QVERIFY2(elapsed.elapsed() < STALL_MS + TestTimeout::shortMs(),
             qPrintable(QStringLiteral("waited %1 ms").arg(elapsed.elapsed())));
    stallWrites = false;
    port.clear(QSerialPort::Output);
}

void AndroidGPSCompatibilityTest::_posixWriteTimeoutReportsProgress()
{
    posixBackend = true;
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy written(&port, &QIODevice::bytesWritten);
    const QByteArray payload(1024 * 1024, 'x');
    QCOMPARE(port.write(payload), payload.size());
    // The terminal cannot take the whole payload within this short wait.
    QVERIFY(!port.waitForBytesWritten(50));
    QCOMPARE(port.error(), QSerialPort::TimeoutError);
    QVERIFY(port.bytesToWrite() > 0);
    QVERIFY(port.bytesToWrite() < payload.size());
    QCOMPARE(written.size(), 1);
    QCOMPARE(written.first().first().toLongLong(), payload.size() - port.bytesToWrite());
    port.clear(QSerialPort::Output);
}

void AndroidGPSCompatibilityTest::_expiredConfigurationSendsNothing()
{
    GPSCancelSource stop;
    SerialGPSTransport transport(QStringLiteral("test"), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QCOMPARE(transport.write("*", QDeadlineTimer(0)).status, GPSWriteStatus::TimedOut);
    QCOMPARE(writeCalls, 0);
    stop.cancel();
    QCOMPARE(transport.write("*", QDeadlineTimer(WRITE_DEADLINE_MS)).status, GPSWriteStatus::Cancelled);
    QCOMPARE(writeCalls, 0);
}

void AndroidGPSCompatibilityTest::_cancellationAfterFullWriteCompletes()
{
    GPSCancelSource stop;
    SerialGPSTransport transport(QStringLiteral("test"), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    writeStep = [&](int length) {
        stop.cancel();
        return length;
    };
    const std::array<uint8_t, 4> payload{};
    const auto result = transport.write(payload, QDeadlineTimer(WRITE_DEADLINE_MS));
    QCOMPARE(result.status, GPSWriteStatus::Completed);
    QCOMPARE(result.writtenBytes, 4);
    QCOMPARE(result.uncertainBytes(), 0);
    QCOMPARE(transport.write(payload, QDeadlineTimer(WRITE_DEADLINE_MS)).status, GPSWriteStatus::Cancelled);
}

void AndroidGPSCompatibilityTest::_quietReadTimeoutAndCancellation()
{
    GPSCancelSource stop;
    SerialGPSTransport transport(QStringLiteral("test"), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    const auto previous = qInstallMessageHandler(captureWarnings);
    const auto restore = qScopeGuard([previous] { qInstallMessageHandler(previous); });
    uint8_t byte{};
    // A quiet read runs its whole event wait without the backend reporting a timeout as a warning.
    QCOMPARE(transport.read(std::span(&byte, 1), 125ms).status, GPSReadStatus::TimedOut);
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QStringLiteral("; "))));
    std::binary_semaphore reading{0};
    std::thread cancellation([&] {
        // Stop only once the read has been called; the bounded wait keeps a failed read from hanging the test.
        (void) reading.try_acquire_for(TestTimeout::mediumDuration());
        stop.cancel();
    });
    const auto joinCancellation = qScopeGuard([&cancellation] { cancellation.join(); });
    reading.release();
    QCOMPARE(transport.read(std::span(&byte, 1), TestTimeout::longDuration()).status, GPSReadStatus::Cancelled);
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QStringLiteral("; "))));
}

void AndroidGPSCompatibilityTest::_incomingDataIsDeliveredOnOwnerThread()
{
    QSerialPort port(QStringLiteral("test"));
    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(activePort);
    QCOMPARE(QString::fromLatin1(port.metaObject()->className()), QStringLiteral("QGCAndroidTestSerialPort"));
    std::atomic<QThread*> notifiedThread = nullptr;
    connect(
        &port, &QIODevice::readyRead, &port, [&] { notifiedThread = QThread::currentThread(); }, Qt::DirectConnection);
    const QByteArray payload("worker-delivered data");
    std::thread producer(
        [backend = activePort, payload] { backend->newDataArrived(payload.constData(), payload.size()); });
    producer.join();
    QTRY_COMPARE_WITH_TIMEOUT(notifiedThread.load(), port.thread(), TestTimeout::shortMs());
    QCOMPARE(port.readAll(), payload);
    port.close();
    QVERIFY(!activePort);
}

void AndroidGPSCompatibilityTest::_unsupportedControlLineClassification_data()
{
    QTest::addColumn<bool>("dtr");
    QTest::addColumn<int>("support");
    QTest::addColumn<int>("error");
    QTest::newRow("DTR-unsupported") << true << 0 << int(QSerialPort::UnsupportedOperationError);
    QTest::newRow("DTR-unknown") << true << -1 << int(QSerialPort::UnknownError);
    QTest::newRow("RTS-unsupported") << false << 0 << int(QSerialPort::UnsupportedOperationError);
    QTest::newRow("RTS-unknown") << false << -1 << int(QSerialPort::UnknownError);
}

void AndroidGPSCompatibilityTest::_unsupportedControlLineClassification()
{
    QFETCH(bool, dtr);
    QFETCH(int, support);
    QFETCH(int, error);
    QSerialPort port(QStringLiteral("test"));
    QVERIFY(port.open(QIODevice::ReadWrite));
    (dtr ? dtrSuccess : rtsSuccess) = false;
    (dtr ? dtrSupport : rtsSupport) = support;
    const QString line = dtr ? QStringLiteral("DTR") : QStringLiteral("RTS");
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to set %1 for device ID").arg(line)));
    QVERIFY(!(dtr ? port.setDataTerminalReady(true) : port.setRequestToSend(true)));
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), static_cast<QSerialPort::SerialPortError>(error));
}

void AndroidGPSCompatibilityTest::_posixControlLineUnsupported_data()
{
    QTest::addColumn<bool>("dtr");
    QTest::newRow("DTR") << true;
    QTest::newRow("RTS") << false;
}

void AndroidGPSCompatibilityTest::_posixControlLineUnsupported()
{
    QFETCH(bool, dtr);
    posixBackend = true;
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^TIOCMGET failed on ")));
    QVERIFY(!(dtr ? port.setDataTerminalReady(true) : port.setRequestToSend(true)));
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
    QVERIFY(port.isOpen());
}

UT_REGISTER_TEST_LIGHTWEIGHT(AndroidGPSCompatibilityTest, TestLabel::Unit)
