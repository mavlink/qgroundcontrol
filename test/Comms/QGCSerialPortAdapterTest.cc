// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "QGCSerialPortAdapterTest.h"

#include "MockQSerialPortEngine.h"
#include "QGCAndroidSerialPort.h"
#include "QGCSerialPortAdapter.h"
#include "UnitTest.h"

#include <QtCore/QByteArray>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

UT_REGISTER_TEST(QGCSerialPortAdapterTest, TestLabel::Unit, TestLabel::Comms)

namespace {

MockQSerialPortEngine* g_lastMock = nullptr;

void installMockEngine()
{
    g_lastMock = nullptr;
    QGCAndroidSerialPortFactory::setEngineFactory(
        [](QAndroidSerialEngineReceiver* sink, QObject* owner) {
            auto mock = std::make_unique<MockQSerialPortEngine>(sink, owner);
            g_lastMock = mock.get();
            return std::unique_ptr<IQSerialPortEngine>(std::move(mock));
        });
}

}  // namespace

void QGCSerialPortAdapterTest::cleanup()
{
    QGCSerialPortAdapter::setForceAndroidBackendForTests(false);
    QGCAndroidSerialPortFactory::resetEngineFactory();
    g_lastMock = nullptr;
    UnitTest::cleanup();
}

void QGCSerialPortAdapterTest::_defaultBackend_isHost()
{
    // Without the override, the host adapter wraps QSerialPort. The error() path returns
    // NoError before open(), and isOpen() is false. We can't fully exercise this without
    // a real port, so we just verify construction succeeds and yields a usable adapter.
    QGCSerialPortAdapter adapter;
    QVERIFY(!adapter.isOpen());
    QCOMPARE(adapter.error(), QGCSerialPortAdapter::NoError);
}

void QGCSerialPortAdapterTest::_forceAndroidBackend_routesThroughMockEngine()
{
    QGCSerialPortAdapter::setForceAndroidBackendForTests(true);
    installMockEngine();

    QGCSerialPortAdapter adapter;
    adapter.setPortName(QStringLiteral("/dev/mock0"));
    QVERIFY(adapter.setBaudRate(115200));
    QVERIFY(adapter.setDataBits(8));
    QVERIFY(adapter.setParity(0));
    QVERIFY(adapter.setStopBits(1));
    QVERIFY(adapter.setFlowControl(0));

    QVERIFY(adapter.open(QIODevice::ReadWrite));
    QVERIFY(adapter.isOpen());
    QVERIFY(g_lastMock);
    QCOMPARE(g_lastMock->lastPortName(), QStringLiteral("/dev/mock0"));
    QCOMPARE(g_lastMock->lastOpenParams().baudRate, 115200);

    adapter.close();
    QVERIFY(!adapter.isOpen());
}

void QGCSerialPortAdapterTest::_forceAndroidBackend_writeAndRead()
{
    QGCSerialPortAdapter::setForceAndroidBackendForTests(true);
    installMockEngine();

    QGCSerialPortAdapter adapter;
    adapter.setPortName(QStringLiteral("/dev/mock0"));
    QVERIFY(adapter.open(QIODevice::ReadWrite));

    // Write side: adapter.write → QGCAndroidSerialPort.writeData → drain timer →
    // engine.writeSync. Drain timer needs the event loop to fire.
    const QByteArray payload("ping", 4);
    QCOMPARE(adapter.write(payload.constData(), payload.size()), qint64(4));
    QTest::qWait(20);
    QCOMPARE(g_lastMock->syncWrites().size(), 1);
    QCOMPARE(g_lastMock->syncWrites().first(), payload);

    // Read side: mock delivers to receiver → QGCAndroidSerialPort.dataReady →
    // emits readyRead → adapter forwards.
    QSignalSpy readySpy(&adapter, &QGCSerialPortAdapter::readyRead);
    const QByteArray rx("pong", 4);
    g_lastMock->simulateRead(rx);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(adapter.readAll(), rx);

    adapter.close();
}

void QGCSerialPortAdapterTest::_forceAndroidBackend_resourceError_propagates()
{
    QGCSerialPortAdapter::setForceAndroidBackendForTests(true);
    installMockEngine();

    QGCSerialPortAdapter adapter;
    adapter.setPortName(QStringLiteral("/dev/mock0"));
    QVERIFY(adapter.open(QIODevice::ReadWrite));

    QSignalSpy errorSpy(&adapter, &QGCSerialPortAdapter::errorOccurred);

    g_lastMock->simulateException(AndroidSerial::JavaExceptionKind::Resource,
                                  QStringLiteral("cable yanked"));

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.first().first().value<QGCSerialPortAdapter::Error>(),
             QGCSerialPortAdapter::ResourceError);
    // Resource exception triggers eager close in QGCAndroidSerialPort, which the adapter
    // surfaces via isOpen() going false.
    QVERIFY(!adapter.isOpen());
}
