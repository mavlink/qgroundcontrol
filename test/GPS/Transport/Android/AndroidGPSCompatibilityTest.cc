#include <atomic>
#include <fcntl.h>
#include <functional>
#include <thread>
#include <unistd.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AndroidSerial.h"
#include "GPSTransportResult.h"
#include "SerialGPSTransport.h"
#include "UnitTest.h"
#include "qserialport_p.h"

namespace {
bool posixBackend = false;
std::function<int(int)> writeStep;
int writeCalls = 0;
int dtrSupport = 1;
bool dtrSuccess = true;
int rtsSupport = 1;
bool rtsSuccess = true;
QStringList warnings;
QSerialPortPrivate* activePort = nullptr;

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

int writeWithProgress(int, const char*, int length, int)
{
    ++writeCalls;
    return writeStep(length);
}
}  // namespace AndroidSerial

class AndroidGPSCompatibilityTest : public UnitTest
{
    Q_OBJECT
private slots:

    void init() override
    {
        UnitTest::init();
        posixBackend = false;
        writeCalls = 0;
        dtrSupport = 1;
        dtrSuccess = true;
        rtsSupport = 1;
        rtsSuccess = true;
        writeStep = [](int length) { return length; };
        warnings.clear();
        activePort = nullptr;
    }

    void backendWriteResults_data()
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

    void backendWriteResults()
    {
        QFETCH(int, step);
        QFETCH(int, status);
        QFETCH(int, written);
        QFETCH(bool, fatal);
        std::atomic_bool stop = false;
        SerialGPSTransport transport(QStringLiteral("test"), stop);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        writeStep = [step](int length) { return step < 0 ? -1 : (std::min) (step, length); };
        const uint8_t payload[4]{};
        if (step < 0) {
            expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                             QRegularExpression(QStringLiteral("^Failed to write to port")));
        }
        const auto result = transport.write(payload, 4, QDeadlineTimer(100));
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
            QCOMPARE(transport.write(payload, 4, QDeadlineTimer(100)).acceptedBytes, 0);
            QCOMPARE(writeCalls, calls);
        }
    }

    void writesAreBufferedAndReported()
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
        QVERIFY(!port.waitForBytesWritten(10));
    }

    void writeWaitTimeoutKeepsPendingBytes()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::ReadWrite));
        QSignalSpy written(&port, &QIODevice::bytesWritten);
        writeStep = [](int length) { return (std::min) (length, 1); };
        QCOMPARE(port.write("abc", 3), 3);
        writeStep = [](int) { return 0; };
        QVERIFY(!port.waitForBytesWritten(10));
        QCOMPARE(port.error(), QSerialPort::TimeoutError);
        QCOMPARE(port.bytesToWrite(), 3);
        port.clearError();
        writeStep = [](int length) { return (std::min) (length, 1); };
        QVERIFY(!port.waitForBytesWritten(10));
        QCOMPARE(port.bytesToWrite(), 2);
        QCOMPARE(port.error(), QSerialPort::TimeoutError);
        QCOMPARE(written.size(), 1);
        writeStep = [](int length) { return length; };
        QVERIFY(port.waitForBytesWritten(10));
        QCOMPARE(port.bytesToWrite(), 0);
        QCOMPARE(written.size(), 2);
    }

    void closeSendsPendingWrites()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::ReadWrite));
        QCOMPARE(port.write("abc", 3), 3);
        QCOMPARE(writeCalls, 0);
        port.close();
        QCOMPARE(writeCalls, 1);
    }

    void posixWriteTimeoutReportsProgress()
    {
        posixBackend = true;
        const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
        QVERIFY(master >= 0);
        const auto closeMaster = qScopeGuard([master] { ::close(master); });
        QCOMPARE(::grantpt(master), 0);
        QCOMPARE(::unlockpt(master), 0);
        QSerialPort port(QString::fromLocal8Bit(::ptsname(master)));
        QVERIFY(port.open(QIODevice::ReadWrite));
        QSignalSpy written(&port, &QIODevice::bytesWritten);
        const QByteArray payload(1024 * 1024, 'x');
        QCOMPARE(port.write(payload), payload.size());
        QVERIFY(!port.waitForBytesWritten(50));
        QCOMPARE(port.error(), QSerialPort::TimeoutError);
        QVERIFY(port.bytesToWrite() > 0);
        QVERIFY(port.bytesToWrite() < payload.size());
        QCOMPARE(written.size(), 1);
        QCOMPARE(written.first().first().toLongLong(), payload.size() - port.bytesToWrite());
        port.clear(QSerialPort::Output);
    }

    void expiredConfigurationSendsNothing()
    {
        std::atomic_bool stop = false;
        SerialGPSTransport transport(QStringLiteral("test"), stop);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        const uint8_t payload = 42;
        QCOMPARE(transport.write(&payload, 1, QDeadlineTimer(0)).status, GPSWriteStatus::TimedOut);
        QCOMPARE(writeCalls, 0);
        stop = true;
        QCOMPARE(transport.write(&payload, 1, QDeadlineTimer(100)).status, GPSWriteStatus::Cancelled);
        QCOMPARE(writeCalls, 0);
    }

    void cancellationRetiresAttemptedWrite()
    {
        std::atomic_bool stop = false;
        SerialGPSTransport transport(QStringLiteral("test"), stop);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        writeStep = [&](int length) {
            stop = true;
            return length;
        };
        const uint8_t payload[4]{};
        const auto result = transport.write(payload, 4, QDeadlineTimer(100));
        QCOMPARE(result.status, GPSWriteStatus::Cancelled);
        QCOMPARE(result.writtenBytes, 4);
        QCOMPARE(result.uncertainBytes(), 0);
        QVERIFY(transport.fatalError());
    }

    void quietReadTimeoutAndCancellation()
    {
        std::atomic_bool stop = false;
        SerialGPSTransport transport(QStringLiteral("test"), stop);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        const auto previous = qInstallMessageHandler(captureWarnings);
        const auto restore = qScopeGuard([previous] { qInstallMessageHandler(previous); });
        uint8_t byte{};
        QCOMPARE(transport.read(&byte, 1, 125).status, GPSReadStatus::TimedOut);
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QStringLiteral("; "))));
        QElapsedTimer elapsed;
        elapsed.start();
        std::jthread cancellation([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            stop = true;
        });
        QCOMPARE(transport.read(&byte, 1, 10000).status, GPSReadStatus::Cancelled);
        QVERIFY(elapsed.elapsed() < 1000);
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QStringLiteral("; "))));
    }

    void incomingDataIsDeliveredOnOwnerThread()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::ReadWrite));
        QVERIFY(activePort);
        QCOMPARE(QString::fromLatin1(port.metaObject()->className()), QStringLiteral("QGCAndroidTestSerialPort"));
        std::atomic<QThread*> notifiedThread = nullptr;
        connect(
            &port, &QIODevice::readyRead, &port, [&] { notifiedThread = QThread::currentThread(); },
            Qt::DirectConnection);
        const QByteArray payload("worker-delivered data");
        {
            std::jthread producer(
                [backend = activePort, payload] { backend->newDataArrived(payload.constData(), payload.size()); });
        }
        QTRY_COMPARE_WITH_TIMEOUT(notifiedThread.load(), port.thread(), TestTimeout::shortMs());
        QCOMPARE(port.readAll(), payload);
        port.close();
        QVERIFY(!activePort);
    }

    void unsupportedDtrClassification()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::ReadWrite));
        dtrSuccess = false;
        dtrSupport = 0;
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to set DTR for device ID")));
        QVERIFY(!port.setDataTerminalReady(true));
        verifyExpectedLogMessage();
        QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
        port.clearError();
        dtrSupport = -1;
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to set DTR for device ID")));
        QVERIFY(!port.setDataTerminalReady(true));
        verifyExpectedLogMessage();
        QCOMPARE(port.error(), QSerialPort::UnknownError);
    }

    void unsupportedRtsClassification()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::ReadWrite));
        rtsSuccess = false;
        rtsSupport = 0;
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to set RTS for device ID")));
        QVERIFY(!port.setRequestToSend(true));
        verifyExpectedLogMessage();
        QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
        port.clearError();
        rtsSupport = -1;
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to set RTS for device ID")));
        QVERIFY(!port.setRequestToSend(true));
        verifyExpectedLogMessage();
        QCOMPARE(port.error(), QSerialPort::UnknownError);
    }

    void posixDtrUnsupported()
    {
        posixBackend = true;
        const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
        QVERIFY(master >= 0);
        const auto closeMaster = qScopeGuard([master] { ::close(master); });
        QCOMPARE(::grantpt(master), 0);
        QCOMPARE(::unlockpt(master), 0);
        QSerialPort port(QString::fromLocal8Bit(::ptsname(master)));
        QVERIFY(port.open(QIODevice::ReadWrite));
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^TIOCMGET failed on ")));
        QVERIFY(!port.setDataTerminalReady(true));
        verifyExpectedLogMessage();
        QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
        QVERIFY(port.isOpen());
    }

    void posixRtsUnsupported()
    {
        posixBackend = true;
        const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
        QVERIFY(master >= 0);
        const auto closeMaster = qScopeGuard([master] { ::close(master); });
        QCOMPARE(::grantpt(master), 0);
        QCOMPARE(::unlockpt(master), 0);
        QSerialPort port(QString::fromLocal8Bit(::ptsname(master)));
        QVERIFY(port.open(QIODevice::ReadWrite));
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^TIOCMGET failed on ")));
        QVERIFY(!port.setRequestToSend(true));
        verifyExpectedLogMessage();
        QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
        QVERIFY(port.isOpen());
    }
};

UT_REGISTER_TEST(AndroidGPSCompatibilityTest, TestLabel::Unit)

#include "AndroidGPSCompatibilityTest.moc"
