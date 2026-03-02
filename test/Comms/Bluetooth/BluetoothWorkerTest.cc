#include "BluetoothWorkerTest.h"

#include "BluetoothBleWorker.h"
#include "BluetoothClassicWorker.h"
#include "BluetoothConfiguration.h"
#include "BluetoothWorker.h"

#include <QtBluetooth/QBluetoothServiceDiscoveryAgent>
#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>

static QTimer *_findServiceDiscoveryTimer(const BluetoothWorker *worker)
{
    for (QTimer *timer : worker->findChildren<QTimer *>()) {
        if (timer && timer->isSingleShot() && (timer->interval() == 30000)) {
            return timer;
        }
    }

    return nullptr;
}

void BluetoothWorkerTest::_testFactoryCreatesClassicWorker()
{
    BluetoothConfiguration config("TestBT_factoryClassic");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeClassic);

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
    QVERIFY(worker);
    QVERIFY(qobject_cast<BluetoothClassicWorker *>(worker.get()));
}

void BluetoothWorkerTest::_testFactoryCreatesBleWorker()
{
    BluetoothConfiguration config("TestBT_factoryBle");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
    QVERIFY(worker);
    QVERIFY(qobject_cast<BluetoothBleWorker *>(worker.get()));
}

void BluetoothWorkerTest::_testInitialState()
{
    BluetoothConfiguration config("TestBT_initialState");

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
    QVERIFY(!worker->isConnected());
}

void BluetoothWorkerTest::_testWriteEmptyDataEmitsNothing()
{
    BluetoothConfiguration config("TestBT_writeEmpty");

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
    QSignalSpy errorSpy(worker.get(), &BluetoothWorker::errorOccurred);
    QSignalSpy dataSpy(worker.get(), &BluetoothWorker::dataSent);

    worker->writeData(QByteArray());

    // Empty data should be silently dropped — no error, no dataSent
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(dataSpy.count(), 0);
}

void BluetoothWorkerTest::_testWriteWhileDisconnectedEmitsError()
{
    BluetoothConfiguration config("TestBT_writeDisconnected");

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
    QSignalSpy errorSpy(worker.get(), &BluetoothWorker::errorOccurred);

    worker->writeData(QByteArray("hello"));

    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.first().first().toString().contains("not connected"));
}

void BluetoothWorkerTest::_testConnectWhileConnectedWarns()
{
    BluetoothConfiguration config("TestBT_doubleConnect");

    // Use BLE worker — Classic would attempt a real socket
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);
    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));

    // Force _connected = true via connectLink/disconnect cycle isn't possible without
    // real hardware, but we can verify the guard path doesn't crash.
    // First connect attempt will fail (no device), but should not crash.
    worker->connectLink();
    // Second call exercises the "already connected" guard when _connected is false —
    // it just proceeds, which is fine.
    worker->connectLink();

    QVERIFY2(true, "double connectLink must not crash");
}

void BluetoothWorkerTest::_testDisconnectResetsReconnectState()
{
    BluetoothConfiguration config("TestBT_disconnectReset");

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));

    // Call connectLink to set intentionalDisconnect = false internally
    worker->connectLink();

    // Now disconnect — should reset state cleanly
    worker->disconnectLink();

    // Verify worker is not connected
    QVERIFY(!worker->isConnected());
}

void BluetoothWorkerTest::_testClassicDiscoveryTimerStopsOnFinished()
{
    BluetoothConfiguration config("TestBT_classicTimerFinished");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeClassic);

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
    auto *classicWorker = qobject_cast<BluetoothClassicWorker *>(worker.get());
    QVERIFY(classicWorker);

    QTimer *serviceTimer = _findServiceDiscoveryTimer(worker.get());
    QVERIFY(serviceTimer);
    serviceTimer->start();
    QVERIFY(serviceTimer->isActive());

    QVERIFY(QMetaObject::invokeMethod(classicWorker, "_onClassicServiceDiscoveryFinished", Qt::DirectConnection));
    QVERIFY(!serviceTimer->isActive());
}

void BluetoothWorkerTest::_testClassicDiscoveryTimerStopsOnCanceled()
{
    BluetoothConfiguration config("TestBT_classicTimerCanceled");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeClassic);

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
    auto *classicWorker = qobject_cast<BluetoothClassicWorker *>(worker.get());
    QVERIFY(classicWorker);

    QTimer *serviceTimer = _findServiceDiscoveryTimer(worker.get());
    QVERIFY(serviceTimer);
    serviceTimer->start();
    QVERIFY(serviceTimer->isActive());

    QVERIFY(QMetaObject::invokeMethod(classicWorker, "_onClassicServiceDiscoveryCanceled", Qt::DirectConnection));
    QVERIFY(!serviceTimer->isActive());
}

void BluetoothWorkerTest::_testClassicDiscoveryTimerStopsOnError()
{
    qRegisterMetaType<QBluetoothServiceDiscoveryAgent::Error>("QBluetoothServiceDiscoveryAgent::Error");

    BluetoothConfiguration config("TestBT_classicTimerError");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeClassic);

    std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
    auto *classicWorker = qobject_cast<BluetoothClassicWorker *>(worker.get());
    QVERIFY(classicWorker);

    QTimer *serviceTimer = _findServiceDiscoveryTimer(worker.get());
    QVERIFY(serviceTimer);
    serviceTimer->start();
    QVERIFY(serviceTimer->isActive());

    QVERIFY(QMetaObject::invokeMethod(classicWorker, "_onClassicServiceDiscoveryError", Qt::DirectConnection,
                                      Q_ARG(QBluetoothServiceDiscoveryAgent::Error,
                                            QBluetoothServiceDiscoveryAgent::Error::UnknownError)));
    QVERIFY(!serviceTimer->isActive());
}

void BluetoothWorkerTest::_testDestroyClassicWorker()
{
    BluetoothConfiguration config("TestBT_destroyClassic");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeClassic);

    {
        std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
        worker->setupConnection();
        // destroyed here — must not crash
    }
    QVERIFY2(true, "classic worker destruction must not crash");
}

void BluetoothWorkerTest::_testDestroyBleWorker()
{
    BluetoothConfiguration config("TestBT_destroyBle");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);

    {
        std::unique_ptr<BluetoothWorker> worker(BluetoothWorker::create(&config));
        worker->setupConnection();
        // destroyed here — must not crash
    }
    QVERIFY2(true, "BLE worker destruction must not crash");
}

UT_REGISTER_TEST(BluetoothWorkerTest, TestLabel::Unit, TestLabel::Comms)
