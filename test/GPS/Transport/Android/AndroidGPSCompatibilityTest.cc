#include <QtCore/QElapsedTimer>
#include <QtTest/QTest>

#include <atomic>
#include <fcntl.h>
#include <functional>
#include <thread>
#include <unistd.h>

#include "AndroidSerial.h"
#include "SerialGPSTransport.h"
#include "qserialport_p.h"

namespace {
bool posixBackend = false;
std::function<int(int)> writeStep;
int writeCalls = 0;
int dtrSupport = 1;
bool dtrSuccess = true;
QStringList warnings;

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

void registerPointer(QSerialPortPrivate*) {}

void unregisterPointer(QSerialPortPrivate*) {}

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
    return true;
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

class AndroidGPSCompatibilityTest : public QObject
{
    Q_OBJECT
private slots:

    void init()
    {
        posixBackend = false;
        writeCalls = 0;
        dtrSupport = 1;
        dtrSuccess = true;
        writeStep = [](int length) { return length; };
        warnings.clear();
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
        const auto result = transport.write(payload, 4);
        QCOMPARE(writeCalls, 1);
        QCOMPARE(result.status, count == 4 ? GPSWriteStatus::Completed : GPSWriteStatus::Error);
        QCOMPARE(result.acceptedBytes, 4);
        QCOMPARE(result.writtenBytes, (std::max) (count, 0));
        QCOMPARE(result.uncertainBytes, 4 - (std::max) (count, 0));
        QCOMPARE(transport.fatalError(), count != 4);
        if (count != 4) {
            QCOMPARE(transport.write(payload, 4).acceptedBytes, 0);
            QCOMPARE(writeCalls, 1);
        }
    }

    void boundedWritesSendNothing()
    {
        std::atomic_bool stop = false;
        SerialGPSTransport transport(QStringLiteral("test"), stop);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        const uint8_t payload = 42;
        const auto result = transport.writeBounded(&payload, 1, QDeadlineTimer(100));
        QCOMPARE(result.status, GPSWriteStatus::Unsupported);
        QCOMPARE(result.acceptedBytes, 0);
        QCOMPARE(writeCalls, 0);
        QVERIFY(!transport.fatalError());
        stop = true;
        QCOMPARE(transport.write(&payload, 1).status, GPSWriteStatus::Cancelled);
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
        const auto result = transport.write(payload, 4);
        QCOMPARE(result.status, GPSWriteStatus::Cancelled);
        QCOMPARE(result.writtenBytes, 4);
        QCOMPARE(result.uncertainBytes, 0);
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

    void unsupportedDtrClassification()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::ReadWrite));
        dtrSuccess = false;
        dtrSupport = 0;
        QVERIFY(!port.setDataTerminalReady(true));
        QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
        port.clearError();
        dtrSupport = -1;
        QVERIFY(!port.setDataTerminalReady(true));
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
        QVERIFY(!port.setDataTerminalReady(true));
        QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
        QVERIFY(port.isOpen());
    }
};

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    AndroidGPSCompatibilityTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "AndroidGPSCompatibilityTest.moc"
