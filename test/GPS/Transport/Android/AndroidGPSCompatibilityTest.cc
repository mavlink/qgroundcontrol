#include <atomic>
#include <fcntl.h>
#include <functional>
#include <thread>
#include <unistd.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>
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

    void legacyWriteResults_data()
    {
        QTest::addColumn<int>("count");
        QTest::newRow("complete") << 4;
        QTest::newRow("short") << 2;
        QTest::newRow("failure") << -1;
        QTest::newRow("no-progress") << 0;
    }

    void legacyWriteResults()
    {
        QFETCH(int, count);
        std::atomic_bool stop = false;
        SerialGPSTransport transport(QStringLiteral("test"), stop);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        writeStep = [count](int) { return count; };
        const uint8_t payload[4]{};
        if (count < 0) {
            expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                             QRegularExpression(QStringLiteral("^Failed to write to port")));
        }
        const auto result = transport.writeConfiguration(payload, 4, QDeadlineTimer(100));
        if (count < 0) {
            verifyExpectedLogMessage();
        }
        QCOMPARE(writeCalls, 1);
        QCOMPARE(result.status, count == 4 ? GPSWriteStatus::Completed : GPSWriteStatus::Error);
        QCOMPARE(result.acceptedBytes, 4);
        QCOMPARE(result.writtenBytes, (std::max) (count, 0));
        QCOMPARE(result.uncertainBytes(), 4 - (std::max) (count, 0));
        QCOMPARE(transport.fatalError(), count != 4);
        if (count != 4) {
            QCOMPARE(transport.writeConfiguration(payload, 4, QDeadlineTimer(100)).acceptedBytes, 0);
            QCOMPARE(writeCalls, 1);
        }
    }

    void expiredConfigurationSendsNothing()
    {
        std::atomic_bool stop = false;
        SerialGPSTransport transport(QStringLiteral("test"), stop);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        const uint8_t payload = 42;
        QCOMPARE(transport.writeConfiguration(&payload, 1, QDeadlineTimer(0)).status, GPSWriteStatus::TimedOut);
        QCOMPARE(writeCalls, 0);
        stop = true;
        QCOMPARE(transport.writeConfiguration(&payload, 1, QDeadlineTimer(100)).status, GPSWriteStatus::Cancelled);
        QCOMPARE(writeCalls, 0);
    }

    void boundedWritesSendNothing_data()
    {
        QTest::addColumn<int>("timeout");
        QTest::newRow("bounded") << 100;
        QTest::newRow("expired") << 0;
        QTest::newRow("forever") << -1;
    }

    void boundedWritesSendNothing()
    {
        QFETCH(int, timeout);
        std::atomic_bool stop = false;
        SerialGPSTransport transport(QStringLiteral("test"), stop);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        const uint8_t payload = 42;
        const auto result = transport.writeBounded(&payload, 1, QDeadlineTimer(timeout));
        QCOMPARE(result.status, GPSWriteStatus::Unsupported);
        QCOMPARE(result.acceptedBytes, 0);
        QCOMPARE(result.writtenBytes, 0);
        QCOMPARE(result.uncertainBytes(), 0);
        QCOMPARE(writeCalls, 0);
        QVERIFY(!transport.fatalError());
        stop = true;
        QCOMPARE(transport.writeConfiguration(&payload, 1, QDeadlineTimer(100)).status, GPSWriteStatus::Cancelled);
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
        const auto result = transport.writeConfiguration(payload, 4, QDeadlineTimer(100));
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
