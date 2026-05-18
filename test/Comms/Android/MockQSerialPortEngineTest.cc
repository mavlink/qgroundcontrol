// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "MockQSerialPortEngineTest.h"

#include "MockQSerialPortEngine.h"
#include "UnitTest.h"
#include "qandroidserialenginereceiver_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QObject>

UT_REGISTER_TEST(MockQSerialPortEngineTest, TestLabel::Unit, TestLabel::Comms)

namespace {

// Minimal QAndroidSerialEngineReceiver that records all callbacks for assertion.
class RecordingReceiver : public QAndroidSerialEngineReceiver
{
public:
    void dataReady(QByteArray&& bytes) override
    {
        dataReadyCalls.append(std::move(bytes));
    }
    void closeNotification() override
    {
        ++closeCount;
    }
    void exceptionNotification(AndroidSerial::JavaExceptionKind kind, const QString& message) override
    {
        exceptions.append({kind, message});
    }

    struct Exc { AndroidSerial::JavaExceptionKind kind; QString message; };
    QList<QByteArray> dataReadyCalls;
    int closeCount = 0;
    QList<Exc> exceptions;
};

AndroidSerial::SerialParameters defaultParams()
{
    return AndroidSerial::SerialParameters{
        .baudRate = 57600,
        .dataBits = 8,
        .stopBits = 1,
        .parity   = AndroidSerial::NoParity,
    };
}

}  // namespace

void MockQSerialPortEngineTest::_openClose_transitionsState()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);

    QCOMPARE(eng.state(), IQSerialPortEngine::State::Closed);
    QVERIFY(!eng.isOpen());

    QVERIFY(eng.open(QStringLiteral("/dev/mock0"), defaultParams(), AndroidSerial::NoFlowControl, /*assertDtr=*/true));
    QCOMPARE(eng.state(), IQSerialPortEngine::State::Open);
    QVERIFY(eng.isOpen());
    QCOMPARE(eng.lastPortName(), QStringLiteral("/dev/mock0"));
    QCOMPARE(eng.dtrAsserted(), true);

    eng.close();
    QCOMPARE(eng.state(), IQSerialPortEngine::State::Closed);
    QVERIFY(!eng.isOpen());
}

void MockQSerialPortEngineTest::_openShouldFail_returnsFalseLeavesClosed()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);
    eng.setOpenShouldFail(true);

    QVERIFY(!eng.open(QStringLiteral("/dev/mock0"), defaultParams(), 0, true));
    QCOMPARE(eng.state(), IQSerialPortEngine::State::Closed);
}

void MockQSerialPortEngineTest::_setters_failBeforeOpen()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);

    QVERIFY(!eng.setDataTerminalReady(true));
    QVERIFY(!eng.setRequestToSend(true));
    QVERIFY(!eng.setBreak(true));
    QVERIFY(!eng.setFlowControl(AndroidSerial::RtsCtsFlowControl));
    QVERIFY(!eng.setParameters(defaultParams()));
    QVERIFY(!eng.purgeBuffers(true, true));
}

void MockQSerialPortEngineTest::_setters_succeedAfterOpen()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);
    QVERIFY(eng.open(QStringLiteral("/dev/mock0"), defaultParams(), 0, false));

    QVERIFY(eng.setDataTerminalReady(true));
    QCOMPARE(eng.dtrAsserted(), true);

    QVERIFY(eng.setRequestToSend(true));
    QCOMPARE(eng.rtsAsserted(), true);

    QVERIFY(eng.setBreak(true));
    QCOMPARE(eng.breakSet(), true);

    QVERIFY(eng.setFlowControl(AndroidSerial::RtsCtsFlowControl));
    QCOMPARE(eng.flowControl(), AndroidSerial::RtsCtsFlowControl);
}

void MockQSerialPortEngineTest::_writeSync_recordsBytes_returnsLength()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);
    QVERIFY(eng.open(QStringLiteral("/dev/mock0"), defaultParams(), 0, true));

    const QByteArray payload("hello", 5);
    const int written = eng.writeSync(QSpan<const char>(payload.constData(), payload.size()), 1000);
    QCOMPARE(written, 5);
    QCOMPARE(eng.syncWrites().size(), 1);
    QCOMPARE(eng.syncWrites().first(), payload);
}

void MockQSerialPortEngineTest::_writeAsync_recordsBytes_returnsLength()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);
    QVERIFY(eng.open(QStringLiteral("/dev/mock0"), defaultParams(), 0, true));

    const QByteArray payload("xyz", 3);
    const int written = eng.writeAsync(QSpan<const char>(payload.constData(), payload.size()), 1000);
    QCOMPARE(written, 3);
    QCOMPARE(eng.asyncWrites().size(), 1);
    QCOMPARE(eng.asyncWrites().first(), payload);
}

void MockQSerialPortEngineTest::_writeFailure_returnsInjectedValue()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);
    QVERIFY(eng.open(QStringLiteral("/dev/mock0"), defaultParams(), 0, true));

    eng.setNextWriteFailure(-1);
    const QByteArray payload("fail", 4);
    QCOMPARE(eng.writeSync(QSpan<const char>(payload.constData(), payload.size()), 1000), -1);
    // Second call returns to default (length).
    QCOMPARE(eng.writeSync(QSpan<const char>(payload.constData(), payload.size()), 1000), 4);
}

void MockQSerialPortEngineTest::_purgeBuffers_clearsInputStaging()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);
    QVERIFY(eng.open(QStringLiteral("/dev/mock0"), defaultParams(), 0, true));

    // No staging API directly; verify the purge succeeds and waitForReadyRead returns empty.
    QVERIFY(eng.purgeBuffers(/*input=*/true, /*output=*/true));
    QCOMPARE(eng.waitForReadyRead(0, 0), QByteArray());
}

void MockQSerialPortEngineTest::_waitForReadyRead_drainsStaging()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);
    QVERIFY(eng.open(QStringLiteral("/dev/mock0"), defaultParams(), 0, true));

    // simulateRead delivers via the receiver, not via waitForReadyRead — that path mirrors
    // the production async-read flow. waitForReadyRead is the blocking wait pulled from
    // staging; the mock's _staged starts empty, so it returns empty.
    QCOMPARE(eng.waitForReadyRead(0, 0), QByteArray());
}

void MockQSerialPortEngineTest::_simulateRead_invokesDataReady()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);

    const QByteArray payload("\x01\x02\x03", 3);
    eng.simulateRead(payload);

    QCOMPARE(rx.dataReadyCalls.size(), 1);
    QCOMPARE(rx.dataReadyCalls.first(), payload);
}

void MockQSerialPortEngineTest::_simulateClose_invokesCloseNotification()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);

    eng.simulateClose();
    eng.simulateClose();
    QCOMPARE(rx.closeCount, 2);
}

void MockQSerialPortEngineTest::_simulateException_invokesExceptionNotification()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);

    eng.simulateException(AndroidSerial::JavaExceptionKind::Resource, QStringLiteral("cable yanked"));
    QCOMPARE(rx.exceptions.size(), 1);
    QCOMPARE(rx.exceptions.first().kind, AndroidSerial::JavaExceptionKind::Resource);
    QCOMPARE(rx.exceptions.first().message, QStringLiteral("cable yanked"));
}

void MockQSerialPortEngineTest::_readThread_lifecycle()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);

    // Cannot start before open.
    QVERIFY(!eng.startReadThread());
    QVERIFY(!eng.readThreadRunning());

    QVERIFY(eng.open(QStringLiteral("/dev/mock0"), defaultParams(), 0, true));
    QVERIFY(eng.startReadThread());
    QVERIFY(eng.readThreadRunning());

    QVERIFY(eng.stopReadThread());
    QVERIFY(!eng.readThreadRunning());

    // close() also clears the running flag.
    QVERIFY(eng.startReadThread());
    eng.close();
    QVERIFY(!eng.readThreadRunning());
}

void MockQSerialPortEngineTest::_controlLines_returnsInjectedMask()
{
    RecordingReceiver rx;
    QObject owner;
    MockQSerialPortEngine eng(&rx, &owner);

    eng.setControlLinesMask(0xDEAD);
    QCOMPARE(eng.controlLinesMask(), 0xDEADu);
}
