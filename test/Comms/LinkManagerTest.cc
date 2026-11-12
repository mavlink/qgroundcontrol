#include "LinkManagerTest.h"

#include <algorithm>
#include <memory>

#include <QtCore/QScopeGuard>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QTest>

#include "AutoConnectSettings.h"
#include "LinkManager.h"
#include "MavlinkSettings.h"
#include "MockLink.h"
#include "SettingsManager.h"
#include "UDPLink.h"
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <fcntl.h>
#include <unistd.h>
#endif

SharedLinkConfigurationPtr LinkManagerTest::_addMockConfig(const QString &name, bool dynamic, bool autoConnect)
{
    MockConfiguration *const mockConfig = new MockConfiguration(name);
    mockConfig->setDynamic(dynamic);
    mockConfig->setAutoConnect(autoConnect);

    SharedLinkConfigurationPtr config = linkManager()->addConfiguration(mockConfig);
    config->setAutoConnectStarted(true);
    if (!linkManager()->createConnectedLink(config)) {
        linkManager()->removeConfiguration(config.get());
        return nullptr;
    }
    return config;
}

void LinkManagerTest::_reconnect()
{
    linkManager()->_reconnectAutoConnectLinks();
}

void LinkManagerTest::_expectUdpBindFailureLogs()
{
    expectLogMessage("Comms.UDPLink", QtWarningMsg, QRegularExpression(QStringLiteral("AddressInUseError")));
    expectLogMessage("Comms.UDPLink", QtWarningMsg, QRegularExpression(QStringLiteral("Failed to bind UDP socket")));
    expectLogMessage("Comms.UDPLink", QtWarningMsg, QRegularExpression(QStringLiteral("Communication error")));
}

void LinkManagerTest::_verifyUdpBindFailureLogs()
{
    for (int i = 0; i < 3; i++) {
        verifyExpectedLogMessage();
    }
}

void LinkManagerTest::_testReconnectsDroppedAutoConnectLink()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("ReconnectMock"), false /*dynamic*/, true /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->link());

    config->link()->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());

    _reconnect();
    QVERIFY(config->link());

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testSuppressedLinkNotReconnected()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("SuppressMock"), false /*dynamic*/, true /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->link());

    // Manual disconnect sets suppressAutoReconnect so the timer leaves it alone.
    linkManager()->disconnectLink(config->link());
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    QVERIFY(config->suppressAutoReconnect());

    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testDynamicLinkNotReconnected()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("DynamicMock"), true /*dynamic*/, true /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->link());

    config->link()->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());

    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testNonAutoConnectLinkNotReconnected()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("ManualMock"), false /*dynamic*/, false /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->link());

    config->link()->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());

    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testNeverStartedLinkNotConnected()
{
    MockConfiguration *const mockConfig = new MockConfiguration(QStringLiteral("NeverStartedMock"));
    mockConfig->setDynamic(false);
    mockConfig->setAutoConnect(true);
    SharedLinkConfigurationPtr config = linkManager()->addConfiguration(mockConfig);
    QVERIFY(!config->autoConnectStarted());
    QVERIFY(config->link() == nullptr);

    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testLinkActiveStableAcrossReconnect()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("ActiveMock"), false /*dynamic*/, true /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->linkActive());

    config->link()->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    QVERIFY(config->linkActive());

    linkManager()->disconnectLinkConfiguration(config.get());
    QVERIFY(!config->linkActive());
    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testUdpUnresolvedHostFailsConnect()
{
    UDPConfiguration* const udpConfig = new UDPConfiguration(QStringLiteral("UnresolvedUdp"));
    udpConfig->addHost(QStringLiteral("drone.invalid"), 14550);
    SharedLinkConfigurationPtr config = linkManager()->addConfiguration(udpConfig);

    expectLogMessage("Comms.UDPLink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Could not resolve host: drone.invalid")));
    expectAppMessage(QRegularExpression(QStringLiteral("Could not resolve host: drone.invalid")));
    QVERIFY(linkManager()->createConnectedLink(config));
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testUdpUnresolvedHostAllowedStaysConnected()
{
    UDPConfiguration* const udpConfig = new UDPConfiguration(QStringLiteral("UnresolvedUdpAllowed"));
    udpConfig->setRequireResolvedHosts(false);
    udpConfig->addHost(QStringLiteral("drone.invalid"), 14550);
    SharedLinkConfigurationPtr config = linkManager()->addConfiguration(udpConfig);

    expectLogMessage("Comms.UDPLink", QtWarningMsg, QRegularExpression(QStringLiteral("Could not resolve host")));
    QVERIFY(linkManager()->createConnectedLink(config));
    QTRY_VERIFY_WITH_TIMEOUT(config->link() && config->link()->isConnected(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    linkManager()->removeConfiguration(config.get());
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
}

void LinkManagerTest::_testUdpBindFailureDisconnects()
{
    QUdpSocket blocker;
    QVERIFY(blocker.bind(QHostAddress::AnyIPv4, 0, QAbstractSocket::DontShareAddress));

    UDPConfiguration* const udpConfig = new UDPConfiguration(QStringLiteral("BindFailUdp"));
    udpConfig->setLocalPort(blocker.localPort());
    SharedLinkConfigurationPtr config = linkManager()->addConfiguration(udpConfig);

    expectAppMessage(QRegularExpression(QStringLiteral("Link BindFailUdp:")));
    _expectUdpBindFailureLogs();
    QVERIFY(linkManager()->createConnectedLink(config));
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    _verifyUdpBindFailureLogs();
    verifyExpectedLogMessage();

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testUdpAutoConnectBindFailureReusesConfig()
{
    linkManager()->init();
    QUdpSocket blocker;
    QVERIFY(blocker.bind(QHostAddress::AnyIPv4, 0, QAbstractSocket::DontShareAddress));

    AutoConnectSettings* const autoConnectSettings = SettingsManager::instance()->autoConnectSettings();
    const QVariant oldAutoConnectUDP = autoConnectSettings->autoConnectUDP()->rawValue();
    const QVariant oldListenPort = autoConnectSettings->udpListenPort()->rawValue();
    const QVariant oldTargetHost = autoConnectSettings->udpTargetHostIP()->rawValue();
    const auto restoreSettings = qScopeGuard([&] {
        autoConnectSettings->autoConnectUDP()->setRawValue(oldAutoConnectUDP);
        autoConnectSettings->udpListenPort()->setRawValue(oldListenPort);
        autoConnectSettings->udpTargetHostIP()->setRawValue(oldTargetHost);
    });
    autoConnectSettings->autoConnectUDP()->setRawValue(true);
    autoConnectSettings->udpListenPort()->setRawValue(blocker.localPort());
    autoConnectSettings->udpTargetHostIP()->setRawValue(QString());

    const auto defaultConfigs = [this] { return _configsNamed(LinkManager::_defaultUDPLinkName); };
    QVERIFY(defaultConfigs().isEmpty());

    _expectUdpBindFailureLogs();
    linkManager()->_addUDPAutoConnectLink();
    QCOMPARE(defaultConfigs().size(), 1);
    const SharedLinkConfigurationPtr config = defaultConfigs().constFirst();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    _verifyUdpBindFailureLogs();

    // First retry reuses the same config once the backoff allows it
    QTRY_VERIFY_WITH_TIMEOUT(config->reconnectReady(), TestTimeout::mediumMs());
    _expectUdpBindFailureLogs();
    linkManager()->_addUDPAutoConnectLink();
    QCOMPARE(defaultConfigs().size(), 1);
    QVERIFY(config->link());
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    _verifyUdpBindFailureLogs();

    // Next retry waits for the backoff
    linkManager()->_addUDPAutoConnectLink();
    QVERIFY(config->link() == nullptr);
    QCOMPARE(defaultConfigs().size(), 1);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testForwardingLinkUnresolvedHostRetries()
{
    linkManager()->init();
    MavlinkSettings* const mavlinkSettings = SettingsManager::instance()->mavlinkSettings();
    const QVariant oldForward = mavlinkSettings->forwardMavlink()->rawValue();
    const QVariant oldHost = mavlinkSettings->forwardMavlinkHostName()->rawValue();
    const auto restoreSettings = qScopeGuard([&] {
        mavlinkSettings->forwardMavlink()->setRawValue(oldForward);
        mavlinkSettings->forwardMavlinkHostName()->setRawValue(oldHost);
    });
    mavlinkSettings->forwardMavlink()->setRawValue(true);
    expectAppMessage(QRegularExpression(QStringLiteral("Restart application")));
    mavlinkSettings->forwardMavlinkHostName()->setRawValue(QStringLiteral("drone.invalid:14445"));
    verifyExpectedLogMessage();

    const auto forwardingConfigs = [this] { return _configsNamed(LinkManager::_mavlinkForwardingLinkName); };
    QVERIFY(forwardingConfigs().isEmpty());

    const QRegularExpression unresolved(QStringLiteral("Could not resolve host: drone.invalid"));
    expectLogMessage("Comms.UDPLink", QtWarningMsg, unresolved);
    expectAppMessage(unresolved);
    linkManager()->_addMAVLinkForwardingLink();
    QCOMPARE(forwardingConfigs().size(), 1);
    const SharedLinkConfigurationPtr config = forwardingConfigs().constFirst();
    QVERIFY(config->isForwarding());
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    // Retry reuses the config, resolves again, and does not pop up again
    QTRY_VERIFY_WITH_TIMEOUT(config->reconnectReady(), TestTimeout::mediumMs());
    expectLogMessage("Comms.UDPLink", QtWarningMsg, unresolved);
    linkManager()->_addMAVLinkForwardingLink();
    QCOMPARE(forwardingConfigs().size(), 1);
    QVERIFY(config->link());
    QVERIFY(config->isForwarding());
    // Checked before the lookup completes so slow DNS can't outlast the backoff
    QVERIFY(!config->reconnectReady());
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    // A new failure streak (backoff reset, as after a stable connection) pops up again
    config->resetReconnectBackoff();
    expectLogMessage("Comms.UDPLink", QtWarningMsg, unresolved);
    expectAppMessage(unresolved);
    linkManager()->_addMAVLinkForwardingLink();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testSupportForwardingFailureAllowsRetry()
{
    MavlinkSettings* const mavlinkSettings = SettingsManager::instance()->mavlinkSettings();
    const QVariant oldHost = mavlinkSettings->forwardMavlinkAPMSupportHostName()->rawValue();
    const auto restoreSettings =
        qScopeGuard([&] { mavlinkSettings->forwardMavlinkAPMSupportHostName()->setRawValue(oldHost); });
    mavlinkSettings->forwardMavlinkAPMSupportHostName()->setRawValue(QStringLiteral("drone.invalid:14445"));

    const auto supportConfigs = [this] { return _configsNamed(LinkManager::_mavlinkForwardingSupportLinkName); };
    QVERIFY(supportConfigs().isEmpty());

    const QRegularExpression unresolved(QStringLiteral("Could not resolve host: drone.invalid"));
    for (int attempt = 0; attempt < 2; attempt++) {
        expectLogMessage("Comms.UDPLink", QtWarningMsg, unresolved);
        expectAppMessage(unresolved);
        linkManager()->createMavlinkForwardingSupportLink();
        QTRY_VERIFY_WITH_TIMEOUT(!linkManager()->mavlinkSupportForwardingEnabled(), TestTimeout::mediumMs());
        QCOMPARE(supportConfigs().size(), 1);
        QVERIFY(supportConfigs().constFirst()->link() == nullptr);
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    }

    linkManager()->removeConfiguration(supportConfigs().constFirst().get());
}

void LinkManagerTest::_testDynamicUdpLinkIgnoresSameNamedUserLink()
{
    linkManager()->init();
    MavlinkSettings* const mavlinkSettings = SettingsManager::instance()->mavlinkSettings();
    const QVariant oldForward = mavlinkSettings->forwardMavlink()->rawValue();
    const auto restoreSettings = qScopeGuard([&] { mavlinkSettings->forwardMavlink()->setRawValue(oldForward); });
    mavlinkSettings->forwardMavlink()->setRawValue(true);

    UDPConfiguration* const userUdpConfig =
        new UDPConfiguration(QLatin1String(LinkManager::_mavlinkForwardingLinkName));
    userUdpConfig->setDynamic(false);
    const SharedLinkConfigurationPtr userConfig = linkManager()->addConfiguration(userUdpConfig);

    linkManager()->_addMAVLinkForwardingLink();

    QVERIFY(!userConfig->isDynamic());
    QVERIFY(!userConfig->isForwarding());
    QVERIFY(userConfig->link() == nullptr);
    const QList<SharedLinkConfigurationPtr> configs = _configsNamed(LinkManager::_mavlinkForwardingLinkName);
    QCOMPARE(configs.size(), 2);

    // Removing a config doesn't stop a UDP link that is still connecting, so let it connect first
    const auto dynamicIt =
        std::ranges::find_if(configs, [](const SharedLinkConfigurationPtr& c) { return c->isDynamic(); });
    QVERIFY(dynamicIt != configs.cend());
    const SharedLinkConfigurationPtr dynamicConfig = *dynamicIt;
    QTRY_VERIFY_WITH_TIMEOUT(dynamicConfig->link() && dynamicConfig->link()->isConnected(), TestTimeout::mediumMs());

    for (const SharedLinkConfigurationPtr& config : configs) {
        linkManager()->removeConfiguration(config.get());
    }
    QTRY_VERIFY_WITH_TIMEOUT(dynamicConfig->link() == nullptr, TestTimeout::mediumMs());
}

QList<SharedLinkConfigurationPtr> LinkManagerTest::_configsNamed(const char* name)
{
    QList<SharedLinkConfigurationPtr> configs;
    for (const SharedLinkConfigurationPtr& config : linkManager()->_rgLinkConfigs) {
        if (config->name() == QLatin1String(name)) {
            configs.append(config);
        }
    }
    return configs;
}

UT_REGISTER_TEST(LinkManagerTest, TestLabel::Integration, TestLabel::Comms)

#ifndef QGC_NO_SERIAL_LINK
#include "SerialAutoConnect.h"
#include "SerialLink.h"
#include "SerialPortManager.h"

void LinkManagerTest::_testSerialReservationFollowsLink()
{
    SerialPortManager ports;
    const QString name = QStringLiteral("/test/link-claim");
    auto config = std::make_shared<SerialConfiguration>(QStringLiteral("Reservation"));
    config->setPortName(name);
    SharedLinkConfigurationPtr sharedConfig = config;
    auto claim = ports.reservePort(name);
    QVERIFY(claim);
    {
        SerialLink link(sharedConfig, std::move(claim));
        QVERIFY(!claim);
        QVERIFY(ports.isPortReserved(name));
        QVERIFY(!ports.reservePort(name));
    }
    QVERIFY(ports.reservePort(name));
}

void LinkManagerTest::_testReservedSerialPortNotOpened()
{
    const QString port = QStringLiteral("/test/gps-reserved");
    auto reservation = SerialPortManager::instance()->reservePort(port);
    QVERIFY(reservation);
    auto config = std::make_shared<SerialConfiguration>(QStringLiteral("Reserved GPS port"));
    config->setPortName(port);
    SharedLinkConfigurationPtr sharedConfig = config;
    QVERIFY(!linkManager()->createConnectedLink(sharedConfig));
    QVERIFY(!config->link());
    QVERIFY(SerialPortManager::instance()->isPortReserved(port));
}

void LinkManagerTest::_testManualConnectToSerialPortInUseSaysWhy()
{
    const auto addSerialConfig = [this](const QString& name, const QString& port) {
        auto* const serialConfig = new SerialConfiguration(name);
        serialConfig->setPortName(port);
        return linkManager()->addConfiguration(serialConfig);
    };

    // Not every port in use has a link: the GPS reserves its port too, and says what it is.
    const QString reservedPort = QStringLiteral("/test/in-use");
    auto reservation = SerialPortManager::instance()->reservePort(reservedPort);
    QVERIFY(reservation);
    const SharedLinkConfigurationPtr config = addSerialConfig(QStringLiteral("Manual link"), reservedPort);
    const auto removeConfig = qScopeGuard([this, &config] { linkManager()->_removeConfiguration(config.get()); });

    expectAppMessage(QRegularExpression(QStringLiteral("Cannot connect 'Manual link': the serial port is in use\\.")));
    linkManager()->createConnectedLink(config.get());
    verifyExpectedLogMessage();
    QVERIFY(!config->link());

    reservation.reset();
    reservation = SerialPortManager::instance()->reservePort(reservedPort, QStringLiteral("the GPS receiver"));
    QVERIFY(reservation);
    expectAppMessage(QRegularExpression(
        QStringLiteral("Cannot connect 'Manual link': the serial port is in use by the GPS receiver\\.")));
    linkManager()->createConnectedLink(config.get());
    verifyExpectedLogMessage();
    QVERIFY(!config->link());
    reservation.reset();

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
    QVERIFY(master >= 0);
    const auto closeMaster = qScopeGuard([master] { ::close(master); });
    QCOMPARE(::grantpt(master), 0);
    QCOMPARE(::unlockpt(master), 0);
    const char* name = ::ptsname(master);
    QVERIFY(name);
    const QString location = QString::fromLocal8Bit(name);

    SharedLinkConfigurationPtr holder = addSerialConfig(QStringLiteral("Holder"), location);
    const SharedLinkConfigurationPtr second = addSerialConfig(QStringLiteral("Second link"), location);
    const auto removeLinks = qScopeGuard([this, &holder, &second] {
        if (holder->link()) {
            linkManager()->disconnectLink(holder->link());
            (void) QTest::qWaitFor([&holder] { return !holder->link(); }, TestTimeout::shortMs());
        }
        linkManager()->_removeConfiguration(second.get());
        linkManager()->_removeConfiguration(holder.get());
    });
    QVERIFY(linkManager()->createConnectedLink(holder));
    QTRY_VERIFY_WITH_TIMEOUT(holder->link() && holder->link()->isConnected(), TestTimeout::shortMs());

    expectAppMessage(QRegularExpression(
        QStringLiteral("Cannot connect 'Second link': its serial port is already open as the link 'Holder'")));
    linkManager()->createConnectedLink(second.get());
    verifyExpectedLogMessage();
    QVERIFY(!second->link());

    linkManager()->disconnectLink(holder->link());
    QTRY_VERIFY_WITH_TIMEOUT(!holder->link(), TestTimeout::shortMs());
#endif
}

void LinkManagerTest::_testSerialLinkSavedByStableName()
{
    SerialPortManager* const ports = SerialPortManager::instance();
    const auto restoreInventory = qScopeGuard([ports, enumerator = ports->_enumerator] {
        ports->_enumerator = enumerator;
        ports->rescan();
    });
    QString location = QStringLiteral("/test/usb/001/004");
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    // A port that opens, for a link to have it.
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
    QVERIFY(master >= 0);
    const auto closeMaster = qScopeGuard([master] { ::close(master); });
    QCOMPARE(::grantpt(master), 0);
    QCOMPARE(::unlockpt(master), 0);
    const char* name = ::ptsname(master);
    QVERIFY(name);
    location = QString::fromLocal8Bit(name);
#endif
    SerialPortManager::Port port{location, location, QGCSerialPortInfo::BoardTypeUnknown, QString()};
    port.stableName = QStringLiteral("usb:0403:6015:R1");
    ports->_enumerator = [&port] { return QList<SerialPortManager::Port>{port}; };
    ports->rescan();

    // What the link editor lists is what it saves a link by.
    QCOMPARE(linkManager()->serialPorts(), QStringList{port.stableName});

    const auto addSerialConfig = [this](const QString& configName, const QString& portName) {
        auto* const serialConfig = new SerialConfiguration(configName);
        serialConfig->setPortName(portName);
        return linkManager()->addConfiguration(serialConfig);
    };
    SharedLinkConfigurationPtr holder = addSerialConfig(QStringLiteral("Holder"), location);
    const SharedLinkConfigurationPtr second = addSerialConfig(QStringLiteral("Second link"), port.stableName);
    const auto removeLinks = qScopeGuard([this, &holder, &second] {
        if (holder->link()) {
            linkManager()->disconnectLink(holder->link());
            (void) QTest::qWaitFor([&holder] { return !holder->link(); }, TestTimeout::shortMs());
        }
        linkManager()->_removeConfiguration(second.get());
        linkManager()->_removeConfiguration(holder.get());
    });

    // A link saved by the system location of its port is saved by the stable name once it has been edited.
    const std::unique_ptr<LinkConfiguration> edited(linkManager()->startConfigurationEditing(holder.get()));
    const auto* const editedSerialConfig = qobject_cast<const SerialConfiguration*>(edited.get());
    QVERIFY(editedSerialConfig);
    QCOMPARE(editedSerialConfig->portName(), port.stableName);

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    // The link that has the port under one of its names is named to a link that wants it under another.
    QVERIFY(linkManager()->createConnectedLink(holder));
    QTRY_VERIFY_WITH_TIMEOUT(holder->link() && holder->link()->isConnected(), TestTimeout::shortMs());
    expectAppMessage(QRegularExpression(
        QStringLiteral("Cannot connect 'Second link': its serial port is already open as the link 'Holder'")));
    linkManager()->createConnectedLink(second.get());
    verifyExpectedLogMessage();
    QVERIFY(!second->link());

    linkManager()->disconnectLink(holder->link());
    QTRY_VERIFY_WITH_TIMEOUT(!holder->link(), TestTimeout::shortMs());
#endif
}

void LinkManagerTest::_testOccupiedSerialAutoConnectRecovers()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    linkManager()->init();
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
    QVERIFY(master >= 0);
    const auto closeMaster = qScopeGuard([master] { ::close(master); });
    QCOMPARE(::grantpt(master), 0);
    QCOMPARE(::unlockpt(master), 0);
    const char* name = ::ptsname(master);
    QVERIFY(name);
    const QString location = QString::fromLocal8Bit(name);
    QSerialPort occupied(location);
    QVERIFY(occupied.open(QIODevice::ReadWrite));
    const QList<SerialPortManager::Port> ports = {
        {location, location, QGCSerialPortInfo::BoardTypePixhawk, QStringLiteral("Test")}};
    const auto removePort = qScopeGuard([&] {
        linkManager()->_serialAutoConnect->_configs.remove(location);
        linkManager()->_serialAutoConnect->_waitingPorts.remove(location);
    });
    linkManager()->_serialAutoConnect->update(ports, {.pixhawk = true});
    linkManager()->_serialAutoConnect->_waitingPorts._deadlinesUs[location] = 0;
    linkManager()->_serialAutoConnect->update(ports, {.pixhawk = true});
    const auto config = linkManager()->_serialAutoConnect->_configs.value(location);
    QVERIFY(config);
    QCOMPARE(config->type(), LinkConfiguration::TypeSerial);
    QTRY_VERIFY_WITH_TIMEOUT(!config->link(), TestTimeout::shortMs());
    QCOMPARE(linkManager()->_serialAutoConnect->_configs.value(location), config);
    QVERIFY(!config->reconnectReady());
    linkManager()->_serialAutoConnect->update(ports, {.pixhawk = true});
    QVERIFY(!config->link());

    occupied.close();
    QTRY_VERIFY_WITH_TIMEOUT(config->reconnectReady(), TestTimeout::mediumMs());
    linkManager()->_serialAutoConnect->update(ports, {.pixhawk = true});
    QTRY_VERIFY_WITH_TIMEOUT(config->link() && config->link()->isConnected(), TestTimeout::shortMs());
    QCOMPARE(linkManager()->_serialAutoConnect->_configs.value(location), config);
    QVERIFY(!SerialPortManager::instance()->canAutoConnectPort(location));
    linkManager()->disconnectLink(config->link());
    QTRY_VERIFY_WITH_TIMEOUT(!config->link(), TestTimeout::shortMs());
#else
    QSKIP("Occupied serial reconnect coverage requires a Linux pseudo-terminal");
#endif
}
#endif
