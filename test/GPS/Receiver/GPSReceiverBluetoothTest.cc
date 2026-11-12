#include "GPSReceiverBluetoothTest.h"

#include <chrono>
#include <cmath>

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

#include "GPSObservation.h"
#include "GPSReceiver.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSatelliteModel.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "Protocols/Support/GPSBaseStationConfigs.h"
#include "Protocols/Support/GPSProtocolTestData.h"
#include "Protocols/fixtures/GPSFixtureExpectations.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedBluetooth.h"
#ifdef QGC_GPS_RECEIVER_SIMULATOR
#include "Protocols/Hardware/GPSReceiverSimulator.h"
#endif

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
constexpr const char* kReceiverLog = "GPS.Receiver.GPSReceiver";
/// The driver of a passive receiver, which reports the read that found its link ended.
constexpr const char* kPassiveLog = "GPS.Protocols.Passive";

/// A passive receiver on the Bluetooth device of useBluetooth(), which the user connects.
GPSReceiver::Configuration bluetoothConfiguration(bool autoConnect = false)
{
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    useBluetooth(configuration);
    configuration.forwardReceiverRtcm = false;
    configuration.autoConnect = autoConnect;
    return configuration;
}
}  // namespace

void GPSReceiverBluetoothTest::_passiveReceiverOverBluetooth()
{
    ScriptedBluetooth bluetooth;
    GPSReceiver receiver(nullptr, nullptr, {.bluetooth = bluetooth.side()});
    QVERIFY(receiver.bluetoothSupported());
    const auto configuration = bluetoothConfiguration();
    receiver.setConfiguration(configuration);
    QCOMPARE(receiver.effectiveConnectionType(), RTKSettings::Bluetooth);
    QVERIFY(receiver.hasEndpoint());
    QVERIFY(receiver.connectReceiver());
    // The device's own name reads better than its address, which tells two devices of one name apart.
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("Survey receiver (00:11:22:AA:BB:CC)"));
    QCOMPARE(receiver.state(), GPSReceiver::State::Connecting);

    // The session's worker asks for the link from its thread, and the link's end connects the device on this one.
    QTRY_VERIFY_WITH_TIMEOUT(bluetooth.worker() && bluetooth.asked().connects == 1, TestTimeout::mediumMs());
    QCOMPARE(bluetooth.asked().device.address(), QBluetoothAddress(configuration.bluetoothAddress));
    QCOMPARE(bluetooth.asked().device.name(), configuration.bluetoothName);
    QVERIFY(!receiver.facts()->telemetryAvailable());
    bluetooth.worker()->accept();
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable(), TestTimeout::mediumMs());
    QCOMPARE(receiver.state(), GPSReceiver::State::Connected);

    // A u-blox receiver's recorded output arrives as a radio delivers it: in pieces that cut its messages.
    const QByteArray recording = dataFile(FIXTURE_DIR, QStringLiteral("navigation.ubx"));
    QVERIFY(!recording.isEmpty());
    for (qsizetype sent = 0; sent < recording.size(); sent += 37) {
        bluetooth.worker()->receive(recording.mid(sent, 37));
    }
    QTRY_VERIFY_WITH_TIMEOUT(receiver.acceptedPositionObservation(GPSObservation::PositionUse::GroundStation),
                             TestTimeout::mediumMs());
    const QGeoCoordinate position =
        receiver.acceptedPositionObservation(GPSObservation::PositionUse::GroundStation)->coordinate();
    QVERIFY(std::abs(position.latitude() - GPSFixture::positions[0].latitude) < 1e-6);
    QVERIFY(std::abs(position.longitude() - GPSFixture::positions[0].longitude) < 1e-6);
    QCOMPARE(receiver.detectedReceiver(), gpsInputProtocolName(GPSType::UBlox));
    QTRY_VERIFY_WITH_TIMEOUT(receiver.satellites()->count() > 0, TestTimeout::mediumMs());
    // A receiver that is not configured is sent nothing.
    QVERIFY(bluetooth.asked().written.isEmpty());

    // The loss of the link ends the session with what the Bluetooth socket said of it.
    const QString lost = QStringLiteral("The remote host closed the connection");
    expectLogMessage(kPassiveLog, QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver read failed.*%1").arg(lost)));
    expectLogMessage(kReceiverLog, QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost:.*%1").arg(lost)));
    bluetooth.worker()->lose(lost);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(receiver.errorMessage(),
             GPSReceiver::tr("Receiver connection lost: %1. Check the device and reconnect.").arg(lost));
    QCOMPARE(receiver.state(), GPSReceiver::State::Disconnected);
    // The worker is stopped, so that it does not reconnect by itself, and goes with the session's end of the link.
    QTRY_VERIFY_WITH_TIMEOUT(bluetooth.workers.last().isNull(), TestTimeout::mediumMs());
    QVERIFY(bluetooth.asked().disconnects >= 1);
    QCOMPARE(bluetooth.asked().connects, 1);
}

void GPSReceiverBluetoothTest::_openIsRefused_data()
{
    QTest::addColumn<QString>("refusal");
    QTest::addColumn<QString>("reason");
    QTest::newRow("device") << QStringLiteral("device") << QStringLiteral("Host is down");
    QTest::newRow("permission") << QStringLiteral("permission")
                                << GPSBluetoothLink::tr("Bluetooth permission was denied");
}

void GPSReceiverBluetoothTest::_openIsRefused()
{
    QFETCH(QString, refusal);
    QFETCH(QString, reason);
    ScriptedBluetooth bluetooth;
    bluetooth.permitted = refusal != QStringLiteral("permission");
    GPSReceiver receiver(nullptr, nullptr, {.bluetooth = bluetooth.side()});
    receiver.setConfiguration(bluetoothConfiguration());
    expectLogMessage(kReceiverLog, QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport:.*%1").arg(reason)));
    QVERIFY(receiver.connectReceiver());
    if (bluetooth.permitted) {
        QTRY_VERIFY_WITH_TIMEOUT(bluetooth.worker() && bluetooth.asked().connects == 1, TestTimeout::mediumMs());
        bluetooth.worker()->refuse(reason);
    }
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    // A receiver that could not be opened was never connected, and the reason is the link's.
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Failed to open the receiver: %1.").arg(reason));
    QCOMPARE(receiver.state(), GPSReceiver::State::Disconnected);
    QVERIFY(!receiver.facts()->telemetryAvailable());
    if (bluetooth.permitted) {
        // The worker's search for another service on the device, and its reconnection, are stopped.
        QTRY_VERIFY_WITH_TIMEOUT(bluetooth.workers.last().isNull(), TestTimeout::mediumMs());
        QVERIFY(bluetooth.asked().disconnects >= 1);
    } else {
        // Without the permission no connection is asked for.
        QVERIFY(bluetooth.workers.isEmpty());
    }
}

void GPSReceiverBluetoothTest::_savedDeviceReconnects()
{
    ManualScheduler scheduler;
    ScriptedBluetooth bluetooth;
    GPSReceiver receiver(nullptr, &scheduler, {.bluetooth = bluetooth.side()});
    receiver.setConfiguration(bluetoothConfiguration(true));
    const auto attempts = [&bluetooth] { return bluetooth.records.size(); };
    const auto asked = [&] {
        return QTest::qWaitFor([&] { return bluetooth.worker() && bluetooth.asked().connects == 1; },
                               TestTimeout::mediumMs());
    };
    const auto ended = [&](const QString& message) {
        const bool lost = QTest::qWaitFor([&receiver] { return !receiver.hasReceiver(); }, TestTimeout::mediumMs());
        return lost && receiver.reconnecting() && receiver.errorMessage() == message;
    };

    // A saved device is connected at startup, as a saved address is.
    receiver.tick();
    QVERIFY(receiver.hasReceiver());
    QVERIFY(asked());
    QCOMPARE(attempts(), 1);

    // A device that cannot be opened is tried again after a delay.
    const QString down = QStringLiteral("Host is down");
    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    bluetooth.worker()->refuse(down);
    QVERIFY(ended(GPSReceiver::tr("Failed to open the receiver: %1. Reconnecting automatically.").arg(down)));
    verifyExpectedLogMessage();
    receiver.tick();
    QCOMPARE(attempts(), 1);
    QVERIFY(scheduler.advanceBy(1s));
    receiver.tick();
    QVERIFY(receiver.hasReceiver());
    QVERIFY(asked());
    QCOMPARE(attempts(), 2);
    bluetooth.worker()->accept();
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable(), TestTimeout::mediumMs());

    // So is one whose link is lost: by the receiver's connection policy, with a new connection each time.
    const QString lost = QStringLiteral("The remote host closed the connection");
    expectLogMessage(kPassiveLog, QtWarningMsg, QRegularExpression(QStringLiteral("Receiver read failed")));
    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    bluetooth.worker()->lose(lost);
    QVERIFY(ended(GPSReceiver::tr("Receiver connection lost: %1. Reconnecting automatically.").arg(lost)));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(scheduler.advanceBy(1s));
    receiver.tick();
    QVERIFY(asked());
    QCOMPARE(attempts(), 3);
    for (const auto& record : std::as_const(bluetooth.records)) {
        QCOMPARE(record->connects, 1);
    }

    // Disconnect pauses that.
    receiver.disconnectReceiver();
    QVERIFY(!receiver.reconnecting());
    QVERIFY(scheduler.advanceBy(60s));
    receiver.tick();
    QCOMPARE(attempts(), 3);
    receiver.shutdown();
}

void GPSReceiverBluetoothTest::_disconnectEndsLink_data()
{
    QTest::addColumn<bool>("connected");
    // The session still waits for its link, which a Bluetooth stack can take long to refuse.
    QTest::newRow("connecting") << false;
    QTest::newRow("connected") << true;
}

void GPSReceiverBluetoothTest::_disconnectEndsLink()
{
    QFETCH(bool, connected);
    ScriptedBluetooth bluetooth;
    GPSReceiver receiver(nullptr, nullptr, {.bluetooth = bluetooth.side()});
    receiver.setConfiguration(bluetoothConfiguration());
    QVERIFY(receiver.connectReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(bluetooth.worker() && bluetooth.asked().connects == 1, TestTimeout::mediumMs());
    if (connected) {
        bluetooth.worker()->accept();
        QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable(), TestTimeout::mediumMs());
    }

    receiver.disconnectReceiver();
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(receiver.errorMessage().isEmpty());
    // The stop ends the session's wait at once. With the session, the device is disconnected and its worker goes.
    QTRY_VERIFY_WITH_TIMEOUT(bluetooth.workers.last().isNull(), TestTimeout::mediumMs());
    QVERIFY(bluetooth.asked().disconnects >= 1);
    QVERIFY(receiver.errorMessage().isEmpty());
}

void GPSReceiverBluetoothTest::_baseIsConfiguredOverBluetooth()
{
#ifdef QGC_GPS_RECEIVER_SIMULATOR
    // The simulated u-blox receiver answers the driver that configures it, behind a socket that stands in for the
    // radio of the scripted worker.
    GPSReceiverSimulator simulator(
        {.capture = {}, .captureBaudRate = 0, .receiver = UBXReceiverModel::Receiver::F9P, .secondLength = 10ms});
    QVERIFY2(simulator.listen(0), qPrintable(simulator.errorString()));
    QTcpSocket radio;
    ScriptedBluetooth bluetooth;
    bluetooth.onConnect = [&](ScriptedBluetoothWorker* worker) {
        (void) connect(&radio, &QTcpSocket::connected, worker, [worker] { worker->accept(); });
        (void) connect(&radio, &QTcpSocket::readyRead, worker, [&radio, worker] { worker->receive(radio.readAll()); });
        worker->onWrite = [&radio, worker](const QByteArray& data) {
            radio.write(data);
            worker->sent(data);
        };
        radio.connectToHost(QHostAddress(QHostAddress::LocalHost), simulator.port());
    };
    GPSReceiver receiver(nullptr, nullptr, {.bluetooth = bluetooth.side()});
    auto configuration = receiverConfiguration(gpsReceiverManufacturerForType(GPSType::UBlox));
    useBluetooth(configuration);
    configuration.base = surveyInBase(1.5, 3s);
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());

    // The driver's commands cross the pipe and the worker, each write confirmed, and the receiver's replies come
    // back: the base is configured and surveys its position.
    QTRY_VERIFY2_WITH_TIMEOUT(receiver.facts()->telemetryAvailable() || !receiver.hasReceiver(),
                              qPrintable(receiver.errorMessage()), TestTimeout::longMs());
    QVERIFY2(receiver.hasReceiver(), qPrintable(receiver.errorMessage()));
    QCOMPARE(receiver.receiverIdentity(), QStringLiteral("ZED-F9P HPG 1.32"));
    QVERIFY(!bluetooth.asked().written.isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(baseState(*receiver.facts()), GPSBaseState::Valid, TestTimeout::longMs());
    QVERIFY(simulator.wireValid());
    QVERIFY(receiver.errorMessage().isEmpty());
    receiver.disconnectReceiver();
    QTRY_VERIFY_WITH_TIMEOUT(bluetooth.workers.last().isNull(), TestTimeout::mediumMs());
#else
    QSKIP("The simulated receiver is not part of this build");
#endif
}

void GPSReceiverBluetoothTest::_effectiveConnection_data()
{
    QTest::addColumn<bool>("bluetoothLinks");
    QTest::addColumn<bool>("serialLinks");
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("effective");
#ifdef QGC_NO_SERIAL_LINK
    const int serial = RTKSettings::Tcp;
#else
    const int serial = RTKSettings::Serial;
#endif
    QTest::newRow("base") << true << true << int(RTKSettings::ConfiguredBase) << int(RTKSettings::Bluetooth);
    QTest::newRow("passive") << true << true << int(RTKSettings::Passive) << int(RTKSettings::Bluetooth);
    QTest::newRow("no-serial-links") << true << false << int(RTKSettings::Passive) << int(RTKSettings::Bluetooth);
    // A receiver without Bluetooth, as every receiver of a build without Bluetooth sockets is, does not know the
    // selection, which then connects as any selection it does not know: over a serial device, or TCP without them.
    QTest::newRow("no-bluetooth-links") << false << true << int(RTKSettings::ConfiguredBase) << serial;
    QTest::newRow("no-links") << false << false << int(RTKSettings::Passive) << int(RTKSettings::Tcp);
}

void GPSReceiverBluetoothTest::_effectiveConnection()
{
    QFETCH(bool, bluetoothLinks);
    QFETCH(bool, serialLinks);
    QFETCH(int, role);
    QFETCH(int, effective);
    ScriptedBluetooth bluetooth;
    bluetooth.supported = bluetoothLinks;
    GPSReceiver::Dependencies dependencies{.bluetooth = bluetooth.side()};
    if (!serialLinks) {
        dependencies.serial = withoutSerialLinks;
    }
    ScriptedGPSReceiver harness(nullptr, dependencies);
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration();
    configuration.receiverRole = static_cast<RTKSettings::ReceiverRole>(role);
    useBluetooth(configuration);
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    QCOMPARE(receiver.bluetoothSupported(), bluetoothLinks);
    QCOMPARE(receiver.property("bluetoothSupported").toBool(), bluetoothLinks);
    QCOMPARE(int(receiver.effectiveConnectionType()), effective);
    // A configured base connects over Bluetooth as it does over TCP.
    QVERIFY(receiver.connectionSupported() && receiver.canConnect());
    QCOMPARE(receiver.hasEndpoint(), bluetoothLinks);
    // The picker has a discovery to scan with only where the receiver has Bluetooth: its Bluetooth side's.
    QCOMPARE(receiver.bluetoothDiscovery() != nullptr, bluetoothLinks);
    QCOMPARE(receiver.property("bluetoothDiscovery").value<QObject*>(), bluetooth.discovery.data());
    QCOMPARE(receiver.connectReceiver(), bluetoothLinks);
    if (bluetoothLinks) {
        QCOMPARE(harness.workers.current()->capturedConfig().baudRate, GPSTransport::BRIDGE_BAUD_RATE);
        QCOMPARE(receiver.activeEndpoint(), QStringLiteral("Survey receiver (00:11:22:AA:BB:CC)"));
    } else {
        // The saved Bluetooth device is not what the connection is missing.
        QVERIFY(receiver.errorMessage() != GPSReceiverConnectionPolicy::tr("Select the receiver's Bluetooth device."));
        QVERIFY(!receiver.errorMessage().isEmpty());
    }
}

UT_REGISTER_TEST(GPSReceiverBluetoothTest, TestLabel::Unit)
