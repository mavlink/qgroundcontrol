#include "GPSReceiverConnectionPolicyTest.h"

#include <functional>
#include <optional>
#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverConnectionTarget.h"
#include "GPSSerialPortManagerAdapter.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "NTRIPManager.h"
#include "NotificationQueue.h"
#include "PositionManager.h"
#include "RTKSettings.h"
#include "ScriptedProvider.h"
#include "SerialPortManager.h"

namespace {
using Port = SerialPortManager::Port;

Port rtkPort(const QString& location = QStringLiteral("/test/rtk"))
{
    return {location, location.section(QLatin1Char('/'), -1), QGCSerialPortInfo::BoardTypeRTKGPS,
            QStringLiteral("u-blox")};
}

Port genericPort(const QString& location)
{
    return {location, location.section(QLatin1Char('/'), -1), QGCSerialPortInfo::BoardTypeUnknown, {}};
}

GPSReceiver::Configuration serialConfiguration(bool autoConnect = true)
{
    GPSReceiver::Configuration configuration;
    configuration.autoConnect = autoConnect;
    configuration.receiverRole = GPSReceiver::ConfiguredBase;
    configuration.connectionType = GPSReceiver::Serial;
    configuration.baseReceiverManufacturer = GPSReceiver::manufacturerForType(GPSType::ublox);
    configuration.serialDevice = QStringLiteral("/test/rtk");
    configuration.serialBaudRate = 115200;
    configuration.baseMode = static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn);
    return configuration;
}

GPSReceiver::Configuration passiveSerialConfiguration(bool autoConnect = false)
{
    auto configuration = serialConfiguration(autoConnect);
    configuration.receiverRole = GPSReceiver::Passive;
    configuration.serialDevice = QStringLiteral("/test/manual");
    return configuration;
}

GPSReceiver::Configuration tcpConfiguration(bool autoConnect = false)
{
    auto configuration = serialConfiguration(autoConnect);
    configuration.receiverRole = GPSReceiver::Passive;
    configuration.connectionType = GPSReceiver::Tcp;
    configuration.tcpHost = QStringLiteral("rtk.test");
    configuration.tcpPort = 2101;
    return configuration;
}

struct ConnectCall
{
    enum class Kind
    {
        Tcp,
        Udp,
        Serial,
    };

    Kind kind = Kind::Tcp;
    QString endpoint;
    GPSType type = GPSType::ublox;
    uint32_t baudRate = 0;
    bool allowPersistentChanges = false;
};

QString callSummary(const ConnectCall& call)
{
    const auto prefix = [kind = call.kind] {
        switch (kind) {
            case ConnectCall::Kind::Tcp:
                return QStringLiteral("tcp");
            case ConnectCall::Kind::Udp:
                return QStringLiteral("udp");
            case ConnectCall::Kind::Serial:
                return QStringLiteral("serial");
        }
        return QString();
    }();
    return QStringLiteral("%1:%2:%3:%4:%5")
        .arg(prefix, call.endpoint)
        .arg(static_cast<int>(call.type))
        .arg(call.baudRate)
        .arg(call.allowPersistentChanges);
}

QStringList callSummaries(const QList<ConnectCall>& calls)
{
    QStringList summaries;
    summaries.reserve(calls.size());
    for (const auto& call : calls) {
        summaries.append(callSummary(call));
    }
    return summaries;
}

class FakeConnectionTarget final : public GPSReceiverConnectionTarget
{
public:
    explicit FakeConnectionTarget(SerialPortManager::Enumerator enumerator = [] { return QList<Port>{}; })
        : ports(nullptr, std::move(enumerator))
    {}

    bool hasReceiver() const override { return connected; }

    GPSConnectionError connectionError() const override { return error; }

    void setConnectionError(GPSConnectionError value, const QString& message) override
    {
        error = value;
        errorMessage = message;
        errorMessages.append(message);
    }

    void disconnectReceiver(bool clearError) override
    {
        ++disconnects;
        disconnectClearError.append(clearError);
        connected = false;
        if (clearError) {
            error = GPSConnectionError::None;
            errorMessage.clear();
        }
        if (onDisconnect) {
            onDisconnect();
        }
    }

    bool connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges) override
    {
        return record({.kind = ConnectCall::Kind::Tcp,
                       .endpoint = QStringLiteral("%1:%2").arg(host).arg(port),
                       .type = type,
                       .baudRate = GPSTransport::BRIDGE_BAUDRATE,
                       .allowPersistentChanges = allowPersistentChanges});
    }

    bool connectUdp(quint16 port, GPSType type) override
    {
        return record({.kind = ConnectCall::Kind::Udp,
                       .endpoint = QString::number(port),
                       .type = type,
                       .baudRate = 0,
                       .allowPersistentChanges = false});
    }

#ifndef QGC_NO_SERIAL_LINK
    GPSSerialPorts* serialPorts() const override { return &serialPortAdapter; }

    bool connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges) override
    {
        return record({.kind = ConnectCall::Kind::Serial,
                       .endpoint = device,
                       .type = type,
                       .baudRate = baudRate,
                       .allowPersistentChanges = allowPersistentChanges});
    }
#endif

    NotificationQueue& notifications() override { return queue; }

    bool record(const ConnectCall& call)
    {
        calls.append(call);
        if (onConnect) {
            onConnect();
        }
        connected = acceptConnections;
        error = connected ? GPSConnectionError::None : GPSConnectionError::OpenFailed;
        if (!connected) {
            errorMessage = QStringLiteral("connect failed");
        }
        return connected;
    }

    QObject owner;
    NotificationQueue queue{&owner};
    SerialPortManager ports;
    mutable GPSSerialPortManagerAdapter serialPortAdapter{&ports};
    QList<ConnectCall> calls;
    QStringList errorMessages;
    QList<bool> disconnectClearError;
    QString errorMessage;
    GPSConnectionError error = GPSConnectionError::None;
    int disconnects = 0;
    bool connected = false;
    bool acceptConnections = true;
    std::function<void()> onConnect;
    std::function<void()> onDisconnect;
};

struct PolicyHarness
{
    explicit PolicyHarness(
        const GPSReceiver::Configuration& configuration,
        SerialPortManager::Enumerator enumerator = [] { return QList<Port>{}; })
        : target(std::move(enumerator))
        , policy(target, nullptr, &scheduler)
    {
        policy.setConfiguration(configuration);
    }

    FakeConnectionTarget target;
    ManualScheduler scheduler;
    GPSReceiverConnectionPolicy policy;
};
}  // namespace

void GPSReceiverConnectionPolicyTest::init()
{
    UnitTest::init();
    ignoreLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport|session ended")));
}

void GPSReceiverConnectionPolicyTest::_manualRetryDrivesTarget()
{
    PolicyHarness harness(tcpConfiguration());
    QSignalSpy autoDisabled(&harness.policy, &GPSReceiverConnectionPolicy::autoConnectDisabled);
    const QString expected = QStringLiteral("tcp:rtk.test:2101:%1:%2:%3")
                                 .arg(static_cast<int>(GPSType::passive))
                                 .arg(GPSTransport::BRIDGE_BAUDRATE)
                                 .arg(false);

    QVERIFY(harness.policy.connectConfigured(false));
    QCOMPARE(callSummaries(harness.target.calls), QStringList{expected});
    QCOMPARE(autoDisabled.size(), 1);
    harness.policy.receiverReady();

    QSignalSpy reconnecting(&harness.policy, &GPSReceiverConnectionPolicy::reconnectingChanged);
    harness.target.connected = false;
    harness.target.acceptConnections = false;
    QCOMPARE(harness.policy.sessionEnded(false), GPSReceiverSessionOutcome::Retrying);
    QVERIFY(harness.policy.reconnecting());
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(1)));
    harness.policy.update();
    QCOMPARE(callSummaries(harness.target.calls), (QStringList{expected, expected}));
    QVERIFY(harness.policy.reconnecting());
    QVERIFY(harness.policy.retryPending());

    harness.policy.disconnectConfigured();
    QCOMPARE(harness.target.disconnects, 1);
    QVERIFY(!harness.policy.reconnecting());
    QCOMPARE(reconnecting.size(), 1);
}

void GPSReceiverConnectionPolicyTest::_connectConfiguredValidationErrors_data()
{
    QTest::addColumn<QString>("reason");
    for (const auto* reason :
         {"unknown-manufacturer", "already-connected", "tcp-host", "tcp-port", "udp-port", "base-over-udp",
          "serial-empty", "serial-missing", "serial-bootloader", "invalid-baud", "passive-auto-baud"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
}

void GPSReceiverConnectionPolicyTest::_connectConfiguredValidationErrors()
{
    QFETCH(QString, reason);
    auto configuration = serialConfiguration(false);
    QList<Port> inventory{rtkPort()};
    if (reason == QStringLiteral("unknown-manufacturer")) {
        configuration.baseReceiverManufacturer = 99;
    } else if (reason == QStringLiteral("tcp-host")) {
        configuration = tcpConfiguration(false);
        configuration.tcpHost.clear();
    } else if (reason == QStringLiteral("tcp-port")) {
        configuration = tcpConfiguration(false);
        configuration.tcpPort = 0;
    } else if (reason == QStringLiteral("udp-port")) {
        configuration.receiverRole = GPSReceiver::Passive;
        configuration.connectionType = GPSReceiver::Udp;
        configuration.udpPort = 0;
    } else if (reason == QStringLiteral("base-over-udp")) {
        configuration.connectionType = GPSReceiver::Udp;
        configuration.udpPort = 14401;
    } else if (reason == QStringLiteral("serial-empty")) {
        configuration.serialDevice.clear();
    } else if (reason == QStringLiteral("serial-missing")) {
        inventory.clear();
    } else if (reason == QStringLiteral("serial-bootloader")) {
        inventory.first().bootloader = true;
    } else if (reason == QStringLiteral("invalid-baud")) {
        configuration.serialBaudRate = 1000;
    } else if (reason == QStringLiteral("passive-auto-baud")) {
        configuration = passiveSerialConfiguration(false);
        configuration.serialBaudRate = 0;
        inventory = {genericPort(configuration.serialDevice)};
    }
    PolicyHarness harness(configuration, [&] { return inventory; });
    if (reason == QStringLiteral("already-connected")) {
        harness.target.connected = true;
    }
    QSignalSpy autoDisabled(&harness.policy, &GPSReceiverConnectionPolicy::autoConnectDisabled);

    QVERIFY(!harness.policy.connectConfigured(false));
    QVERIFY(!harness.target.errorMessage.isEmpty());
    QVERIFY(harness.target.calls.isEmpty());
    QCOMPARE(autoDisabled.size(), 0);
}

void GPSReceiverConnectionPolicyTest::_autoDiscoveryWaitsReconnectsAndDisables()
{
    QList<Port> inventory{rtkPort()};
    auto configuration = serialConfiguration(true);
    PolicyHarness harness(configuration, [&] { return inventory; });

    harness.policy.update();
    QVERIFY(harness.target.calls.isEmpty());
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    // Auto-connect detects the rate and never passes flash-save consent.
    QCOMPARE(callSummaries(harness.target.calls), QStringList{QStringLiteral("serial:/test/rtk:0:0:0")});
    QVERIFY(harness.target.connected);
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);
    harness.target.connected = false;
    QCOMPARE(harness.policy.sessionEnded(false), GPSReceiverSessionOutcome::AutoRetrying);
    QVERIFY(harness.policy.retryPending());

    inventory.clear();
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                 harness.policy.update();
                                 return harness.target.disconnects == 1;
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(harness.target.disconnects, 1);
    QVERIFY(!harness.target.connected);
    QVERIFY(!harness.policy.retryPending());
    inventory.append(rtkPort());
    QTRY_VERIFY_WITH_TIMEOUT(!harness.target.ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 2);

    // An unplugged receiver is left to discovery instead of a retry.
    harness.target.connected = false;
    QCOMPARE(harness.policy.sessionEnded(true), GPSReceiverSessionOutcome::Unplugged);
    QVERIFY(!harness.policy.retryPending());
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 3);

    configuration.autoConnect = false;
    harness.policy.setConfiguration(configuration);
    harness.policy.update();
    QCOMPARE(harness.target.disconnects, 2);
    QVERIFY(!harness.target.connected);
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 3);
}

void GPSReceiverConnectionPolicyTest::_tcpModeSkipsSerialDiscovery()
{
    auto configuration = tcpConfiguration(true);
    QList<Port> inventory{rtkPort()};
    PolicyHarness harness(configuration, [&] { return inventory; });
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QVERIFY(harness.target.calls.isEmpty());

    configuration = serialConfiguration(true);
    harness.policy.setConfiguration(configuration);
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);
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
    auto configuration = serialConfiguration(true);
    if (reason == QStringLiteral("passive-role")) {
        configuration.receiverRole = GPSReceiver::PositionOnly;
    }
    Port port = rtkPort();
    port.bootloader = reason == QStringLiteral("bootloader");
    if (reason == QStringLiteral("other-board")) {
        port.boardType = QGCSerialPortInfo::BoardTypePixhawk;
    }
    PolicyHarness harness(configuration, [&] { return QList<Port>{port}; });
    (void) harness.target.ports.availablePorts();
    SerialPortManager::ReservationPtr reservation;
    if (reason == QStringLiteral("busy")) {
        reservation = harness.target.ports.reservePort(port.systemLocation);
    } else if (reason == QStringLiteral("single-port")) {
        harness.target.ports.setSinglePortOnly(true);
        reservation = harness.target.ports.reservePort(QStringLiteral("/test/mavlink"));
    }
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QVERIFY(harness.target.calls.isEmpty());
}

void GPSReceiverConnectionPolicyTest::_autoRetryBacksOffAndRespectsReservations()
{
    QList<Port> inventory{rtkPort()};
    PolicyHarness harness(serialConfiguration(true), [&] { return inventory; });
    harness.target.acceptConnections = false;
    const auto failedAttempt = [&] {
        QVERIFY(!harness.target.connected);
        QVERIFY(harness.policy.retryPending());
    };

    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);
    failedAttempt();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(999)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 2);
    failedAttempt();

    auto claim = harness.target.ports.reservePort(QStringLiteral("/test/rtk"));
    QVERIFY(claim);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1999)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 2);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 2);
    claim.reset();
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 3);
    for (const int delay : {4000, 8000, 16000, 30000, 30'000}) {
        failedAttempt();
        QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(delay - 1)));
        harness.policy.update();
        const auto beforeDue = harness.target.calls.size();
        QVERIFY(harness.scheduler.advanceBy(std::chrono::milliseconds(1)));
        harness.policy.update();
        QCOMPARE(harness.target.calls.size(), beforeDue + 1);
    }
    failedAttempt();

    auto configuration = serialConfiguration(true);
    configuration.receiverRole = GPSReceiver::Passive;
    harness.policy.setConfiguration(configuration);
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(30)));
    const auto attempts = harness.target.calls.size();
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), attempts);
    QVERIFY(!harness.policy.retryPending());
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
    Port first = rtkPort(QStringLiteral("/test/receiver-first"));
    first.physicalDeviceId = QStringLiteral("1:2:serial");
    Port second = first;
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
    PolicyHarness harness(serialConfiguration(true), [&] { return QList<Port>{first, second}; });
    auto claim = harness.target.ports.reservePort(first.systemLocation);
    QVERIFY(claim);
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), connectSecond ? 1 : 0);
    if (connectSecond) {
        QCOMPARE(harness.target.calls.constFirst().endpoint, second.systemLocation);
    }
}

void GPSReceiverConnectionPolicyTest::_genericUsbNeedsExplicitSelection_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::newRow("unicore") << 5;
    QTest::newRow("quectel") << 6;
}

void GPSReceiverConnectionPolicyTest::_genericUsbNeedsExplicitSelection()
{
    QFETCH(int, manufacturer);
    auto configuration = serialConfiguration(true);
    configuration.baseReceiverManufacturer = manufacturer;
    configuration.serialDevice = QStringLiteral("/test/ch340");
    const QList<Port> inventory{genericPort(QStringLiteral("/test/ch340")), genericPort(QStringLiteral("/test/ftdi")),
                                rtkPort(QStringLiteral("/test/known"))};
    PolicyHarness harness(configuration, [&] { return inventory; });
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);
    QCOMPARE(harness.target.calls.constFirst().endpoint, QStringLiteral("/test/known"));
    QVERIFY(harness.target.ports.canReservePort(QStringLiteral("/test/ch340")));
    QVERIFY(harness.target.ports.canReservePort(QStringLiteral("/test/ftdi")));
}

void GPSReceiverConnectionPolicyTest::_manualConnectionRetiresAutoOwnership()
{
    auto configuration = serialConfiguration(true);
    PolicyHarness harness(configuration, [] { return QList<Port>{rtkPort(QStringLiteral("/test/known"))}; });
    connect(&harness.policy, &GPSReceiverConnectionPolicy::autoConnectDisabled, &harness.policy, [&] {
        configuration.autoConnect = false;
        harness.policy.setConfiguration(configuration);
    });
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);

    QSignalSpy autoDisabled(&harness.policy, &GPSReceiverConnectionPolicy::autoConnectDisabled);
    harness.policy.disconnectConfigured();
    QVERIFY(!harness.target.connected);
    QCOMPARE(autoDisabled.size(), 1);
    QVERIFY(!harness.policy.reconnecting());
    harness.policy.update();
    harness.policy.stop();
    QCOMPARE(harness.target.calls.size(), 1);

    configuration = serialConfiguration(true);
    harness.policy.setConfiguration(configuration);
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 2);
}

void GPSReceiverConnectionPolicyTest::_manualRetryWaitsForReturningPort()
{
    auto configuration = passiveSerialConfiguration(false);
    QList<Port> inventory{genericPort(QStringLiteral("/test/manual"))};
    PolicyHarness harness(configuration, [&] { return inventory; });
    QVERIFY(harness.policy.connectConfigured(false));
    harness.policy.receiverReady();

    inventory.clear();
    harness.target.connected = false;
    QCOMPARE(harness.policy.sessionEnded(true), GPSReceiverSessionOutcome::WaitingForPort);
    QVERIFY(harness.policy.reconnecting());
    QTRY_VERIFY_WITH_TIMEOUT(harness.target.ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(30)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);

    inventory.append(genericPort(QStringLiteral("/test/manual")));
    QTRY_VERIFY_WITH_TIMEOUT(!harness.target.ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 2);
    QCOMPARE(harness.target.calls.constLast().endpoint, QStringLiteral("/test/manual"));
    QVERIFY(harness.target.connected);
    harness.policy.disconnectConfigured();
    QVERIFY(!harness.policy.reconnecting());
    QVERIFY(!harness.policy.retryPending());
}

void GPSReceiverConnectionPolicyTest::_configurationChangeCancelsManualRetry_data()
{
    QTest::addColumn<QString>("change");
    QTest::newRow("connection") << QStringLiteral("connection");
    QTest::newRow("auto-connect") << QStringLiteral("auto-connect");
}

void GPSReceiverConnectionPolicyTest::_configurationChangeCancelsManualRetry()
{
    QFETCH(QString, change);
    auto configuration = tcpConfiguration(false);
    PolicyHarness harness(configuration);
    QVERIFY(harness.policy.connectConfigured(false));
    harness.policy.receiverReady();
    harness.target.connected = false;
    QCOMPARE(harness.policy.sessionEnded(false), GPSReceiverSessionOutcome::Retrying);
    QVERIFY(harness.policy.reconnecting());
    QVERIFY(harness.policy.retryPending());

    if (change == QStringLiteral("connection")) {
        ++configuration.tcpPort;
    } else {
        configuration.autoConnect = true;
    }
    harness.policy.setConfiguration(configuration);
    QVERIFY(!harness.policy.reconnecting());
    QVERIFY(!harness.policy.retryPending());
    QVERIFY(!harness.target.connected);
}

void GPSReceiverConnectionPolicyTest::_connectSavedWaitsForReceiver()
{
    auto configuration = passiveSerialConfiguration(false);
    configuration.serialDevice = QStringLiteral("/test/startup");
    configuration.serialBaudRate = 4800;
    QList<Port> inventory;
    PolicyHarness harness(configuration, [&] { return inventory; });
    QSignalSpy changes(&harness.policy, &GPSReceiverConnectionPolicy::reconnectingChanged);

    harness.policy.connectSaved();
    QVERIFY(harness.policy.reconnecting());
    QCOMPARE(changes.size(), 1);
    QVERIFY(harness.target.calls.isEmpty());
    inventory.append(genericPort(QStringLiteral("/test/startup")));
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                 harness.policy.update();
                                 return harness.target.calls.size() == 1;
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(harness.target.calls.constFirst().endpoint, QStringLiteral("/test/startup"));
    QCOMPARE(harness.target.calls.constFirst().type, GPSType::passive);
    harness.policy.disconnectConfigured();
    QVERIFY(!harness.policy.reconnecting());
    QVERIFY(!harness.policy.retryPending());
}

void GPSReceiverConnectionPolicyTest::_connectSavedKeepsDiscovery_data()
{
    QTest::addColumn<bool>("present");
    QTest::newRow("receiver-present") << true;
    QTest::newRow("receiver-absent") << false;
}

void GPSReceiverConnectionPolicyTest::_connectSavedKeepsDiscovery()
{
    QFETCH(bool, present);
    QList<Port> inventory;
    if (present) {
        inventory.append(rtkPort());
    }
    PolicyHarness harness(serialConfiguration(true), [&] { return inventory; });
    harness.policy.connectSaved();
    QVERIFY(!harness.policy.reconnecting());
    if (present) {
        QCOMPARE(harness.target.calls.size(), 1);
        QVERIFY(harness.target.connected);
        return;
    }
    QCOMPARE(harness.target.calls.size(), 0);
    QVERIFY(!harness.target.connected);
    inventory.append(rtkPort());
    QTRY_VERIFY_WITH_TIMEOUT(!harness.target.ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    harness.policy.update();
    QVERIFY(harness.scheduler.advanceBy(std::chrono::seconds(6)));
    harness.policy.update();
    QCOMPARE(harness.target.calls.size(), 1);
    QVERIFY(harness.target.connected);
}

void GPSReceiverConnectionPolicyTest::_shutdownDuringConnectionTick()
{
    auto* applicationCorrections = GPSManager::instance()->corrections();
    const auto restoreCorrections = qScopeGuard(
        [applicationCorrections] { GPSManager::instance()->ntrip()->setCorrectionManager(applicationCorrections); });
    int enumerations = 0;
    SerialPortManager ports(nullptr, [&] {
        ++enumerations;
        return QList<Port>{rtkPort()};
    });
    GPSSerialPortManagerAdapter serialPorts(&ports);
    GPSManager manager;
    manager.gpsRtk()->setConfiguration(serialConfiguration(true));
    manager.gpsRtk()->setSerialPorts(&serialPorts);
    connect(&ports, &SerialPortManager::portsEnumerated, &manager, &GPSManager::shutdown);
    manager._updateConnections();
    QVERIFY(manager._shutdown);
    QVERIFY(!manager.gpsRtk()->hasReceiver());
    const int seen = enumerations;
    manager._updateConnections();
    manager._updateConnections();
    QCOMPARE(enumerations, seen);
    QVERIFY(!manager.gpsRtk()->hasReceiver());
}

void GPSReceiverConnectionPolicyTest::_manualRetryRecreatesGPSReceiverSession()
{
    ManualScheduler scheduler;
    GPSReceiver receiver(nullptr, &scheduler);
    ScriptedProviderFactory providers;
    receiver.setProviderFactory(providers.providerFactory());
    receiver.setConfiguration(tcpConfiguration(false));

    QVERIFY(receiver.connectConfiguredGPS(false));
    QCOMPARE(providers.count(), 1);
    auto* first = providers.current();
    QVERIFY(first);
    QCOMPARE(first->capturedConfig().baudRate, GPSTransport::BRIDGE_BAUDRATE);
    QVERIFY(!first->capturedConfig().allowPersistentChanges);
    first->ready();
    QVERIFY(receiver.connected());
    QVERIFY(!receiver.reconnecting());

    first->fail(GPSConnectionError::DeviceError);
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver connection lost. Reconnecting automatically."));
    QVERIFY(receiver.reconnecting());
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    receiver.connectionPolicy()->update();
    QCOMPARE(providers.count(), 2);
    auto* retry = providers.current();
    QVERIFY(retry);
    QVERIFY(!retry->capturedConfig().allowPersistentChanges);
    retry->ready();
    QVERIFY(receiver.connected());
    QVERIFY(!receiver.reconnecting());
}

void GPSReceiverConnectionPolicyTest::_serialPolicyIntegrationUsesSelectedPort()
{
#ifndef QGC_NO_SERIAL_LINK
    QList<Port> inventory{
        genericPort(QStringLiteral("/test/unselected")),
        {QStringLiteral("/test/selected"), QStringLiteral("selected"), QGCSerialPortInfo::BoardTypeUnknown,
         QStringLiteral("USB serial")},
    };
    SerialPortManager ports(nullptr, [&] { return inventory; });
    ManualScheduler scheduler;
    GPSReceiver receiver(nullptr, &scheduler);
    ScriptedProviderFactory providers;
    receiver.setProviderFactory(providers.providerFactory());
    auto configuration = serialConfiguration(true);
    configuration.serialDevice = QStringLiteral("/test/selected");
    receiver.setConfiguration(configuration);
    GPSSerialPortManagerAdapter serialPorts(&ports);
    receiver.setSerialPorts(&serialPorts);
    QSignalSpy autoDisabled(&receiver, &GPSReceiver::autoConnectDisabled);

    QVERIFY(receiver.connectConfiguredGPS());
    QCOMPARE(autoDisabled.size(), 1);
    QCOMPARE(providers.count(), 1);
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("/test/selected"));
    QVERIFY(ports.isPortReserved(QStringLiteral("/test/selected")));
    QVERIFY(!ports.isPortReserved(QStringLiteral("/test/unselected")));
    auto* provider = providers.current();
    QVERIFY(provider);
    QPointer<ScriptedProvider> firstProvider = provider;
    QCOMPARE(provider->capturedConfig().baudRate, 115200U);
    provider->ready(QStringLiteral("ZED-F9P HPG 1.32"));
    GPSSurveyReport survey;
    survey.active = true;
    survey.valid = true;
    provider->survey(survey);
    QVERIFY(receiver.connected());
    QCOMPARE(receiver.receiverIdentity(), QStringLiteral("ZED-F9P HPG 1.32"));
    QVERIFY(receiver.status().active);
    QVERIFY(receiver.status().valid);

    inventory.removeLast();
    emit ports.portsEnumerated({QStringLiteral("/test/unselected")});
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver unplugged. Reconnecting when it is plugged back in."));
    QVERIFY(receiver.reconnecting());
    QVERIFY(!receiver.hasReceiver());
    provider->finish();
    QTRY_VERIFY_WITH_TIMEOUT(firstProvider.isNull(), TestTimeout::mediumMs());
    inventory.append({QStringLiteral("/test/selected"), QStringLiteral("selected"), QGCSerialPortInfo::BoardTypeUnknown,
                      QStringLiteral("USB serial")});
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                 receiver.connectionPolicy()->update();
                                 return providers.count() == 2;
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("/test/selected"));
    receiver.disconnectConfiguredGPS();
    QVERIFY(!receiver.reconnecting());

    // Discovery owns the next session, so unplugging that receiver leaves its return to discovery.
    inventory.append(rtkPort(QStringLiteral("/test/discovered")));
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                 receiver.connectionPolicy()->update();
                                 const bool advanced = scheduler.advanceBy(std::chrono::seconds(6));
                                 receiver.connectionPolicy()->update();
                                 return advanced && providers.count() == 3;
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("/test/discovered"));
    emit ports.portsEnumerated({QStringLiteral("/test/unselected")});
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver unplugged."));
    QVERIFY(!receiver.reconnecting());

    // A connection the policy does not own leaves the next step to the user.
    QVERIFY(receiver.connectSerial(QStringLiteral("/test/unselected"), GPSType::ublox, 115200, false));
    emit ports.portsEnumerated({});
    QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver unplugged. Select a device and reconnect."));
    QVERIFY(!receiver.reconnecting());
#else
    QSKIP("Manual serial connection requires serial support");
#endif
}

void GPSReceiverConnectionPolicyTest::_autoConnectUsesSavedManufacturer_data()
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
    QTest::newRow("quectel-on-ublox") << GPSReceiver::manufacturerForType(GPSType::quectel)
                                      << QStringLiteral("u-blox GNSS receiver") << static_cast<int>(GPSType::quectel)
                                      << false;
    QTest::newRow("quectel-generic-name")
        << GPSReceiver::manufacturerForType(GPSType::quectel) << QStringLiteral("CP2102 USB to UART")
        << static_cast<int>(GPSType::quectel) << true;
}

void GPSReceiverConnectionPolicyTest::_autoConnectUsesSavedManufacturer()
{
#ifndef QGC_NO_SERIAL_LINK
    QFETCH(int, manufacturer);
    QFETCH(QString, boardName);
    QFETCH(int, type);
    QFETCH(bool, consentRefused);
    const QList<Port> inventory{
        {QStringLiteral("/test/rtk"), QStringLiteral("rtk"), QGCSerialPortInfo::BoardTypeRTKGPS, boardName}};
    SerialPortManager ports(nullptr, [&] { return inventory; });
    ManualScheduler scheduler;
    GPSReceiver receiver(nullptr, &scheduler);
    ScriptedProviderFactory providers;
    receiver.setProviderFactory(providers.providerFactory());
    auto configuration = serialConfiguration(true);
    configuration.baseReceiverManufacturer = manufacturer;
    receiver.setConfiguration(configuration);
    GPSSerialPortManagerAdapter serialPorts(&ports);
    receiver.setSerialPorts(&serialPorts);

    receiver.connectionPolicy()->update();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(6)));
    receiver.connectionPolicy()->update();
    QCOMPARE(providers.count(), 1);
    auto* provider = providers.current();
    QCOMPARE(static_cast<int>(provider->type()), type);
    QCOMPARE(provider->capturedConfig().baudRate, 0U);
    QVERIFY(!provider->capturedConfig().allowPersistentChanges);
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("/test/rtk"));
    if (manufacturer == GPS_AUTOMATIC_MANUFACTURER) {
        provider->detected(GPSType::quectel);
        QCOMPARE(receiver.detectedReceiver(), QStringLiteral("Quectel"));
    }

    // Only a receiver that asked for flash-save consent, which auto-connect never passes, gets advice on giving it.
    const QString detail =
        consentRefused ? QStringLiteral("LG290P role mismatch: save the requested base role externally")
                       : QStringLiteral(
                             "No verified LG290P(03) identity; receiver configuration was not changed. UBX frames were "
                             "received; this looks like a u-blox receiver");
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg, QRegularExpression("did not accept configuration"));
    provider->fail(consentRefused ? GPSConnectionError::ConsentRequired : GPSConnectionError::ConfigFailed, detail);
    verifyExpectedLogMessage();
    QVERIFY(!receiver.hasReceiver());
    if (consentRefused) {
        QVERIFY(receiver.errorMessage().startsWith(QStringLiteral("Receiver configuration failed: %1.").arg(detail)));
        QVERIFY2(receiver.errorMessage().contains(QStringLiteral("connect manually and allow flash save and restart")),
                 qPrintable(receiver.errorMessage()));
    } else {
        QCOMPARE(receiver.errorMessage(), GPSReceiver::tr("Receiver configuration failed: %1").arg(detail));
    }
    QVERIFY(receiver.connectionPolicy()->retryPending());
    receiver.disconnectGPS();
#else
    QSKIP("Auto-connect requires serial support");
#endif
}

UT_REGISTER_TEST(GPSReceiverConnectionPolicyTest, TestLabel::Unit)
