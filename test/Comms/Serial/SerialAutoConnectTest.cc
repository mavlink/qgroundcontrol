#include "SerialAutoConnectTest.h"

#include "SerialAutoConnect.h"
#include "SerialLink.h"

void SerialAutoConnectTest::_compositeSelection_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<bool>("firstSelected");
    QTest::addColumn<bool>("secondSelected");
    QTest::newRow("duplicate") << QStringLiteral("duplicate") << true << false;
    QTest::newRow("nmea-label-is-not-mavlink-policy") << QStringLiteral("nmea-label") << true << false;
    QTest::newRow("gps-sibling") << QStringLiteral("gps-sibling") << false << true;
    QTest::newRow("bootloader-sibling") << QStringLiteral("bootloader") << false << true;
    QTest::newRow("routed-sibling") << QStringLiteral("excluded") << false << true;
    QTest::newRow("busy-primary") << QStringLiteral("busy") << false << false;
    QTest::newRow("unknown-identities") << QStringLiteral("unknown") << true << true;
    QTest::newRow("distinct-devices") << QStringLiteral("distinct") << true << true;
}

void SerialAutoConnectTest::_compositeSelection()
{
    QFETCH(QString, scenario);
    QFETCH(bool, firstSelected);
    QFETCH(bool, secondSelected);
    SerialPortManager ports;
    SerialAutoConnect discovery(ports, [](SharedLinkConfigurationPtr&) {});
    SerialPortManager::Port first{QStringLiteral("/test/composite-first"), QStringLiteral("first"),
                                  QGCSerialPortInfo::BoardTypePixhawk, QStringLiteral("Test")};
    first.physicalDeviceId = QStringLiteral("1:2:serial");
    auto second = first;
    second.systemLocation = QStringLiteral("/test/composite-second");
    second.portName = QStringLiteral("second");
    if (scenario == QStringLiteral("nmea-label")) {
        second.description = QStringLiteral("NMEA interface");
    } else if (scenario == QStringLiteral("gps-sibling")) {
        first.boardType = QGCSerialPortInfo::BoardTypeRTKGPS;
    } else if (scenario == QStringLiteral("bootloader")) {
        first.bootloader = true;
    } else if (scenario == QStringLiteral("unknown")) {
        first.physicalDeviceId.clear();
        second.physicalDeviceId.clear();
    } else if (scenario == QStringLiteral("distinct")) {
        second.physicalDeviceId = QStringLiteral("1:2:other");
    }
    SerialPortManager::ReservationPtr claim;
    if (scenario == QStringLiteral("busy")) {
        claim = ports.reservePort(first.systemLocation);
        QVERIFY(claim);
    } else if (scenario == QStringLiteral("excluded")) {
        claim = ports.excludeFromAutoConnect(first.systemLocation);
        QVERIFY(claim);
    }
    // Inspect the discovery grace period before either candidate opens a serial port.
    discovery.update({first, second}, {.pixhawk = true});
    QCOMPARE(discovery._waitingPorts.contains(first.systemLocation), firstSelected);
    QCOMPARE(discovery._waitingPorts.contains(second.systemLocation), secondSelected);
}

void SerialAutoConnectTest::_discoveryDeadlineAndRetryState()
{
    SerialPortManager ports;
    QList<SharedLinkConfigurationPtr> attempts;
    SerialAutoConnect discovery(ports, [&](SharedLinkConfigurationPtr& config) { attempts.append(config); });
    const QString name = QStringLiteral("/test/autoconnect");
    const QList<SerialPortManager::Port> inventory{
        {name, QStringLiteral("autoconnect"), QGCSerialPortInfo::BoardTypePixhawk, QStringLiteral("Test")}};
    const SerialAutoConnect::Options enabled{.pixhawk = true};
    discovery.update(inventory, enabled);
    QVERIFY(attempts.isEmpty());
    QVERIFY(!discovery._waitingPorts.value(name).hasExpired());
    discovery._waitingPorts[name] = QDeadlineTimer::Forever;
    for (int i = 0; i < 10; ++i) {
        discovery.update(inventory, enabled);
    }
    QVERIFY(attempts.isEmpty());
    discovery._waitingPorts[name].setRemainingTime(0);
    discovery.update(inventory, enabled);
    QCOMPARE(attempts.size(), 1);
    const auto config = attempts.first();
    QVERIFY(!config->reconnectReady());
    discovery.update(inventory, enabled);
    QCOMPARE(attempts.size(), 1);
    QCOMPARE(discovery._configs.value(name), config);

    config->resetReconnectBackoff();
    auto claim = ports.reservePort(name);
    QVERIFY(claim);
    discovery.update(inventory, enabled);
    QCOMPARE(attempts.size(), 1);
    QVERIFY(config->reconnectReady());
    claim.reset();
    auto exclusion = ports.excludeFromAutoConnect(name);
    discovery.update(inventory, enabled);
    QCOMPARE(attempts.size(), 1);
    QVERIFY(config->reconnectReady());
    exclusion.reset();
    discovery.update(inventory, {});
    QCOMPARE(attempts.size(), 1);
    discovery.update(inventory, enabled);
    QCOMPARE(attempts.size(), 2);
    QCOMPARE(attempts.last(), config);

    config->resetReconnectBackoff();
    config->setSuppressAutoReconnect(true);
    discovery.update(inventory, enabled);
    QCOMPARE(attempts.size(), 2);
    discovery.update({}, enabled);
    QVERIFY(discovery._configs.isEmpty());
    QVERIFY(discovery._waitingPorts.isEmpty());
    discovery.update(inventory, enabled);
    QVERIFY(discovery._waitingPorts.contains(name));
    QVERIFY(discovery._configs.isEmpty());
}

UT_REGISTER_TEST(SerialAutoConnectTest, TestLabel::Unit, TestLabel::Comms)

void SerialAutoConnectTest::_replacementDeviceResetsConfiguration()
{
    SerialPortManager ports;
    QList<SharedLinkConfigurationPtr> attempts;
    SerialAutoConnect discovery(ports, [&](SharedLinkConfigurationPtr& config) { attempts.append(config); });
    SerialPortManager::Port port{QStringLiteral("/test/reused"), QStringLiteral("reused"),
                                 QGCSerialPortInfo::BoardTypePixhawk, QStringLiteral("Controller")};
    port.physicalDeviceId = QStringLiteral("controller-A");
    const SerialAutoConnect::Options options{.pixhawk = true, .sikRadio = true};
    discovery.update({port}, options);
    discovery._waitingPorts[port.systemLocation].setRemainingTime(0);
    discovery.update({port}, options);
    QCOMPARE(attempts.size(), 1);
    attempts.first()->setSuppressAutoReconnect(true);
    discovery.update({port}, options);
    QCOMPARE(attempts.size(), 1);

    port.physicalDeviceId = QStringLiteral("radio-B");
    port.boardType = QGCSerialPortInfo::BoardTypeSiKRadio;
    discovery.update({port}, options);
    QVERIFY(!discovery._configs.contains(port.systemLocation));
    QVERIFY(discovery._waitingPorts.contains(port.systemLocation));
    discovery._waitingPorts[port.systemLocation].setRemainingTime(0);
    discovery.update({port}, options);
    QCOMPARE(attempts.size(), 2);
    QVERIFY(attempts.last() != attempts.first());
    QVERIFY(!attempts.last()->suppressAutoReconnect());
    auto* config = qobject_cast<SerialConfiguration*>(attempts.last().get());
    QVERIFY(config);
    QCOMPARE(config->baud(), 57600);
    QVERIFY(!config->usbDirect());
}
