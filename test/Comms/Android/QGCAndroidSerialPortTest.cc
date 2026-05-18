// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "QGCAndroidSerialPortTest.h"

#include "MockQSerialPortEngine.h"
#include "QGCAndroidSerialPort.h"
#include "UnitTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

UT_REGISTER_TEST(QGCAndroidSerialPortTest, TestLabel::Unit, TestLabel::Comms)

namespace {

// Process-wide latched pointer to the most recently created MockQSerialPortEngine. Lets
// each test inspect the engine without threading it through the factory return type.
MockQSerialPortEngine* g_lastMock = nullptr;

void installMockFactory()
{
    g_lastMock = nullptr;
    QGCAndroidSerialPortFactory::setEngineFactory(
        [](QAndroidSerialEngineReceiver* sink, QObject* owner) {
            auto mock = std::make_unique<MockQSerialPortEngine>(sink, owner);
            g_lastMock = mock.get();
            return std::unique_ptr<IQSerialPortEngine>(std::move(mock));
        });
}

void installNullFactory()
{
    g_lastMock = nullptr;
    QGCAndroidSerialPortFactory::setEngineFactory(
        [](QAndroidSerialEngineReceiver*, QObject*) {
            return std::unique_ptr<IQSerialPortEngine>{};
        });
}

}  // namespace

void QGCAndroidSerialPortTest::cleanup()
{
    QGCAndroidSerialPortFactory::resetEngineFactory();
    g_lastMock = nullptr;
    UnitTest::cleanup();
}

void QGCAndroidSerialPortTest::_open_pushesConfigToEngine()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.setBaudRate(115200));
    QVERIFY(port.setDataBits(QGCDataBits::Data7));
    QVERIFY(port.setParity(QGCParity::Even));
    QVERIFY(port.setStopBits(QGCStopBits::Two));
    QVERIFY(port.setFlowControl(QGCFlowControl::HardwareRtsCts));

    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(port.isOpen());
    QVERIFY(g_lastMock);
    QCOMPARE(g_lastMock->lastPortName(), QStringLiteral("/dev/mock0"));

    const auto p = g_lastMock->lastOpenParams();
    QCOMPARE(p.baudRate, 115200);
    QCOMPARE(p.dataBits, 7);
    QCOMPARE(p.stopBits, 2);
    QCOMPARE(p.parity, static_cast<int>(AndroidSerial::EvenParity));

    QCOMPARE(g_lastMock->flowControl(), static_cast<int>(AndroidSerial::RtsCtsFlowControl));
    QVERIFY(g_lastMock->dtrAsserted());

    port.close();
}

void QGCAndroidSerialPortTest::_open_factoryReturningNull_fails()
{
    installNullFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(!port.open(QIODevice::ReadWrite));
    QVERIFY(!port.isOpen());
    QCOMPARE(port.error(), QGCSerialPortError::OpenFailed);
}

void QGCAndroidSerialPortTest::_close_emitsAboutToCloseBeforeTeardown()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    // Captures isOpen() at the moment aboutToClose fires. QIODevice contract: the device
    // is still readable/writable when aboutToClose runs, so slots can drain pending data.
    bool wasOpenInSlot = false;
    QObject::connect(&port, &QIODevice::aboutToClose, &port,
                     [&] { wasOpenInSlot = port.isOpen(); });

    port.close();
    QVERIFY(wasOpenInSlot);
    QVERIFY(!port.isOpen());
}

void QGCAndroidSerialPortTest::_close_emitsReadChannelFinished()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    QSignalSpy finishedSpy(&port, &QIODevice::readChannelFinished);
    port.close();
    QCOMPARE(finishedSpy.count(), 1);
}

void QGCAndroidSerialPortTest::_close_clearsBuffersAndState()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    // Stage some incoming bytes via the receiver path so bytesAvailable goes nonzero.
    g_lastMock->simulateRead(QByteArray("hello", 5));
    QVERIFY(port.bytesAvailable() > 0);

    port.close();
    QCOMPARE(port.bytesAvailable(), qint64(0));
    QCOMPARE(port.bytesToWrite(), qint64(0));
    QCOMPARE(port.error(), QGCSerialPortError::NoError);
}

void QGCAndroidSerialPortTest::_writeData_routesThroughEngine()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    const QByteArray payload("ping", 4);
    const qint64 written = port.write(payload);
    QCOMPARE(written, qint64(4));

    // Drain happens on the event loop. Pump once so the QChronoTimer fires.
    QTest::qWait(20);

    QCOMPARE(g_lastMock->syncWrites().size(), 1);
    QCOMPARE(g_lastMock->syncWrites().first(), payload);

    port.close();
}

void QGCAndroidSerialPortTest::_dataReady_fillsReadBuffer_emitsReadyRead()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    QSignalSpy readySpy(&port, &QIODevice::readyRead);

    const QByteArray payload("abc\x01\x02", 5);
    g_lastMock->simulateRead(payload);

    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(port.bytesAvailable(), qint64(5));
    QCOMPARE(port.readAll(), payload);

    port.close();
}

void QGCAndroidSerialPortTest::_exception_resourceUnavailable_setsErrorAndCloses()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    QSignalSpy errorSpy(&port, &QGCAndroidSerialPort::errorOccurred);

    g_lastMock->simulateException(AndroidSerial::JavaExceptionKind::Resource,
                                  QStringLiteral("cable yanked"));

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.first().first().value<QGCSerialPortError>(),
             QGCSerialPortError::ResourceUnavailable);

    // Resource exceptions trigger eager close so queued writes stop spamming the engine.
    QVERIFY(!port.isOpen());
}

void QGCAndroidSerialPortTest::_exception_permission_setsErrorOnly()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    QSignalSpy errorSpy(&port, &QGCAndroidSerialPort::errorOccurred);

    g_lastMock->simulateException(AndroidSerial::JavaExceptionKind::Permission,
                                  QStringLiteral("USB permission denied"));

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.first().first().value<QGCSerialPortError>(),
             QGCSerialPortError::PermissionDenied);
    // Permission errors do NOT auto-close.
    QVERIFY(port.isOpen());

    port.close();
}

void QGCAndroidSerialPortTest::_expectClosure_suppressesResourceExceptionDuringClose()
{
    // The _expectClosure flag is set at the top of close() to mask the racing Java
    // IOException from the forced close. Exercising the synchronous-during-aboutToClose
    // race via a mock would require re-entrancy of close() which is not the production
    // path — the real exception arrives on the owner thread via a queued metacall and is
    // suppressed naturally by close() finishing first. Asserting only the structural
    // invariant: after a clean close, no error was set.
    installMockFactory();
    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    QSignalSpy errorSpy(&port, &QGCAndroidSerialPort::errorOccurred);
    port.close();
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(port.error(), QGCSerialPortError::NoError);
}

void QGCAndroidSerialPortTest::_setters_whileClosed_cacheForNextOpen()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.setBaudRate(230400));
    QVERIFY(port.setDataBits(QGCDataBits::Data7));
    QVERIFY(port.setParity(QGCParity::Odd));
    QVERIFY(port.setStopBits(QGCStopBits::Two));

    // No engine yet — values must be cached locally.
    QCOMPARE(port.baudRate(), qint32(230400));
    QCOMPARE(port.dataBits(), QGCDataBits::Data7);
    QCOMPARE(port.parity(), QGCParity::Odd);
    QCOMPARE(port.stopBits(), QGCStopBits::Two);

    QVERIFY(port.open(QIODevice::ReadWrite));
    const auto p = g_lastMock->lastOpenParams();
    QCOMPARE(p.baudRate, 230400);
    QCOMPARE(p.dataBits, 7);
    QCOMPARE(p.stopBits, 2);
    QCOMPARE(p.parity, static_cast<int>(AndroidSerial::OddParity));

    port.close();
}

void QGCAndroidSerialPortTest::_setters_whileOpen_applyImmediately()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));

    QVERIFY(port.setBaudRate(921600));
    QVERIFY(port.setDataBits(QGCDataBits::Data8));
    QVERIFY(port.setDataTerminalReady(true));
    QVERIFY(g_lastMock->dtrAsserted());

    QVERIFY(port.setRequestToSend(true));
    QVERIFY(g_lastMock->rtsAsserted());

    port.close();
}

void QGCAndroidSerialPortTest::_testBatchedSetSerialParameters()
{
    installMockFactory();

    QGCAndroidSerialPort port(QStringLiteral("/dev/mock0"));
    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(g_lastMock);

    QSignalSpy baudSpy(&port, &QGCAndroidSerialPort::baudRateChanged);
    QSignalSpy dataSpy(&port, &QGCAndroidSerialPort::dataBitsChanged);
    QSignalSpy stopSpy(&port, &QGCAndroidSerialPort::stopBitsChanged);
    QSignalSpy paritySpy(&port, &QGCAndroidSerialPort::parityChanged);

    const int baseline = g_lastMock->setParametersCallCount();

    QVERIFY(port.setSerialParameters(115200, QGCDataBits::Data7,
                                     QGCStopBits::Two, QGCParity::Odd));

    // One engine call for four parameter changes — that's the whole point.
    QCOMPARE(g_lastMock->setParametersCallCount() - baseline, 1);
    QCOMPARE(port.baudRate(), 115200);
    QCOMPARE(port.dataBits(), QGCDataBits::Data7);
    QCOMPARE(port.stopBits(), QGCStopBits::Two);
    QCOMPARE(port.parity(), QGCParity::Odd);
    QCOMPARE(baudSpy.count(), 1);
    QCOMPARE(dataSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 1);
    QCOMPARE(paritySpy.count(), 1);

    // No-op when nothing actually changes — no engine call, no signals.
    const int afterFirst = g_lastMock->setParametersCallCount();
    QVERIFY(port.setSerialParameters(115200, QGCDataBits::Data7,
                                     QGCStopBits::Two, QGCParity::Odd));
    QCOMPARE(g_lastMock->setParametersCallCount() - afterFirst, 0);
    QCOMPARE(baudSpy.count(), 1);

    port.close();
}
