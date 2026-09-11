#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AndroidSerial.h"

namespace {
AndroidSerialWrite::Step writeStep;
QByteArray attempted;
int writeCalls = 0;
}  // namespace

// Replace only the platform boundary; exercise the production bundled QSerialPort and its buffers.
namespace AndroidSerial {
bool usePosixSerial()
{
    return false;
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
    return true;
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

AndroidSerialWrite::Result writeResult(int, const char* data, int length, int timeout)
{
    ++writeCalls;
    attempted.append(data, length);
    return writeStep(data, length, timeout);
}
}  // namespace AndroidSerial

class AndroidSerialPortWriteTest : public QObject
{
    Q_OBJECT
private slots:

    void init()
    {
        attempted.clear();
        writeCalls = 0;
        writeStep = [](const char*, int count, int) {
            return AndroidSerialWrite::Result{AndroidSerialWrite::Status::Completed, count};
        };
    }

    void acceptancePrecedesConfirmedDelivery()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::WriteOnly));
        QSignalSpy written(&port, &QIODevice::bytesWritten);
        QCOMPARE(port.write("abcdef", 6), 6);
        QCOMPARE(port.bytesToWrite(), 6);
        QCOMPARE(writeCalls, 0);
        QCOMPARE(written.count(), 0);
        QVERIFY(port.flush());
        QCOMPARE(port.bytesToWrite(), 0);
        QCOMPARE(attempted, QByteArray("abcdef"));
        QCOMPARE(written.count(), 1);
        QCOMPARE(written.front().front().toLongLong(), 6);
        QCoreApplication::processEvents();
        QCOMPARE(writeCalls, 1);
    }

    void uncertainFlushNeverRetriesAndReportsOnlyConfirmedBytes()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::WriteOnly));
        QSignalSpy written(&port, &QIODevice::bytesWritten);
        writeStep = [](const char*, int count, int) {
            return AndroidSerialWrite::Result{AndroidSerialWrite::Status::TimedOut, 2, count - 2};
        };
        QCOMPARE(port.write("abcdef", 6), 6);
        QVERIFY(!port.flush());
        QCOMPARE(port.error(), QSerialPort::WriteError);
        QCOMPARE(written.count(), 1);
        QCOMPARE(written.front().front().toLongLong(), 2);
        QCOMPARE(port.bytesToWrite(), 0);
        QVERIFY(!port.flush());
        QCOMPARE(port.write("new", 3), -1);
        QCoreApplication::processEvents();
        QCOMPARE(writeCalls, 1);
        port.close();
        QVERIFY(port.open(QIODevice::WriteOnly));
        writeStep = [](const char*, int count, int) {
            return AndroidSerialWrite::Result{AndroidSerialWrite::Status::Completed, count};
        };
        QCOMPARE(port.write("new", 3), 3);
        QVERIFY(port.flush());
        QCOMPARE(attempted, QByteArray("abcdefnew"));
    }

    void knownPartialSuccessWritesOnlyUnsentSuffix()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::WriteOnly));
        QSignalSpy written(&port, &QIODevice::bytesWritten);
        writeStep = [](const char*, int count, int) {
            return AndroidSerialWrite::Result{AndroidSerialWrite::Status::Completed, writeCalls == 1 ? 2 : count};
        };
        QCOMPARE(port.write("abcdef", 6), 6);
        QVERIFY(port.flush());
        QCOMPARE(attempted, QByteArray("abcdefcdef"));
        QCOMPARE(written.front().front().toLongLong(), 6);
    }

    void failureAfterSuccessfulStepPreservesConfirmedPrefix()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::WriteOnly));
        QSignalSpy written(&port, &QIODevice::bytesWritten);
        writeStep = [](const char*, int count, int) {
            return writeCalls == 1 ? AndroidSerialWrite::Result{AndroidSerialWrite::Status::Completed, count}
                                   : AndroidSerialWrite::Result{AndroidSerialWrite::Status::Error, 0, count};
        };
        QCOMPARE(port.write(QByteArray(20000, 'x')), 20000);
        QVERIFY(!port.flush());
        QCOMPARE(writeCalls, 2);
        QCOMPARE(written.count(), 1);
        QCOMPARE(written.front().front().toLongLong(), 48);
        QCOMPARE(port.bytesToWrite(), 0);
        QCoreApplication::processEvents();
        QCOMPARE(writeCalls, 2);
    }

    void expiredFlushRetainsKnownUnsentBytes()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::WriteOnly));
        QCOMPARE(port.write("abcdef", 6), 6);
        QVERIFY(!port.waitForBytesWritten(0));
        QCOMPARE(port.bytesToWrite(), 6);
        QCOMPARE(writeCalls, 0);
        QVERIFY(port.waitForBytesWritten(500));
        QCOMPARE(attempted, QByteArray("abcdef"));
    }

    void boundedAndOrdinaryWritesShareRetirement()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::WriteOnly));
        writeStep = [](const char*, int count, int) {
            return AndroidSerialWrite::Result{AndroidSerialWrite::Status::Error, 0, count};
        };
        const auto result = port.writeBounded("abc", 3, QDeadlineTimer(500), [] { return false; });
        QCOMPARE(result.uncertainBytes, 3);
        QCOMPARE(port.write("next", 4), -1);
        QCOMPARE(writeCalls, 1);
    }

    void cancelledBoundedWriteDoesNotTransmit()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::WriteOnly));
        const auto result = port.writeBounded("abc", 3, QDeadlineTimer(500), [] { return true; });
        QCOMPARE(result.status, AndroidSerialWrite::Status::Cancelled);
        QCOMPARE(writeCalls, 0);
        QCOMPARE(port.write("next", 4), -1);
    }

    void closeBeforeQueuedFlushDoesNotTransmitOldBytes()
    {
        QSerialPort port(QStringLiteral("test"));
        QVERIFY(port.open(QIODevice::WriteOnly));
        QCOMPARE(port.write("old", 3), 3);
        port.close();
        QVERIFY(port.open(QIODevice::WriteOnly));
        QCOMPARE(port.write("new", 3), 3);
        QTRY_COMPARE_WITH_TIMEOUT(attempted, QByteArray("new"), 500);
        QCOMPARE(writeCalls, 1);
    }

    void deletionDuringConfirmedNotificationIsSafe()
    {
        auto* port = new QSerialPort(QStringLiteral("test"));
        QVERIFY(port->open(QIODevice::WriteOnly));
        QPointer<QSerialPort> guard(port);
        connect(port, &QIODevice::bytesWritten, port, [port] { delete port; });
        QCOMPARE(port->write("abc", 3), 3);
        port->flush();
        QVERIFY(!guard);
    }
};
QTEST_GUILESS_MAIN(AndroidSerialPortWriteTest)
#include "AndroidSerialPortWriteTest.moc"
