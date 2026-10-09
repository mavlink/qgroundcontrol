#include "GPSReceiverConnectionPolicyTest.h"

#include <utility>

#include <QtTest/QSignalSpy>

#include "GPSReceiver.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"

using namespace GPSTest;

namespace {
constexpr const char* kReceiverLog = "GPS.Receiver.GPSReceiver";

GPSReceiver::Configuration tcpConfiguration(bool autoConnect = true)
{
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    useTcp(configuration);
    configuration.autoConnect = autoConnect;
    return configuration;
}

#ifndef QGC_NO_SERIAL_LINK
/// A configured base without a saved device, which Connect automatically discovers on USB.
GPSReceiver::Configuration discoveryConfiguration()
{
    return GPSTest::serialConfiguration(gpsReceiverManufacturerForType(GPSType::ublox), {}, 115200);
}

GPSReceiver::Configuration savedSerialConfiguration(bool autoConnect = true)
{
    auto configuration = GPSTest::serialConfiguration(kPassiveManufacturer, QStringLiteral("/test/saved"), 4800);
    configuration.autoConnect = autoConnect;
    return configuration;
}
#endif

/// A receiver whose workers are scripted, over a clock the test advances and, with serial links, serial devices the
/// test plugs in.
struct PolicyHarness
{
#ifdef QGC_NO_SERIAL_LINK
    explicit PolicyHarness(const GPSReceiver::Configuration& configuration)
        : scripted(&scheduler)
#else
    explicit PolicyHarness(const GPSReceiver::Configuration& configuration, QList<SerialPortManager::Port> listed = {})
        : serial(std::move(listed))
        , scripted(&scheduler, {.serialPorts = &serial.manager})
#endif
    {
        receiver.setConfiguration(configuration);
    }

    ~PolicyHarness()
    {
        receiver.shutdown();
        releaseRetired();
    }

    ScriptedReceiverWorker* worker() const { return scripted.workers.current(); }

    qsizetype attempts() const { return scripted.workers.count(); }

    void update() { receiver.tick(); }

    /// Runs discovery past the delay before a listed port is connected.
    [[nodiscard]] bool discover()
    {
        update();
        const bool advanced = scheduler.advanceBy(std::chrono::seconds(6));
        update();
        return advanced;
    }

    void releaseRetired() const { scripted.workers.finishRetired(); }

    ManualScheduler scheduler;
#ifndef QGC_NO_SERIAL_LINK
    TestSerialPorts serial;
    SerialPortManager& ports = serial.manager;
#endif
    ScriptedGPSReceiver scripted;
    GPSReceiver& receiver = scripted.receiver;
};
}  // namespace

bool GPSReceiverConnectionPolicyTest::_retryPending(const GPSReceiver& receiver)
{
    return receiver._connection->_retryDeadlineUs.has_value();
}

void GPSReceiverConnectionPolicyTest::_lostConnectionRetriesWithBackoff()
{
    PolicyHarness harness(tcpConfiguration());
    GPSReceiver& receiver = harness.receiver;
    const auto loseConnection = [&](GPSConnectionError error) {
        expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
        harness.worker()->fail(error);
        verifyExpectedLogMessage();
        QVERIFY(!receiver.hasReceiver());
        QVERIFY(receiver.reconnecting());
        QVERIFY(_retryPending(receiver));
    };

    QVERIFY(receiver.connectReceiver());
    QCOMPARE(harness.attempts(), 1);
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("rtk.test:2101"));
    QCOMPARE(harness.worker()->type(), GPSType::passive);
    QCOMPARE(harness.worker()->capturedConfig().baudRate, GPSTransport::BRIDGE_BAUDRATE);
    harness.worker()->ready();
    QVERIFY(receiver.facts()->telemetryAvailable());
    QVERIFY(!receiver.reconnecting());

    // The lost session and the reconnect it starts are one receiver change.
    QSignalSpy changes(&receiver, &GPSReceiver::receiverChanged);
    loseConnection(GPSConnectionError::DeviceError);
    QCOMPARE(changes.size(), 1);
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver connection lost. Reconnecting automatically."));
    harness.update();
    QCOMPARE(harness.attempts(), 1);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(1)));
    harness.update();
    QCOMPARE(harness.attempts(), 2);
    // Flash-save consent is never reused by an automatic attempt.
    QVERIFY(!harness.worker()->capturedConfig().allowPersistentChanges);

    loseConnection(GPSConnectionError::OpenFailed);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1999)));
    harness.update();
    QCOMPARE(harness.attempts(), 2);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1)));
    harness.update();
    QCOMPARE(harness.attempts(), 3);
    harness.worker()->ready();
    QVERIFY(receiver.facts()->telemetryAvailable());
    QVERIFY(!receiver.reconnecting());

    // Reaching the receiver restarts the backoff.
    loseConnection(GPSConnectionError::DeviceError);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(1)));
    harness.update();
    QCOMPARE(harness.attempts(), 4);

    receiver.disconnectReceiver();
    QVERIFY(!receiver.reconnecting());
    QVERIFY(!_retryPending(receiver));
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(60)));
    harness.update();
    QCOMPARE(harness.attempts(), 4);
}

void GPSReceiverConnectionPolicyTest::_connectConfiguredValidationErrors_data()
{
    QTest::addColumn<QString>("reason");
    for (const auto* reason :
         {"unknown-manufacturer", "already-connected", "tcp-host", "tcp-port", "udp-port", "base-over-udp"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
#ifndef QGC_NO_SERIAL_LINK
    for (const auto* reason :
         {"serial-empty", "serial-missing", "serial-bootloader", "invalid-baud", "passive-auto-baud"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
#endif
}

void GPSReceiverConnectionPolicyTest::_connectConfiguredValidationErrors()
{
    QFETCH(QString, reason);
    auto configuration = receiverConfiguration();
    useTcp(configuration);
    configuration.autoConnect = false;
    if (reason == QStringLiteral("unknown-manufacturer")) {
        configuration.baseReceiverManufacturer = 99;
    } else if (reason == QStringLiteral("tcp-host")) {
        configuration.tcpHost.clear();
    } else if (reason == QStringLiteral("tcp-port")) {
        configuration.tcpPort = 0;
    } else if (reason == QStringLiteral("udp-port")) {
        configuration.receiverRole = RTKSettings::Passive;
        configuration.connectionType = RTKSettings::Udp;
        configuration.udpPort = 0;
    } else if (reason == QStringLiteral("base-over-udp")) {
        configuration.connectionType = RTKSettings::Udp;
        configuration.udpPort = 14401;
    }
#ifdef QGC_NO_SERIAL_LINK
    PolicyHarness harness(configuration);
#else
    QList<SerialPortManager::Port> inventory{rtkPort()};
    if (reason.startsWith(QStringLiteral("serial-")) || reason == QStringLiteral("invalid-baud")) {
        configuration = GPSTest::serialConfiguration(gpsReceiverManufacturerForType(GPSType::ublox),
                                                     QStringLiteral("/test/rtk"), 115200);
        configuration.autoConnect = false;
    }
    if (reason == QStringLiteral("serial-empty")) {
        configuration.serialDevice.clear();
    } else if (reason == QStringLiteral("serial-missing")) {
        inventory.clear();
    } else if (reason == QStringLiteral("serial-bootloader")) {
        inventory.first().bootloader = true;
    } else if (reason == QStringLiteral("invalid-baud")) {
        configuration.serialBaudRate = 1000;
    } else if (reason == QStringLiteral("passive-auto-baud")) {
        configuration = savedSerialConfiguration(false);
        configuration.serialBaudRate = 0;
        inventory = {serialPort(configuration.serialDevice)};
    }
    PolicyHarness harness(configuration, inventory);
#endif
    if (reason == QStringLiteral("already-connected")) {
        QVERIFY(harness.receiver.connectReceiver());
    }
    const qsizetype attempts = harness.attempts();

    QVERIFY(!harness.receiver.connectReceiver());
    QVERIFY(!harness.receiver.errorMessage().isEmpty());
    QCOMPARE(harness.attempts(), attempts);
}

void GPSReceiverConnectionPolicyTest::_configurationChangeCancelsRetry_data()
{
    QTest::addColumn<QString>("change");
    QTest::newRow("connection") << QStringLiteral("connection");
    QTest::newRow("auto-connect-off") << QStringLiteral("auto-connect-off");
}

void GPSReceiverConnectionPolicyTest::_configurationChangeCancelsRetry()
{
    QFETCH(QString, change);
    auto configuration = tcpConfiguration();
    PolicyHarness harness(configuration);
    GPSReceiver& receiver = harness.receiver;
    QVERIFY(receiver.connectReceiver());
    harness.worker()->ready();
    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    harness.worker()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    QVERIFY(receiver.reconnecting());
    QVERIFY(_retryPending(receiver));

    if (change == QStringLiteral("connection")) {
        ++configuration.tcpPort;
    } else {
        configuration.autoConnect = false;
    }
    QSignalSpy changes(&receiver, &GPSReceiver::receiverChanged);
    QSignalSpy errors(&receiver, &GPSReceiver::errorMessageChanged);
    receiver.setConfiguration(configuration);
    QCOMPARE(changes.size(), 1);
    QVERIFY(!receiver.reconnecting());
    // Nothing reconnects any more, so the message about it is withdrawn.
    QVERIFY(receiver.errorMessage().isEmpty());
    QCOMPARE(errors.size(), 1);
    QVERIFY(!_retryPending(receiver));
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(60)));
    harness.update();
    QCOMPARE(harness.attempts(), 1);
}

void GPSReceiverConnectionPolicyTest::_sessionEndMessages_data()
{
    QTest::addColumn<GPSConnectionError>("error");
    QTest::addColumn<QString>("detail");
    QTest::addColumn<bool>("autoConnect");
    QTest::addColumn<QString>("log");
    QTest::addColumn<QString>("message");
    const QString lost = QStringLiteral("Receiver read failed");
    const QString broken = QStringLiteral("UBX checksum mismatch");
    QTest::newRow("lost-retrying")
        << GPSConnectionError::DeviceError << lost << true << QStringLiteral("session ended")
        << GPSReceiver::tr("Receiver connection lost: %1. Reconnecting automatically.").arg(lost);
    QTest::newRow("protocol-retrying")
        << GPSConnectionError::ProtocolError << broken << true << QStringLiteral("session ended")
        << GPSReceiver::tr("Receiver protocol error: %1. Reconnecting automatically.").arg(broken);
    QTest::newRow("lost") << GPSConnectionError::DeviceError << lost << false
                          << QStringLiteral("GPS device error, connection lost")
                          << GPSReceiver::tr("Receiver connection lost: %1. Check the device and reconnect.").arg(lost);
    QTest::newRow("protocol")
        << GPSConnectionError::ProtocolError << broken << false << QStringLiteral("protocol error")
        << GPSReceiver::tr("Receiver protocol error: %1. Check the receiver type and reconnect.").arg(broken);
    QTest::newRow("protocol-without-detail")
        << GPSConnectionError::ProtocolError << QString() << false << QStringLiteral("protocol error")
        << GPSReceiver::tr("Receiver protocol error. Check the receiver type and reconnect.");
}

void GPSReceiverConnectionPolicyTest::_sessionEndMessages()
{
    QFETCH(GPSConnectionError, error);
    QFETCH(QString, detail);
    QFETCH(bool, autoConnect);
    QFETCH(QString, log);
    QFETCH(QString, message);
    PolicyHarness harness(tcpConfiguration(false));
    GPSReceiver& receiver = harness.receiver;
    harness.update();
    QCOMPARE(harness.attempts(), 0);
    QVERIFY(receiver.connectReceiver());
    harness.worker()->ready();
    // The setting when the session ends decides the retry; changing only the setting keeps the connection.
    receiver.setConfiguration(tcpConfiguration(autoConnect));
    QVERIFY(receiver.facts()->telemetryAvailable());
    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(log));
    harness.worker()->fail(error, detail);
    verifyExpectedLogMessage();
    QCOMPARE(receiver.errorMessage(), message);
    QCOMPARE(receiver.reconnecting(), autoConnect);
    QCOMPARE(_retryPending(receiver), autoConnect);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(60)));
    harness.update();
    QCOMPARE(harness.attempts(), autoConnect ? 2 : 1);
}

void GPSReceiverConnectionPolicyTest::_pollingTicksReceiver()
{
    PolicyHarness harness(tcpConfiguration());
    GPSReceiver& receiver = harness.receiver;
    bool suspended = true;
    int polls = 0;
    receiver.startConnectionPolling([&] {
        ++polls;
        return suspended;
    });
    QVERIFY(harness.scheduler.advanceBy(GPSReceiver::POLL_INTERVAL));
    QCOMPARE(polls, 1);
    QCOMPARE(harness.attempts(), 0);

    suspended = false;
    QVERIFY(harness.scheduler.advanceBy(GPSReceiver::POLL_INTERVAL - std::chrono::milliseconds(1)));
    QCOMPARE(harness.attempts(), 0);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(harness.attempts(), 1);

    // A later tick retries the lost connection.
    harness.worker()->ready();
    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    harness.worker()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    QVERIFY(harness.scheduler.advanceBy(GPSReceiver::POLL_INTERVAL));
    QCOMPARE(harness.attempts(), 2);

    receiver.shutdown();
    const int pollsAtShutdown = polls;
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(60)));
    QCOMPARE(polls, pollsAtShutdown);
    QCOMPARE(harness.attempts(), 2);
}

#ifndef QGC_NO_SERIAL_LINK

void GPSReceiverConnectionPolicyTest::_discoveryWaitsAndReconnects()
{
    auto configuration = discoveryConfiguration();
    PolicyHarness harness(configuration, {rtkPort()});
    GPSReceiver& receiver = harness.receiver;
    const auto loseConnection = [&](const QString& log) {
        expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(log));
        harness.worker()->fail(GPSConnectionError::DeviceError);
        verifyExpectedLogMessage();
        harness.releaseRetired();
    };

    harness.update();
    QCOMPARE(harness.attempts(), 0);
    QVERIFY(!receiver.reconnecting());
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.update();
    QCOMPARE(harness.attempts(), 1);
    // Discovery detects the rate and never passes flash-save consent.
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("/test/rtk"));
    QCOMPARE(harness.worker()->capturedConfig().baudRate, 0U);
    QVERIFY(!harness.worker()->capturedConfig().allowPersistentChanges);
    harness.update();
    QCOMPARE(harness.attempts(), 1);

    loseConnection(QStringLiteral("session ended"));
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver connection lost. Reconnecting automatically."));
    QVERIFY(_retryPending(receiver));
    QVERIFY(!receiver.reconnecting());

    // A receiver unplugged while its retry waits is left to discovery.
    harness.serial.plug({});
    harness.update();
    QVERIFY(!_retryPending(receiver));
    QCOMPARE(harness.attempts(), 1);
    harness.serial.plug({rtkPort()});
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 2);

    // So is a connected receiver that is unplugged.
    harness.serial.plug({});
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver unplugged."));
    QVERIFY(!_retryPending(receiver));
    harness.releaseRetired();
    harness.serial.plug({rtkPort()});
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 3);

    // Turning the setting off keeps the connection, but nothing reconnects it.
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.hasReceiver());
    loseConnection(QStringLiteral("GPS device error, connection lost"));
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver connection lost. Check the device and reconnect."));
    QVERIFY(!_retryPending(receiver));
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 3);
}

void GPSReceiverConnectionPolicyTest::_savedReceiverReplacesDiscovery_data()
{
    QTest::addColumn<int>("connection");
    QTest::addColumn<QString>("device");
    QTest::addColumn<QString>("endpoint");
    QTest::newRow("nothing-saved") << static_cast<int>(RTKSettings::Serial) << QString()
                                   << QStringLiteral("/test/known");
    // Generic USB serial adapters are never identified as receivers, so they connect only when selected.
    QTest::newRow("serial") << static_cast<int>(RTKSettings::Serial) << QStringLiteral("/test/ch340")
                            << QStringLiteral("/test/ch340");
    QTest::newRow("tcp") << static_cast<int>(RTKSettings::Tcp) << QString() << QStringLiteral("rtk.test:2101");
}

void GPSReceiverConnectionPolicyTest::_savedReceiverReplacesDiscovery()
{
    QFETCH(int, connection);
    QFETCH(QString, device);
    QFETCH(QString, endpoint);
    // Quectel receivers often sit behind generic USB serial adapters; the saved base mode is one it supports.
    auto configuration = GPSTest::serialConfiguration(gpsReceiverManufacturerForType(GPSType::quectel), device, 115200);
    configuration.connectionType = static_cast<RTKSettings::ConnectionType>(connection);
    configuration.tcpHost = QStringLiteral("rtk.test");
    configuration.tcpPort = 2101;
    const QList<SerialPortManager::Port> inventory{serialPort(QStringLiteral("/test/ch340")),
                                                   serialPort(QStringLiteral("/test/ftdi")),
                                                   rtkPort(QStringLiteral("/test/known"))};
    PolicyHarness harness(configuration, inventory);

    QVERIFY(harness.discover());
    QVERIFY2(harness.receiver.errorMessage().isEmpty(), qPrintable(harness.receiver.errorMessage()));
    QCOMPARE(harness.attempts(), 1);
    QCOMPARE(harness.receiver.activeEndpoint(), endpoint);
    QCOMPARE(harness.worker()->type(), GPSType::quectel);
    for (const auto& port : inventory) {
        QCOMPARE(harness.ports.canReservePort(port.systemLocation), port.systemLocation != endpoint);
    }
}

void GPSReceiverConnectionPolicyTest::_excludedPorts_data()
{
    QTest::addColumn<QString>("reason");
    for (const auto* reason : {"bootloader", "busy", "passive-role", "single-port", "other-board"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
}

void GPSReceiverConnectionPolicyTest::_excludedPorts()
{
    QFETCH(QString, reason);
    auto configuration = discoveryConfiguration();
    if (reason == QStringLiteral("passive-role")) {
        configuration.receiverRole = RTKSettings::Passive;
    }
    SerialPortManager::Port port = rtkPort();
    port.bootloader = reason == QStringLiteral("bootloader");
    if (reason == QStringLiteral("other-board")) {
        port.boardType = QGCSerialPortInfo::BoardTypePixhawk;
    }
    PolicyHarness harness(configuration, {port});
    (void) harness.ports.availablePorts();
    SerialPortManager::ReservationPtr reservation;
    if (reason == QStringLiteral("busy")) {
        reservation = harness.ports.reservePort(port.systemLocation);
    } else if (reason == QStringLiteral("single-port")) {
        harness.ports.setSinglePortOnly(true);
        reservation = harness.ports.reservePort(QStringLiteral("/test/mavlink"));
    }
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 0);
}

void GPSReceiverConnectionPolicyTest::_retryBacksOffAndRespectsReservations()
{
    PolicyHarness harness(discoveryConfiguration(), {rtkPort()});
    const auto failAttempt = [&] {
        expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
        harness.worker()->fail(GPSConnectionError::OpenFailed);
        verifyExpectedLogMessage();
        harness.releaseRetired();
        QVERIFY(!harness.receiver.hasReceiver());
        QVERIFY(_retryPending(harness.receiver));
    };
    const auto retriesAfter = [&](std::chrono::milliseconds delay) {
        const qsizetype attempts = harness.attempts();
        QVERIFY(harness.scheduler.advanceBy(delay - std::chrono::milliseconds(1)));
        harness.update();
        QCOMPARE(harness.attempts(), attempts);
        QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1)));
        harness.update();
        QCOMPARE(harness.attempts(), attempts + 1);
    };

    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 1);
    failAttempt();
    retriesAfter(std::chrono::seconds(1));
    failAttempt();

    // A port another connection holds is skipped until it is free, without advancing the backoff.
    auto claim = harness.ports.reservePort(QStringLiteral("/test/rtk"));
    QVERIFY(claim);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(2)));
    harness.update();
    QCOMPARE(harness.attempts(), 2);
    claim.reset();
    harness.update();
    QCOMPARE(harness.attempts(), 3);
    for (const auto delay : {std::chrono::seconds(4), std::chrono::seconds(8), std::chrono::seconds(16),
                             std::chrono::seconds(30), std::chrono::seconds(30)}) {
        failAttempt();
        retriesAfter(delay);
    }
    failAttempt();

    auto configuration = discoveryConfiguration();
    configuration.receiverRole = RTKSettings::Passive;
    QCOMPARE(harness.receiver.errorMessage(),
             GPSReceiver::tr("Failed to open the receiver. Reconnecting automatically."));
    harness.receiver.setConfiguration(configuration);
    QVERIFY(!_retryPending(harness.receiver));
    // Nothing reconnects any more, so the message about it is withdrawn.
    QVERIFY(harness.receiver.errorMessage().isEmpty());
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(30)));
    const qsizetype attempts = harness.attempts();
    harness.update();
    QCOMPARE(harness.attempts(), attempts);
}

void GPSReceiverConnectionPolicyTest::_compositeReceiverSelection_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<bool>("connectSecond");
    QTest::newRow("busy-primary-blocks-duplicate") << QStringLiteral("duplicate") << false;
    QTest::newRow("nmea-interface-remains-eligible") << QStringLiteral("nmea-label") << true;
    QTest::newRow("mavlink-sibling") << QStringLiteral("mavlink-sibling") << true;
    QTest::newRow("bootloader-sibling") << QStringLiteral("bootloader") << true;
    QTest::newRow("unknown-identities") << QStringLiteral("unknown") << true;
    QTest::newRow("distinct-devices") << QStringLiteral("distinct") << true;
}

void GPSReceiverConnectionPolicyTest::_compositeReceiverSelection()
{
    QFETCH(QString, scenario);
    QFETCH(bool, connectSecond);
    SerialPortManager::Port first = rtkPort(QStringLiteral("/test/receiver-first"));
    first.physicalDeviceId = QStringLiteral("1:2:serial");
    SerialPortManager::Port second = first;
    second.systemLocation = QStringLiteral("/test/receiver-second");
    second.portName = QStringLiteral("receiver-second");
    if (scenario == QStringLiteral("nmea-label")) {
        second.description = QStringLiteral("NMEA interface");
    } else if (scenario == QStringLiteral("mavlink-sibling")) {
        first.boardType = QGCSerialPortInfo::BoardTypePixhawk;
    } else if (scenario == QStringLiteral("bootloader")) {
        first.bootloader = true;
    } else if (scenario == QStringLiteral("unknown")) {
        first.physicalDeviceId.clear();
        second.physicalDeviceId.clear();
    } else if (scenario == QStringLiteral("distinct")) {
        second.physicalDeviceId = QStringLiteral("1:2:other");
    }
    PolicyHarness harness(discoveryConfiguration(), {first, second});
    auto claim = harness.ports.reservePort(first.systemLocation);
    QVERIFY(claim);
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), connectSecond ? 1 : 0);
    if (connectSecond) {
        QCOMPARE(harness.receiver.activeEndpoint(), second.systemLocation);
    }
}

void GPSReceiverConnectionPolicyTest::_disconnectPausesAutomaticConnection()
{
    auto configuration = discoveryConfiguration();
    PolicyHarness harness(configuration, {rtkPort()});
    GPSReceiver& receiver = harness.receiver;
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 1);

    receiver.disconnectReceiver();
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(receiver.errorMessage().isEmpty());
    harness.releaseRetired();
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 1);

    // Turning the setting back on resumes discovery.
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    configuration.autoConnect = true;
    receiver.setConfiguration(configuration);
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 2);

    // So does the user's Connect, after which a lost connection is reconnected again.
    receiver.disconnectReceiver();
    harness.releaseRetired();
    receiver.setConfiguration(tcpConfiguration());
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(60)));
    harness.update();
    QCOMPARE(harness.attempts(), 2);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(harness.attempts(), 3);
    harness.worker()->ready();
    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    harness.worker()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    QVERIFY(receiver.reconnecting());
}

void GPSReceiverConnectionPolicyTest::_savedSerialWaitsForReturningPort_data()
{
    QTest::addColumn<bool>("autoConnect");
    QTest::newRow("connect-automatically") << true;
    QTest::newRow("manual-only") << false;
}

void GPSReceiverConnectionPolicyTest::_savedSerialWaitsForReturningPort()
{
    QFETCH(bool, autoConnect);
    const QString device = QStringLiteral("/test/saved");
    const QList<SerialPortManager::Port> present{serialPort(QStringLiteral("/test/other")), serialPort(device)};
    PolicyHarness harness(savedSerialConfiguration(autoConnect));
    GPSReceiver& receiver = harness.receiver;

    // At startup the absent receiver is waited for.
    harness.update();
    QCOMPARE(harness.attempts(), 0);
    QCOMPARE(receiver.reconnecting(), autoConnect);
    if (!autoConnect) {
        harness.serial.plug(present);
        harness.update();
        QCOMPARE(harness.attempts(), 0);
        QVERIFY(receiver.connectReceiver());
        harness.worker()->ready();
        harness.serial.plug({});
        QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver unplugged. Select a device and reconnect."));
        QVERIFY(!receiver.reconnecting());
        return;
    }
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Waiting for the receiver on %1.").arg(device));
    harness.serial.plug(present);
    harness.update();
    QCOMPARE(harness.attempts(), 1);
    QCOMPARE(receiver.activeEndpoint(), device);
    QVERIFY(harness.ports.isPortReserved(device));
    QVERIFY(!harness.ports.isPortReserved(QStringLiteral("/test/other")));
    QCOMPARE(harness.worker()->type(), GPSType::passive);
    QCOMPARE(harness.worker()->capturedConfig().baudRate, 4800U);
    QVERIFY(!harness.worker()->capturedConfig().allowPersistentChanges);
    harness.worker()->ready();
    QVERIFY(receiver.errorMessage().isEmpty());

    // Unplugged, it reconnects as soon as its port returns instead of after a backoff.
    const QString unplugged = GPSReceiver::tr("Receiver unplugged. Reconnecting when it is plugged back in.");
    harness.serial.plug({});
    QCOMPARE(receiver.errorMessage(), unplugged);
    QVERIFY(receiver.reconnecting());
    harness.releaseRetired();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(30)));
    harness.update();
    QCOMPARE(harness.attempts(), 1);
    QCOMPARE(receiver.errorMessage(), unplugged);
    harness.serial.plug(present);
    harness.update();
    QCOMPARE(harness.attempts(), 2);
    QCOMPARE(receiver.activeEndpoint(), device);

    // Turned off while the receiver is unplugged, nothing waits for it, and no message says so.
    harness.serial.plug({});
    QCOMPARE(receiver.errorMessage(), unplugged);
    receiver.setConfiguration(savedSerialConfiguration(false));
    QVERIFY(!receiver.reconnecting());
    QVERIFY(receiver.errorMessage().isEmpty());
}

void GPSReceiverConnectionPolicyTest::_retryOfListedPortBacksOff()
{
    PolicyHarness harness(savedSerialConfiguration(), {serialPort(QStringLiteral("/test/saved"))});
    GPSReceiver& receiver = harness.receiver;
    harness.update();
    QCOMPARE(harness.attempts(), 1);
    harness.worker()->ready();
    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    harness.worker()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    harness.releaseRetired();

    // The device never left, so a port another program holds is not a returning port and waits out the backoff.
    auto claim = harness.ports.reservePort(QStringLiteral("/test/saved"));
    QVERIFY(claim);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(1)));
    harness.update();
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("The selected serial device is already in use."));
    QVERIFY(receiver.reconnecting());
    claim.reset();
    harness.update();
    QCOMPARE(harness.attempts(), 1);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1999)));
    harness.update();
    QCOMPARE(harness.attempts(), 1);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1)));
    harness.update();
    QCOMPARE(harness.attempts(), 2);
}

void GPSReceiverConnectionPolicyTest::_discoveryUsesSavedManufacturer_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<QString>("boardName");
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("consentRefused");
    // The USB identity only admits the port; its name never chooses or replaces the saved family.
    QTest::newRow("automatic-generic-name") << GPS_AUTOMATIC_MANUFACTURER << QStringLiteral("CP2102 USB to UART")
                                            << static_cast<int>(GPSType::automatic) << true;
    QTest::newRow("automatic-named") << GPS_AUTOMATIC_MANUFACTURER << QStringLiteral("u-blox GNSS receiver")
                                     << static_cast<int>(GPSType::automatic) << true;
    // Quectel saved, a u-blox receiver attached: the identity check fails, and flash consent would not help.
    QTest::newRow("quectel-on-ublox") << gpsReceiverManufacturerForType(GPSType::quectel)
                                      << QStringLiteral("u-blox GNSS receiver") << static_cast<int>(GPSType::quectel)
                                      << false;
    QTest::newRow("quectel-generic-name")
        << gpsReceiverManufacturerForType(GPSType::quectel) << QStringLiteral("CP2102 USB to UART")
        << static_cast<int>(GPSType::quectel) << true;
}

void GPSReceiverConnectionPolicyTest::_discoveryUsesSavedManufacturer()
{
    QFETCH(int, manufacturer);
    QFETCH(QString, boardName);
    QFETCH(int, type);
    QFETCH(bool, consentRefused);
    SerialPortManager::Port port = rtkPort();
    port.boardName = boardName;
    auto configuration = discoveryConfiguration();
    configuration.baseReceiverManufacturer = manufacturer;
    PolicyHarness harness(configuration, {port});
    GPSReceiver& receiver = harness.receiver;

    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 1);
    auto* worker = harness.worker();
    QCOMPARE(static_cast<int>(worker->type()), type);
    QCOMPARE(worker->capturedConfig().baudRate, 0U);
    QVERIFY(!worker->capturedConfig().allowPersistentChanges);
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("/test/rtk"));
    if (manufacturer == GPS_AUTOMATIC_MANUFACTURER) {
        worker->detected(GPSType::quectel);
        QCOMPARE(receiver.detectedReceiver(), QStringLiteral("Quectel"));
    }

    // Only a receiver that asked for flash-save consent, which discovery never passes, gets advice on giving it.
    const QString detail =
        consentRefused ? QStringLiteral("LG290P role mismatch: save the requested base role externally")
                       : QStringLiteral(
                             "No verified LG290P(03) identity; receiver configuration was not changed. UBX frames were "
                             "received; this looks like a u-blox receiver");
    expectLogMessage(kReceiverLog, QtWarningMsg,
                     QRegularExpression(consentRefused ? QStringLiteral("did not accept configuration")
                                                       : QStringLiteral("session ended")));
    worker->fail(consentRefused ? GPSConnectionError::ConsentRequired : GPSConnectionError::ConfigFailed, detail);
    verifyExpectedLogMessage();
    QVERIFY(!receiver.hasReceiver());
    if (consentRefused) {
        QVERIFY(receiver.errorMessage().startsWith(QStringLiteral("Receiver configuration failed: %1.").arg(detail)));
        QVERIFY2(receiver.errorMessage().contains(QStringLiteral("connect manually and allow flash save and restart")),
                 qPrintable(receiver.errorMessage()));
    } else {
        QCOMPARE(receiver.errorMessage(),
                 GPSReceiver::tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail));
    }
    QVERIFY(_retryPending(receiver));
}

#endif

UT_REGISTER_TEST(GPSReceiverConnectionPolicyTest, TestLabel::Unit)
