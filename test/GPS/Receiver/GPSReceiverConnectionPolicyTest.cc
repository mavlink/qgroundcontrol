#include "GPSReceiverConnectionPolicyTest.h"

#include <utility>

#include <QtTest/QSignalSpy>

#include "GPSReceiver.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "PositionManager.h"
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

/// A position the position manager accepts.
GPSPositionReport locatedFix(GPSFixQuality fixType)
{
    GPSPositionReport report = fixReport(fixType);
    report.navigation.latitudeDegrees = 47.25;
    report.navigation.longitudeDegrees = 8.5;
    report.navigation.horizontalAccuracyMeters = 1.0f;
    return report;
}

#ifndef QGC_NO_SERIAL_LINK
/// A configured base without a saved device, which Connect automatically discovers on USB.
GPSReceiver::Configuration discoveryConfiguration()
{
    return GPSTest::serialConfiguration(gpsReceiverManufacturerForType(GPSType::UBlox), {}, 115200);
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
        , scripted(&scheduler, {.serial = serial.side()})
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
    QCOMPARE(harness.worker()->type(), GPSType::Passive);
    QCOMPARE(harness.worker()->capturedConfig().baudRate, GPSTransport::BRIDGE_BAUD_RATE);
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
    QTest::addColumn<QString>("message");
    const auto row = [](const char* reason, const QString& message) {
        QTest::newRow(reason) << QString::fromLatin1(reason) << message;
    };
    const QString tcpAddress = GPSReceiverConnectionPolicy::tr("Enter the receiver's TCP host and port.");
    row("unknown-manufacturer", GPSReceiverConnectionPolicy::tr("Select a receiver type before connecting."));
    row("already-connected",
        GPSReceiverConnectionPolicy::tr("Disconnect the current receiver before connecting another."));
    row("tcp-host", tcpAddress);
    row("tcp-port", tcpAddress);
    row("udp-port", GPSReceiverConnectionPolicy::tr("Enter the UDP port that receives the data."));
    row("base-over-udp",
        GPSReceiverConnectionPolicy::tr("A configured base needs a serial or TCP connection. UDP only receives data."));
#ifndef QGC_NO_SERIAL_LINK
    const QString serialDevice =
        GPSReceiverConnectionPolicy::tr("Select an available serial device that is not a bootloader.");
    const QString baudRate = gpsReceiverConfigErrorText(GPSReceiverConfigError::InvalidBaudRate);
    row("serial-empty", serialDevice);
    row("serial-missing", serialDevice);
    row("serial-bootloader", serialDevice);
    row("invalid-baud", baudRate);
    // Where only one serial device can be open at a time, as with Android's USB serial driver.
    row("single-port-in-use", GPSReceiverConnectionPolicy::tr("The selected serial device is already in use."));
    row("single-port-other-in-use",
        GPSReceiverConnectionPolicy::tr("Another connection has a serial device open, and only one can be open at a "
                                        "time. Disconnect it first."));
#endif
}

void GPSReceiverConnectionPolicyTest::_connectConfiguredValidationErrors()
{
    QFETCH(QString, reason);
    QFETCH(QString, message);
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
    if (reason.startsWith(QStringLiteral("serial-")) || reason.startsWith(QStringLiteral("single-port-")) ||
        reason == QStringLiteral("invalid-baud")) {
        configuration = GPSTest::serialConfiguration(gpsReceiverManufacturerForType(GPSType::UBlox),
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
    }
    PolicyHarness harness(configuration, inventory);
    // Another connection, such as a vehicle link, holds the selected device or another one.
    (void) harness.ports.availablePorts();
    harness.ports.setSinglePortOnly(reason.startsWith(QStringLiteral("single-port-")));
    const auto vehicleLink = reason == QStringLiteral("single-port-in-use")
                                 ? harness.ports.reservePort(configuration.serialDevice)
                             : reason == QStringLiteral("single-port-other-in-use")
                                 ? harness.ports.reservePort(QStringLiteral("/test/mavlink"))
                                 : nullptr;
#endif
    if (reason == QStringLiteral("already-connected")) {
        QVERIFY(harness.receiver.connectReceiver());
    }
    const qsizetype attempts = harness.attempts();

    QVERIFY(!harness.receiver.connectReceiver());
    QCOMPARE(harness.receiver.errorMessage(), message);
    QCOMPARE(harness.attempts(), attempts);
}

void GPSReceiverConnectionPolicyTest::_udpConnectsOnlyPassiveReceivers_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::newRow("Automatic") << GPS_AUTOMATIC_MANUFACTURER;
    QTest::newRow("unknown") << 99;
    for (const GPSReceiverDescriptor& family : gpsReceiverDescriptors()) {
        QTest::newRow(QByteArray(family.name.data(), family.name.size()).constData()) << family.manufacturerId;
    }
}

void GPSReceiverConnectionPolicyTest::_udpConnectsOnlyPassiveReceivers()
{
    QFETCH(int, manufacturer);
    // UDP only receives, so its connection takes no receiver type. Whatever manufacturer the settings hold, a
    // configured base is refused before one is opened.
    auto configuration = receiverConfiguration();
    configuration.baseReceiverManufacturer = manufacturer;
    configuration.connectionType = RTKSettings::Udp;
    configuration.autoConnect = false;
    PolicyHarness harness(configuration);
    GPSReceiver& receiver = harness.receiver;
    QVERIFY(!receiver.connectReceiver());
    QVERIFY(!receiver.errorMessage().isEmpty());
    QCOMPARE(harness.attempts(), 0);

    // With the same manufacturer left in the settings, a passive receiver's connection is passive.
    configuration.receiverRole = RTKSettings::Passive;
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(harness.worker()->type(), GPSType::Passive);
    QCOMPARE(receiver.activeRole(), RTKSettings::Passive);
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

void GPSReceiverConnectionPolicyTest::_unconnectableConfigurationIsNotRetried_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<bool>("connectRefused");
    for (const bool connectRefused : {false, true}) {
        const auto row = [connectRefused](const char* scenario) {
            QTest::addRow("%s%s", scenario, connectRefused ? "-connect-refused" : "")
                << QString::fromLatin1(scenario) << connectRefused;
        };
        row("saved-tcp");
        row("saved-base-over-udp");
#ifndef QGC_NO_SERIAL_LINK
        row("saved-serial-unplugged");
        row("discovered");
#endif
    }
}

void GPSReceiverConnectionPolicyTest::_unconnectableConfigurationIsNotRetried()
{
    QFETCH(QString, scenario);
    QFETCH(bool, connectRefused);
    // A u-blox base has no receiver-managed averaging.
    auto fixed = receiverConfiguration();
    useTcp(fixed);
    auto broken = fixed;
    broken.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
    QString error = gpsReceiverConfigErrorText(GPSReceiverConfigError::UnsupportedBaseMode);
    if (scenario == QStringLiteral("saved-base-over-udp")) {
        fixed = receiverConfiguration(kPassiveManufacturer);
        fixed.connectionType = RTKSettings::Udp;
        broken = fixed;
        broken.receiverRole = RTKSettings::ConfiguredBase;
        error = GPSReceiverConnectionPolicy::tr(
            "A configured base needs a serial or TCP connection. UDP only receives data.");
    }
#ifdef QGC_NO_SERIAL_LINK
    PolicyHarness harness(broken);
#else
    QList<SerialPortManager::Port> listed;
    if (scenario == QStringLiteral("saved-serial-unplugged")) {
        fixed = GPSTest::serialConfiguration(fixed.baseReceiverManufacturer, QStringLiteral("/test/saved"), 115200);
        broken = fixed;
        broken.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
    } else if (scenario == QStringLiteral("discovered")) {
        fixed = discoveryConfiguration();
        broken = fixed;
        broken.baseReceiverManufacturer = 99;
        error = GPSReceiverConnectionPolicy::tr("Select a receiver type before connecting.");
        listed = {rtkPort()};
    }
    PolicyHarness harness(broken, listed);
#endif
    GPSReceiver& receiver = harness.receiver;
    QSignalSpy changes(&receiver, &GPSReceiver::receiverChanged);
    QSignalSpy errors(&receiver, &GPSReceiver::errorMessageChanged);

    // The reason is shown once, in place of waiting for the port or a retry, and nothing reconnects.
    for (int pass = 0; pass < 2; ++pass) {
        QVERIFY(harness.discover());
        QCOMPARE(receiver.errorMessage(), error);
        QVERIFY(!receiver.reconnecting());
        QVERIFY(receiver.statusText().isEmpty());
        QVERIFY(!_retryPending(receiver));
        QCOMPARE(harness.attempts(), 0);
        QCOMPARE(errors.size(), 1);
        QVERIFY(changes.isEmpty());
        QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(60)));
    }

    // The user's Connect is refused for the same reason, and changes nothing of that.
    if (connectRefused) {
        QVERIFY(!receiver.connectReceiver());
        QCOMPARE(receiver.errorMessage(), error);
        QCOMPARE(errors.size(), 1);
        QVERIFY(!receiver.reconnecting());
        harness.update();
        QCOMPARE(harness.attempts(), 0);
    }

    // A changed configuration is connected as at startup.
#ifndef QGC_NO_SERIAL_LINK
    if (scenario == QStringLiteral("saved-serial-unplugged")) {
        harness.serial.plug({serialPort(fixed.serialDevice)});
    }
#endif
    receiver.setConfiguration(fixed);
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(harness.discover());
    QCOMPARE(harness.attempts(), 1);
    QVERIFY(receiver.hasReceiver());
    harness.worker()->ready();

    // A lost connection is still retried with backoff.
    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    harness.worker()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    harness.releaseRetired();
    QVERIFY(_retryPending(receiver));
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(999)));
    harness.update();
    QCOMPARE(harness.attempts(), 1);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1)));
    harness.update();
    QCOMPARE(harness.attempts(), 2);
}

void GPSReceiverConnectionPolicyTest::_configurationChangeWithdrawsRefusal_data()
{
    QTest::addColumn<QString>("refused");
    QTest::addColumn<QString>("change");
    QTest::addColumn<bool>("withdrawn");
    const auto row = [](const char* refused, const char* change, bool withdrawn) {
        QTest::addRow("%s-%s", refused, change)
            << QString::fromLatin1(refused) << QString::fromLatin1(change) << withdrawn;
    };
    row("tcp-without-host", "host-entered", true);
    row("tcp-without-host", "udp-selected", true);
    // The host is still missing.
    row("tcp-without-host", "port-changed", false);
    row("tcp-without-host", "forwarding-off", false);
    // A saved receiver that still cannot connect: its changed configuration is left to automatic connection, and the
    // message stays all the same.
    row("base-over-udp", "forwarding-off", false);
    // What ended a session stays, whatever is selected next.
    row("lost-connection", "port-changed", false);
#ifndef QGC_NO_SERIAL_LINK
    row("serial-unplugged", "device-selected", true);
    row("serial-unplugged", "baud-changed", false);
    row("serial-in-use", "device-selected", true);
    // The refused device returned and its session ended: the message on display is no longer the refusal.
    row("serial-returned-and-lost", "device-selected", false);
#endif
}

void GPSReceiverConnectionPolicyTest::_configurationChangeWithdrawsRefusal()
{
    QFETCH(QString, refused);
    QFETCH(QString, change);
    QFETCH(bool, withdrawn);
    auto configuration = tcpConfiguration(false);
    if (refused == QStringLiteral("tcp-without-host")) {
        configuration.tcpHost.clear();
    } else if (refused == QStringLiteral("base-over-udp")) {
        configuration = receiverConfiguration();
        configuration.connectionType = RTKSettings::Udp;
        configuration.autoConnect = false;
    }
#ifdef QGC_NO_SERIAL_LINK
    PolicyHarness harness(configuration);
#else
    const QString other = QStringLiteral("/test/other");
    QList<SerialPortManager::Port> listed{serialPort(other)};
    if (refused.startsWith(QStringLiteral("serial-"))) {
        configuration = savedSerialConfiguration(false);
    }
    if (refused == QStringLiteral("serial-in-use")) {
        listed.append(serialPort(configuration.serialDevice));
    }
    PolicyHarness harness(configuration, listed);
    // Another connection, such as a vehicle link, holds the selected device.
    const auto vehicleLink =
        refused == QStringLiteral("serial-in-use") ? harness.ports.reservePort(configuration.serialDevice) : nullptr;
#endif
    GPSReceiver& receiver = harness.receiver;
    const auto loseConnection = [&] {
        QVERIFY(receiver.connectReceiver());
        harness.worker()->ready();
        expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("connection lost")));
        harness.worker()->fail(GPSConnectionError::DeviceError);
        verifyExpectedLogMessage();
    };
    if (refused == QStringLiteral("lost-connection")) {
        loseConnection();
    } else {
        QVERIFY(!receiver.connectReceiver());
    }
#ifndef QGC_NO_SERIAL_LINK
    if (refused == QStringLiteral("serial-returned-and-lost")) {
        QVERIFY(!receiver.errorMessage().isEmpty());
        harness.serial.plug({serialPort(other), serialPort(configuration.serialDevice)});
        loseConnection();
    }
#endif
    const QString error = receiver.errorMessage();
    QVERIFY(!error.isEmpty());
    const qsizetype attempts = harness.attempts();

    if (change == QStringLiteral("host-entered")) {
        configuration.tcpHost = QStringLiteral("rtk.test");
    } else if (change == QStringLiteral("udp-selected")) {
        configuration.connectionType = RTKSettings::Udp;
    } else if (change == QStringLiteral("port-changed")) {
        ++configuration.tcpPort;
    } else if (change == QStringLiteral("forwarding-off")) {
        configuration.forwardReceiverRtcm = false;
    }
#ifndef QGC_NO_SERIAL_LINK
    if (change == QStringLiteral("device-selected")) {
        configuration.serialDevice = other;
    } else if (change == QStringLiteral("baud-changed")) {
        configuration.serialBaudRate = 9600;
    }
#endif
    QSignalSpy errors(&receiver, &GPSReceiver::errorMessageChanged);
    receiver.setConfiguration(configuration);
    QCOMPARE(receiver.errorMessage(), withdrawn ? QString() : error);
    QCOMPARE(errors.size(), withdrawn ? 1 : 0);
    // Only the message follows the settings; connecting stays the user's.
    harness.update();
    QCOMPARE(harness.attempts(), attempts);
}

void GPSReceiverConnectionPolicyTest::_observerActsOnLostSession_data()
{
    QTest::addColumn<bool>("reconnects");
    QTest::newRow("reconnects") << true;
    QTest::newRow("disconnects") << false;
}

void GPSReceiverConnectionPolicyTest::_observerActsOnLostSession()
{
    QFETCH(bool, reconnects);
    PolicyHarness harness(tcpConfiguration());
    GPSReceiver& receiver = harness.receiver;
    QVERIFY(receiver.connectReceiver());
    harness.worker()->ready();
    // The status Facts reset in the middle of the handler, before any of the receiver's signals reports the loss.
    QObject observer;
    bool acted = false;
    connect(receiver.facts(), &FactGroup::telemetryAvailableChanged, &observer, [&] {
        if (receiver.hasReceiver() || std::exchange(acted, true)) {
            return;
        }
        if (reconnects) {
            (void) receiver.connectReceiver();
        } else {
            receiver.disconnectReceiver();
        }
    });
    const auto loseConnection = [&] {
        expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
        harness.worker()->fail(GPSConnectionError::DeviceError);
        verifyExpectedLogMessage();
    };

    loseConnection();
    QVERIFY(acted);
    // The observer's request is the last word: neither the lost session's message nor its retry remains.
    QCOMPARE(receiver.hasReceiver(), reconnects);
    QCOMPARE(harness.attempts(), reconnects ? 2 : 1);
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(!receiver.reconnecting());
    QVERIFY(!_retryPending(receiver));
    if (reconnects) {
        // The replacement is kept connected like any receiver the user connects.
        harness.worker()->ready();
        loseConnection();
        QVERIFY(receiver.reconnecting());
    }
}

void GPSReceiverConnectionPolicyTest::_observerActsOnReport_data()
{
    // The fix is the last status Fact a position report writes and the survey state the last a survey report writes,
    // each notified in the middle of the handler.
    QTest::addColumn<bool>("survey");
    QTest::addColumn<bool>("reconnects");
    QTest::newRow("position-reconnects") << false << true;
    QTest::newRow("position-disconnects") << false << false;
    QTest::newRow("survey-reconnects") << true << true;
    QTest::newRow("survey-disconnects") << true << false;
}

void GPSReceiverConnectionPolicyTest::_observerActsOnReport()
{
    QFETCH(bool, survey);
    QFETCH(bool, reconnects);
    auto configuration = survey ? receiverConfiguration() : tcpConfiguration();
    useTcp(configuration);
    PolicyHarness harness(configuration);
    GPSReceiver& receiver = harness.receiver;
    GPSReceiverFactGroup& facts = *receiver.facts();
    QVERIFY(receiver.connectReceiver());
    harness.worker()->ready();
    GPSPositionReport position = locatedFix(GPSFixQuality::Fix3D);
    GPSBaseStatus surveyed{.state = GPSBaseState::Active,
                           .position = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                           .meanAccuracyMeters = 1.0};
    if (survey) {
        harness.worker()->baseStatus(surveyed);
    } else {
        harness.worker()->position(position);
        QVERIFY(receiver.acceptedPositionObservation(GPSObservation::PositionUse::GroundStation));
    }
    QObject observer;
    bool acted = false;
    connect(survey ? facts.baseState() : facts.fixType(), &Fact::rawValueChanged, &observer, [&] {
        if (!receiver.hasReceiver() || std::exchange(acted, true)) {
            return;
        }
        receiver.disconnectReceiver();
        if (reconnects) {
            (void) receiver.connectReceiver();
        }
    });

    if (survey) {
        surveyed.state = GPSBaseState::Valid;
        harness.worker()->baseStatus(surveyed);
    } else {
        position.navigation.fixType = GPSFixQuality::RTKFixed;
        position.integrity.jamming.state = GPSIntegrityReport::JammingState::Warning;
        position.integrity.outputOverflowUs = 100;
        harness.worker()->position(position);
    }
    QVERIFY(acted);
    // The report was the ended receiver's: none of it is left with what the observer asked for.
    QCOMPARE(receiver.hasReceiver(), reconnects);
    QCOMPARE(harness.attempts(), reconnects ? 2 : 1);
    QCOMPARE(solution(facts), Solution{});
    QVERIFY(!facts.canSaveCurrentBasePosition());
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(!receiver.acceptedPositionObservation(GPSObservation::PositionUse::GroundStation));
}

void GPSReceiverConnectionPolicyTest::_observerReplacesReadyReceiver()
{
    ManualScheduler scheduler;
    PositionManager positions;
    ScriptedGPSReceiver scripted(&scheduler, {.positions = &positions});
    GPSReceiver& receiver = scripted.receiver;
    receiver.setConfiguration(tcpConfiguration());
    QVERIFY(receiver.connectReceiver());
    // The status Facts report that the receiver is ready in the middle of the handler.
    QObject observer;
    bool acted = false;
    connect(receiver.facts(), &FactGroup::telemetryAvailableChanged, &observer, [&] {
        if (!receiver.facts()->telemetryAvailable() || std::exchange(acted, true)) {
            return;
        }
        receiver.disconnectReceiver();
        (void) receiver.connectReceiver();
    });

    scripted.workers.current()->ready();
    QVERIFY(acted);
    QCOMPARE(scripted.workers.count(), 2);
    QVERIFY(!receiver.facts()->telemetryAvailable());
    // The replacement is not ready, so it is not the position source yet.
    scripted.workers.current()->position(locatedFix(GPSFixQuality::Fix3D));
    QCOMPARE(positions.selectedSource(), PositionManager::SelectedSource::None);
    scripted.workers.current()->ready();
    scripted.workers.current()->position(locatedFix(GPSFixQuality::Fix3D));
    QCOMPARE(positions.selectedSource(), PositionManager::SelectedSource::Receiver);
    receiver.shutdown();
    scripted.workers.finishRetired();
}

void GPSReceiverConnectionPolicyTest::_observerActsOnPositionRelease_data()
{
    QTest::addColumn<bool>("reconnects");
    QTest::newRow("reconnects") << true;
    QTest::newRow("disconnects") << false;
}

void GPSReceiverConnectionPolicyTest::_observerActsOnPositionRelease()
{
    QFETCH(bool, reconnects);
    ManualScheduler scheduler;
    PositionManager positions;
    ScriptedGPSReceiver scripted(&scheduler, {.positions = &positions});
    GPSReceiver& receiver = scripted.receiver;
    receiver.setConfiguration(tcpConfiguration());
    QVERIFY(receiver.connectReceiver());
    scripted.workers.current()->ready();
    scripted.workers.current()->position(locatedFix(GPSFixQuality::Fix3D));
    QCOMPARE(positions.selectedSource(), PositionManager::SelectedSource::Receiver);
    // A lost receiver stops being the position source before anything else reports it.
    QObject observer;
    bool acted = false;
    connect(&positions, &PositionManager::selectionChanged, &observer, [&] {
        if (receiver.hasReceiver() || std::exchange(acted, true)) {
            return;
        }
        if (reconnects) {
            (void) receiver.connectReceiver();
        } else {
            receiver.disconnectReceiver();
        }
    });
    const auto loseConnection = [&] {
        expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
        scripted.workers.current()->fail(GPSConnectionError::DeviceError);
        verifyExpectedLogMessage();
    };

    loseConnection();
    QVERIFY(acted);
    // As for an observer of the receiver's own signals, its request is the last word.
    QCOMPARE(receiver.hasReceiver(), reconnects);
    QCOMPARE(scripted.workers.count(), reconnects ? 2 : 1);
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(!receiver.reconnecting());
    QVERIFY(!_retryPending(receiver));
    if (reconnects) {
        scripted.workers.current()->ready();
        loseConnection();
        QVERIFY(receiver.reconnecting());
    }
    receiver.shutdown();
    scripted.workers.finishRetired();
}

void GPSReceiverConnectionPolicyTest::_observerActsOnShutdown_data()
{
    QTest::addColumn<bool>("connects");
    QTest::newRow("connects") << true;
    QTest::newRow("disconnects") << false;
}

void GPSReceiverConnectionPolicyTest::_observerActsOnShutdown()
{
    QFETCH(bool, connects);
    PolicyHarness harness(tcpConfiguration());
    GPSReceiver& receiver = harness.receiver;
    QVERIFY(receiver.connectReceiver());
    harness.worker()->ready();
    // The status text reports that the receiver is gone before shutdown() returns.
    QObject observer;
    bool acted = false;
    connect(&receiver, &GPSReceiver::statusTextChanged, &observer, [&] {
        if (receiver.hasReceiver() || std::exchange(acted, true)) {
            return;
        }
        if (connects) {
            (void) receiver.connectReceiver();
        } else {
            receiver.disconnectReceiver();
        }
    });

    receiver.shutdown();
    QVERIFY(acted);
    // Shutdown leaves no receiver, whatever an observer asks for meanwhile, and has joined and deleted its worker.
    QVERIFY(!receiver.hasReceiver());
    QCOMPARE(harness.attempts(), 1);
    QVERIFY(!harness.worker());
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(!receiver.reconnecting());
}

void GPSReceiverConnectionPolicyTest::_labelsFollowLostSessionOnce()
{
    PolicyHarness harness(tcpConfiguration());
    GPSReceiver& receiver = harness.receiver;
    QVERIFY(receiver.connectReceiver());
    harness.worker()->ready();
    harness.worker()->position(fixReport(GPSFixQuality::Fix3D));
    QStringList labels;
    QObject observer;
    const auto observe = [&](const QString& label) {
        // The status Facts reset one by one; a label is notified once they all have, with the receiver gone and its
        // reconnect pending.
        QVERIFY(!receiver.hasReceiver());
        QVERIFY(receiver.reconnecting());
        QVERIFY(!receiver.facts()->telemetryAvailable());
        labels.append(label);
    };
    connect(&receiver, &GPSReceiver::statusTextChanged, &observer, [&] { observe(receiver.statusText()); });
    connect(&receiver, &GPSReceiver::summaryLabelChanged, &observer, [&] { observe(receiver.summaryLabel()); });

    expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    harness.worker()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    labels.sort();
    QCOMPARE(labels, (QStringList{QString(), GPSReceiver::tr("Connecting automatically...")}));
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
    auto configuration = GPSTest::serialConfiguration(gpsReceiverManufacturerForType(GPSType::Quectel), device, 115200);
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
    QCOMPARE(harness.worker()->type(), GPSType::Quectel);
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
    QTest::addColumn<uint>("baud");
    QTest::newRow("connect-automatically") << true << 4800U;
    QTest::newRow("manual-only") << false << 4800U;
    // Without a rate, the session of a passive receiver listens for it.
    QTest::newRow("listens-for-baud-rate") << true << 0U;
}

void GPSReceiverConnectionPolicyTest::_savedSerialWaitsForReturningPort()
{
    QFETCH(bool, autoConnect);
    QFETCH(uint, baud);
    const QString device = QStringLiteral("/test/saved");
    const QList<SerialPortManager::Port> present{serialPort(QStringLiteral("/test/other")), serialPort(device)};
    auto configuration = savedSerialConfiguration(autoConnect);
    configuration.serialBaudRate = baud;
    PolicyHarness harness(configuration);
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
    QCOMPARE(harness.worker()->type(), GPSType::Passive);
    QCOMPARE(harness.worker()->capturedConfig().baudRate, baud);
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
                                            << static_cast<int>(GPSType::Automatic) << true;
    QTest::newRow("automatic-named") << GPS_AUTOMATIC_MANUFACTURER << QStringLiteral("u-blox GNSS receiver")
                                     << static_cast<int>(GPSType::Automatic) << true;
    // Quectel saved, a u-blox receiver attached: the identity check fails, and flash consent would not help.
    QTest::newRow("quectel-on-ublox") << gpsReceiverManufacturerForType(GPSType::Quectel)
                                      << QStringLiteral("u-blox GNSS receiver") << static_cast<int>(GPSType::Quectel)
                                      << false;
    QTest::newRow("quectel-generic-name")
        << gpsReceiverManufacturerForType(GPSType::Quectel) << QStringLiteral("CP2102 USB to UART")
        << static_cast<int>(GPSType::Quectel) << true;
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
        worker->detected(GPSType::Quectel);
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

void GPSReceiverConnectionPolicyTest::_observerActsOnPortScan_data()
{
    // Whether the user's Connect or an automatic connection makes the scan, and what the observer does.
    QTest::addColumn<bool>("byUser");
    QTest::addColumn<bool>("connects");
    QTest::newRow("connects-during-automatic-connection") << false << true;
    QTest::newRow("disconnects-during-automatic-connection") << false << false;
    QTest::newRow("disconnects-during-connect") << true << false;
}

void GPSReceiverConnectionPolicyTest::_observerActsOnPortScan()
{
    QFETCH(bool, byUser);
    QFETCH(bool, connects);
    PolicyHarness harness(savedSerialConfiguration(), {serialPort(QStringLiteral("/test/saved"))});
    GPSReceiver& receiver = harness.receiver;
    // The first question about the serial ports scans them, which changes the device picker's entries.
    QObject observer;
    bool acted = false;
    connect(&receiver, &GPSReceiver::serialPortEntriesChanged, &observer, [&] {
        if (std::exchange(acted, true)) {
            return;
        }
        if (connects) {
            (void) receiver.connectReceiver();
        } else {
            receiver.disconnectReceiver();
        }
    });

    if (byUser) {
        QVERIFY(receiver.connectReceiver());
    } else {
        harness.update();
    }
    QVERIFY(acted);
    // The later request stands, the observer's unless the user's Connect follows it, without a failed attempt.
    const bool connected = connects || byUser;
    QCOMPARE(receiver.hasReceiver(), connected);
    QCOMPARE(harness.attempts(), connected ? 1 : 0);
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(!_retryPending(receiver));
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(60)));
    harness.update();
    QCOMPARE(harness.attempts(), connected ? 1 : 0);
    if (connected) {
        // A receiver the user connected is reconnected when lost.
        harness.worker()->ready();
        expectLogMessage(kReceiverLog, QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
        harness.worker()->fail(GPSConnectionError::DeviceError);
        verifyExpectedLogMessage();
        harness.releaseRetired();
        QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(1)));
        harness.update();
        QCOMPARE(harness.attempts(), 2);
    }
}

#endif

UT_REGISTER_TEST(GPSReceiverConnectionPolicyTest, TestLabel::Unit)
