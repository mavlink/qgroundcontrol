#include "GPSBluetoothLinkTest.h"

#include <array>
#include <thread>

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QMetaObject>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>

#include "BluetoothConfiguration.h"
#include "GPSBluetoothLink.h"
#include "GPSCancellation.h"
#include "GPSReceiverBluetooth.h"
#include "GPSTransport.h"
#include "Receiver/Support/ScriptedBluetooth.h"

using namespace GPSTest;

namespace {
const QString kAddress = QStringLiteral("00:11:22:AA:BB:CC");
const QString kName = QStringLiteral("Survey receiver");
}  // namespace

void GPSBluetoothLinkTest::_linkCarriesBytesAndStopsItsWorker()
{
    // The Bluetooth end of a session's pipe on this thread, and the session's transport on a thread of its own, as a
    // receiver worker runs it.
    ScriptedBluetooth bluetooth;
    bluetooth.onConnect = [](ScriptedBluetoothWorker* worker) { worker->accept(); };
    GPSReceiverBluetooth side(nullptr, bluetooth.platform());
    QVERIFY(side.supported());
    QVERIFY(GPSReceiverBluetooth::validAddress(kAddress));
    // An address is six bytes, and without one there is nothing to connect.
    QVERIFY(!GPSReceiverBluetooth::validAddress(QStringLiteral("00:11:22:AA:BB")));
    QVERIFY(!side.transportFactory(QStringLiteral("00:11:22:AA:BB"), kName));
    const auto factory = side.transportFactory(kAddress, kName);
    QVERIFY(factory);
    GPSCancelSource stop;
    QSemaphore wrote;
    QSemaphore ended;
    QSemaphore release;
    GPSOpenResult opened;
    GPSWriteResult written;
    QByteArray reply;
    GPSReadResult last;
    bool fatal = false;
    std::thread session([&] {
        const auto transport = factory(stop.token());
        opened = transport->open();
        written = transport->write("command", QDeadlineTimer(QDeadlineTimer::Forever));
        wrote.release();
        std::array<uint8_t, 16> buffer{};
        last = transport->read(buffer, TestTimeout::longDuration());
        reply = QByteArray(reinterpret_cast<const char*>(buffer.data()), last.bytesRead);
        last = transport->read(buffer, TestTimeout::longDuration());
        fatal = transport->fatalError();
        ended.release();
        // The session keeps its transport, as a worker does while it ends.
        release.acquire();
    });
    const auto join = qScopeGuard([&] {
        stop.cancel();
        release.release();
        session.join();
    });

    // What the session writes reaches the worker, whose report of it completes the write.
    QTRY_VERIFY_WITH_TIMEOUT(wrote.available() > 0, TestTimeout::mediumMs());
    QCOMPARE(opened.status, GPSOpenStatus::Opened);
    QCOMPARE(written.status, GPSWriteStatus::Completed);
    QCOMPARE(written.writtenBytes, 7);
    QCOMPARE(bluetooth.asked().written, QByteArray("command"));
    QCOMPARE(bluetooth.asked().device.address(), QBluetoothAddress(kAddress));
    QCOMPARE(bluetooth.asked().device.name(), kName);

    // What the worker receives reaches the session, and so does the loss of the link, with the worker's reason.
    bluetooth.worker()->receive("reply");
    bluetooth.worker()->lose(QStringLiteral("Link lost"));
    QTRY_VERIFY_WITH_TIMEOUT(ended.available() > 0, TestTimeout::mediumMs());
    QCOMPARE(reply, QByteArray("reply"));
    QCOMPARE(last.status, GPSReadStatus::Closed);
    QCOMPARE(last.detail, QStringLiteral("Link lost"));
    QVERIFY(fatal);

    // The worker is stopped when the link ends, so that it neither reconnects nor searches the device for another
    // service on its own, while the session still has its transport.
    QTRY_COMPARE_WITH_TIMEOUT(bluetooth.asked().disconnects, 1, TestTimeout::mediumMs());
    QVERIFY(bluetooth.worker());
    // With the session, the end of the link goes, and its worker with it.
    release.release();
    QTRY_VERIFY_WITH_TIMEOUT(bluetooth.workers.last().isNull(), TestTimeout::mediumMs());
    QCOMPARE(bluetooth.asked().connects, 1);
}

void GPSBluetoothLinkTest::_linkRefusesToOpen_data()
{
    QTest::addColumn<QString>("refusal");
    QTest::addColumn<QString>("reason");
    // The device does not answer, as the socket of the classic worker reports it.
    QTest::newRow("device") << QStringLiteral("device") << QStringLiteral("Host is down");
    QTest::newRow("permission") << QStringLiteral("permission")
                                << GPSBluetoothLink::tr("Bluetooth permission was denied");
    QTest::newRow("no-worker") << QStringLiteral("no-worker") << GPSBluetoothLink::tr("Bluetooth is not available");
    // A build without Bluetooth sockets has no link to open.
    QTest::newRow("unsupported") << QStringLiteral("unsupported") << QString();
}

void GPSBluetoothLinkTest::_linkRefusesToOpen()
{
    QFETCH(QString, refusal);
    QFETCH(QString, reason);
    ScriptedBluetooth bluetooth;
    bluetooth.permitted = refusal != QStringLiteral("permission");
    bluetooth.supported = refusal != QStringLiteral("unsupported");
    bluetooth.onConnect = [reason](ScriptedBluetoothWorker* worker) { worker->refuse(reason); };
    GPSBluetoothPlatform platform = bluetooth.platform();
    if (refusal == QStringLiteral("no-worker")) {
        platform.createWorker = [](const QBluetoothDeviceInfo&, QObject*) -> BluetoothWorker* { return nullptr; };
    }
    GPSReceiverBluetooth side(nullptr, platform);
    const auto factory = side.transportFactory(kAddress, kName);
    if (!bluetooth.supported) {
        QVERIFY(!side.supported());
        QVERIFY(!factory);
        QVERIFY(!side.discovery());
        return;
    }
    QVERIFY(factory);
    GPSCancelSource stop;
    QSemaphore done;
    GPSOpenResult opened;
    bool fatal = false;
    std::thread session([&] {
        const auto transport = factory(stop.token());
        opened = transport->open();
        fatal = transport->fatalError();
        done.release();
    });
    const auto join = qScopeGuard([&] {
        stop.cancel();
        session.join();
    });

    // The session's open fails with the reason, at once instead of when its wait for the link runs out.
    QTRY_VERIFY_WITH_TIMEOUT(done.available() > 0, TestTimeout::mediumMs());
    QCOMPARE(opened.status, GPSOpenStatus::Error);
    QCOMPARE(opened.detail, reason);
    QVERIFY(fatal);
    // The end of the link goes with the session, and a worker that tried is stopped first.
    QTRY_VERIFY_WITH_TIMEOUT(side.findChildren<GPSBluetoothLink*>().isEmpty(), TestTimeout::mediumMs());
    if (refusal == QStringLiteral("device")) {
        QCOMPARE(bluetooth.asked().connects, 1);
        QVERIFY(bluetooth.asked().disconnects >= 1);
        QVERIFY(bluetooth.workers.last().isNull());
    } else {
        // Without the permission, or a worker, no connection is asked for.
        QVERIFY(bluetooth.workers.isEmpty());
    }
}

void GPSBluetoothLinkTest::_discoveryIsTheLinkConfiguration()
{
    // The build's Bluetooth side, which scans nothing and connects nothing before it is asked to.
    GPSReceiverBluetooth side;
    if (!side.supported()) {
        QSKIP("Receivers do not connect over Bluetooth in this build");
    }
    QObject* const discovery = side.discovery();
    QVERIFY(discovery);
    // Created once, and the side's.
    QCOMPARE(side.discovery(), discovery);
    QCOMPARE(discovery->parent(), &side);

    // It is the configuration of a Bluetooth link in its classic mode, the mode of the serial port profile.
    auto* const configuration = qobject_cast<BluetoothConfiguration*>(discovery);
    QVERIFY(configuration);
    QCOMPARE(configuration->mode(), BluetoothConfiguration::BluetoothMode::ModeClassic);
    QVERIFY(!configuration->scanning());

    // What the device picker uses of it: the model and its entries, the scan and its error.
    const QMetaObject* const meta = discovery->metaObject();
    QVERIFY(meta->indexOfProperty("devicesModel") >= 0);
    QVERIFY(meta->indexOfProperty("scanning") >= 0);
    QVERIFY(meta->indexOfMethod("startScan()") >= 0);
    QVERIFY(meta->indexOfMethod("stopScan()") >= 0);
    QVERIFY(meta->indexOfSignal("errorOccurred(QString)") >= 0);
    QBluetoothDeviceInfo found(QBluetoothAddress(kAddress), kName, 0);
    found.setCoreConfigurations(QBluetoothDeviceInfo::BaseRateCoreConfiguration);
    QVERIFY(QMetaObject::invokeMethod(discovery, "_deviceDiscovered", Qt::DirectConnection,
                                      Q_ARG(QBluetoothDeviceInfo, found)));
    const QVariantList devices = discovery->property("devicesModel").toList();
    QCOMPARE(devices.size(), 1);
    QCOMPARE(devices.first().toMap().value(QStringLiteral("name")).toString(), kName);
    QCOMPARE(devices.first().toMap().value(QStringLiteral("address")).toString(), kAddress);
}

UT_REGISTER_TEST(GPSBluetoothLinkTest, TestLabel::Unit)
