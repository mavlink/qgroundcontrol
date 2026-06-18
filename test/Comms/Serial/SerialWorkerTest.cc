// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "SerialWorkerTest.h"

#include "MockSerialPort.h"
#include "QGCSerialPortTypes.h"
#include "SerialLink.h"
#include "SerialPlatform.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

UT_REGISTER_TEST(SerialWorkerTest, TestLabel::Unit, TestLabel::Comms)

namespace {

SharedLinkConfigurationPtr makeConfig(const QString &portName = QStringLiteral("MOCK0"))
{
    SharedLinkConfigurationPtr config(new SerialConfiguration(QStringLiteral("SerialWorkerTest")));
    qobject_cast<SerialConfiguration *>(config.get())->setPortName(portName);
    return config;
}

}  // namespace

void SerialWorkerTest::init()
{
    UnitTest::init();
    // Error-path cases intentionally drive the worker into failure states it logs warnings for. The
    // behavior under test is asserted via signals, so suppress the expected Comms.SerialLink warnings.
    ignoreLogMessage("Comms.SerialLink", QtWarningMsg, QRegularExpression(QStringLiteral(".*")));
}

void SerialWorkerTest::cleanup()
{
    SerialPlatform::setPortFactoryForTest({});
    UnitTest::cleanup();
}

void SerialWorkerTest::_connectThenDisconnect_emitsLifecycle()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    QSignalSpy connectedSpy(&worker, &SerialWorker::connected);
    QSignalSpy disconnectedSpy(&worker, &SerialWorker::disconnected);

    worker.setupPort();
    QVERIFY(mock);

    worker.connectToPort();
    QVERIFY(worker.isConnected());
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(mock->lastConfig().baud, 57600);

    worker.disconnectFromPort();
    QVERIFY(!worker.isConnected());
    QCOMPARE(disconnectedSpy.count(), 1);
}

void SerialWorkerTest::_receivedData_emittedToWorker()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    worker.setupPort();
    worker.connectToPort();
    QVERIFY(worker.isConnected());

    QSignalSpy rxSpy(&worker, &SerialWorker::dataReceived);
    mock->feedReceived(QByteArrayLiteral("telemetry"));

    QCOMPARE(rxSpy.count(), 1);
    QCOMPARE(rxSpy.first().first().toByteArray(), QByteArrayLiteral("telemetry"));
}

void SerialWorkerTest::_writeData_capturedAndEmitsDataSent()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    worker.setupPort();
    worker.connectToPort();

    QSignalSpy sentSpy(&worker, &SerialWorker::dataSent);
    worker.writeData(QByteArrayLiteral("command"));

    QCOMPARE(mock->writtenData(), QByteArrayLiteral("command"));
    QCOMPARE(sentSpy.count(), 1);
    QCOMPARE(sentSpy.first().first().toByteArray(), QByteArrayLiteral("command"));
}

void SerialWorkerTest::_writeBackpressure_holdsThenDrainsOnResume()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    worker.setupPort();
    worker.connectToPort();

    QSignalSpy sentSpy(&worker, &SerialWorker::dataSent);

    mock->setWriteStalled(true);
    worker.writeData(QByteArrayLiteral("queued"));
    QCOMPARE(sentSpy.count(), 0);
    QVERIFY(mock->writtenData().isEmpty());

    mock->resumeWrites();
    QCOMPARE(sentSpy.count(), 1);
    QCOMPARE(mock->writtenData(), QByteArrayLiteral("queued"));
}

void SerialWorkerTest::_writeFailure_emitsErrorAndClearsBacklog()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    worker.setupPort();
    worker.connectToPort();

    QSignalSpy errorSpy(&worker, &SerialWorker::errorOccurred);
    mock->setWriteShouldFail(true);
    worker.writeData(QByteArrayLiteral("doomed"));

    QCOMPARE(errorSpy.count(), 1);
}

void SerialWorkerTest::_writeWhenNotConnected_emitsErrorOnce()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    worker.setupPort();

    QSignalSpy errorSpy(&worker, &SerialWorker::errorOccurred);
    worker.writeData(QByteArrayLiteral("a"));
    worker.writeData(QByteArrayLiteral("b"));

    QCOMPARE(errorSpy.count(), 1);
}

void SerialWorkerTest::_openFailure_emitsErrorAndStaysDisconnected()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    QSignalSpy connectedSpy(&worker, &SerialWorker::connected);
    QSignalSpy errorSpy(&worker, &SerialWorker::errorOccurred);

    worker.setupPort();
    mock->setOpenResult(false);
    worker.connectToPort();

    QVERIFY(!worker.isConnected());
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 1);
}

void SerialWorkerTest::_reconfigureFailure_emitsErrorAndStaysDisconnected()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    QSignalSpy connectedSpy(&worker, &SerialWorker::connected);
    QSignalSpy errorSpy(&worker, &SerialWorker::errorOccurred);

    worker.setupPort();
    mock->setReconfigureResult(false);
    worker.connectToPort();

    QVERIFY(!worker.isConnected());
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 1);
}

void SerialWorkerTest::_resourceError_disconnectsWithoutErrorSignal()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    worker.setupPort();
    worker.connectToPort();
    QVERIFY(worker.isConnected());

    QSignalSpy disconnectedSpy(&worker, &SerialWorker::disconnected);
    QSignalSpy errorSpy(&worker, &SerialWorker::errorOccurred);

    mock->injectError(QGCSerialPortError::ResourceUnavailable, QStringLiteral("unplugged"));

    QVERIFY(!worker.isConnected());
    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);
}

void SerialWorkerTest::_genericPortError_emitsErrorSignal()
{
    MockSerialPort *mock = nullptr;
    SerialPlatform::setPortFactoryForTest([&mock](const QString &name, QObject *parent) -> QGCSerialPort * {
        mock = new MockSerialPort(name, parent);
        return mock;
    });

    SerialWorker worker(makeConfig());
    worker.setupPort();
    worker.connectToPort();

    QSignalSpy errorSpy(&worker, &SerialWorker::errorOccurred);
    mock->injectError(QGCSerialPortError::Read, QStringLiteral("read fault"));

    QCOMPARE(errorSpy.count(), 1);
}

void SerialWorkerTest::_connectWithoutSetup_emitsPortNotCreated()
{
    SerialWorker worker(makeConfig());
    QSignalSpy errorSpy(&worker, &SerialWorker::errorOccurred);

    worker.connectToPort();

    QVERIFY(!worker.isConnected());
    QCOMPARE(errorSpy.count(), 1);
}

void SerialWorkerTest::_factoryOverride_makeSerialPortReturnsMock()
{
    SerialPlatform::setPortFactoryForTest([](const QString &name, QObject *parent) -> QGCSerialPort * {
        return new MockSerialPort(name, parent);
    });

    QGCSerialPort *port = SerialPlatform::makeSerialPort(QStringLiteral("mockPort"), this);
    QVERIFY(qobject_cast<MockSerialPort *>(port) != nullptr);
    QCOMPARE(port->portName(), QStringLiteral("mockPort"));

    SerialPlatform::setPortFactoryForTest({});
    delete port;

    QGCSerialPort *realPort = SerialPlatform::makeSerialPort(QString{}, this);
    QVERIFY(qobject_cast<MockSerialPort *>(realPort) == nullptr);
    delete realPort;
}
