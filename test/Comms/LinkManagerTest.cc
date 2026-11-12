#include "LinkManagerTest.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

#include <QtCore/QScopeGuard>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>
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

#include "Transport/Support/PseudoTerminal.h"
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

SharedLinkConfigurationPtr LinkManagerTest::_addSerialConfig(const QString& name, const QString& port)
{
    auto* const serialConfig = new SerialConfiguration(name);
    serialConfig->setPortName(port);
    return linkManager()->addConfiguration(serialConfig);
}

void LinkManagerTest::_verifyOpenPortRefusesSecondLink(const QString& holderPort, const QString& secondPort)
{
    SharedLinkConfigurationPtr holder = _addSerialConfig(QStringLiteral("Holder"), holderPort);
    const SharedLinkConfigurationPtr second = _addSerialConfig(QStringLiteral("Second link"), secondPort);
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
}

void LinkManagerTest::_testManualConnectToSerialPortInUseSaysWhy()
{
    // Not every port in use has a link: the GPS reserves its port too, and says what it is.
    const QString reservedPort = QStringLiteral("/test/in-use");
    auto reservation = SerialPortManager::instance()->reservePort(reservedPort);
    QVERIFY(reservation);
    const SharedLinkConfigurationPtr config = _addSerialConfig(QStringLiteral("Manual link"), reservedPort);
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
    GPSTest::PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    _verifyOpenPortRefusesSecondLink(terminal.slavePath(), terminal.slavePath());
#endif
}

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
/// While it lives, the serial ports are two pseudo-terminals, for links to really open, in place of the ports of
/// this computer.
struct LinkManagerTest::PseudoTerminalPorts
{
    explicit PseudoTerminalPorts(LinkManagerTest& test)
        : _test(test)
        , _enumerator(SerialPortManager::instance()->_enumerator)
        , _hasAccess(SerialPortManager::instance()->_hasAccess)
        , _requestAccess(SerialPortManager::instance()->_requestAccess)
        , _accessDenied(SerialPortManager::instance()->_accessDenied)
    {
        SerialPortManager* const ports = SerialPortManager::instance();
        ports->_enumerator = [this] {
            return QList<SerialPortManager::Port>{{first(), first(), QGCSerialPortInfo::BoardTypeUnknown, QString()},
                                                  {second(), second(), QGCSerialPortInfo::BoardTypeUnknown, QString()}};
        };
        ports->rescan();
    }

    ~PseudoTerminalPorts()
    {
        // The links go first: the scan below finds nothing to wait for, and would connect one that still waits
        // for its port.
        for (const SharedLinkConfigurationPtr& config : std::as_const(_links)) {
            // A link can be disconnected only once it has connected.
            (void) QTest::qWaitFor([&config] { return !config->link() || config->link()->isConnected(); },
                                   TestTimeout::shortMs());
            if (config->link()) {
                _test.linkManager()->disconnectLink(config->link());
                (void) QTest::qWaitFor([&config] { return !config->link(); }, TestTimeout::shortMs());
            }
            _test.linkManager()->_removeConfiguration(config.get());
        }
        _links.clear();

        SerialPortManager* const ports = SerialPortManager::instance();
        ports->_enumerator = _enumerator;
        ports->_hasAccess = _hasAccess;
        ports->_requestAccess = _requestAccess;
        ports->_accessDenied = _accessDenied;
        ports->rescan();
    }

    Q_DISABLE_COPY_MOVE(PseudoTerminalPorts)

    bool isValid() const { return _terminals[0].isValid() && _terminals[1].isValid(); }

    const QString& first() const { return _terminals[0].slavePath(); }

    const QString& second() const { return _terminals[1].slavePath(); }

    /// From now on QGC may open a port only once the user has let it, as Android has it for the port of a USB
    /// device. The test is the user: it adds the port to granted or to denied, and scans the ports as QGC does
    /// after an answer.
    void needAccess()
    {
        SerialPortManager* const ports = SerialPortManager::instance();
        ports->_hasAccess = [this](const QString& name) { return granted.contains(name); };
        ports->_requestAccess = [this](const QString& name) {
            if (granted.contains(name)) {
                return false;
            }
            asked.append(name);
            return true;
        };
        ports->_accessDenied = [this](const QString& name) { return denied.contains(name); };
    }

    /// A link for the port, which is disconnected and removed with this.
    SharedLinkConfigurationPtr addLink(const QString& name, const QString& port)
    {
        _links.append(_test._addSerialConfig(name, port));
        return _links.last();
    }

    QSet<QString> granted;
    QSet<QString> denied;
    QStringList asked;

private:
    LinkManagerTest& _test;
    GPSTest::PseudoTerminal _terminals[2];
    QList<SharedLinkConfigurationPtr> _links;
    const SerialPortManager::Enumerator _enumerator;
    const std::function<bool(const QString&)> _hasAccess;
    const std::function<bool(const QString&)> _requestAccess;
    const std::function<bool(const QString&)> _accessDenied;
};
#endif

void LinkManagerTest::_testManualSerialConnectOpensPortAtOnce()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    // The user is asked for no port of this platform, so the link has nothing to wait for.
    PseudoTerminalPorts ports(*this);
    QVERIFY(ports.isValid());
    const SharedLinkConfigurationPtr config = ports.addLink(QStringLiteral("Radio"), ports.first());

    linkManager()->createConnectedLink(config.get());
    QVERIFY(config->link());
    QTRY_VERIFY_WITH_TIMEOUT(config->link() && config->link()->isConnected(), TestTimeout::shortMs());
#else
    QSKIP("A serial link that connects requires a Linux pseudo-terminal");
#endif
}

void LinkManagerTest::_testManualSerialConnectWaitsForPortAccess_data()
{
    QTest::addColumn<bool>("stoppedBefore");

    QTest::newRow("link not connected before") << false;
    // The user wants the link now, whatever they stopped before.
    QTest::newRow("link that the user stopped before") << true;
}

void LinkManagerTest::_testManualSerialConnectWaitsForPortAccess()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(bool, stoppedBefore);

    PseudoTerminalPorts ports(*this);
    QVERIFY(ports.isValid());
    ports.needAccess();
    const SharedLinkConfigurationPtr config = ports.addLink(QStringLiteral("Radio"), ports.first());
    if (stoppedBefore) {
        linkManager()->disconnectLinkConfiguration(config.get());
    }

    linkManager()->createConnectedLink(config.get());
    QCOMPARE(ports.asked, QStringList{ports.first()});
    QVERIFY(!config->link());

    // The ports are scanned every second, also before the user has answered.
    SerialPortManager::instance()->rescan();
    QVERIFY(!config->link());

    ports.granted.insert(ports.first());
    SerialPortManager::instance()->rescan();
    QVERIFY(config->link());
    QTRY_VERIFY_WITH_TIMEOUT(config->link() && config->link()->isConnected(), TestTimeout::shortMs());
#else
    QSKIP("A serial link that connects requires a Linux pseudo-terminal");
#endif
}

void LinkManagerTest::_testManualSerialConnectEndsWhenPortAccessIsDenied_data()
{
    QTest::addColumn<bool>("stoppedMeanwhile");

    QTest::newRow("link still wanted") << false;
    // Nothing to tell of a link that the user no longer wants.
    QTest::newRow("link that the user stopped meanwhile") << true;
}

void LinkManagerTest::_testManualSerialConnectEndsWhenPortAccessIsDenied()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(bool, stoppedMeanwhile);

    PseudoTerminalPorts ports(*this);
    QVERIFY(ports.isValid());
    ports.needAccess();
    const SharedLinkConfigurationPtr config = ports.addLink(QStringLiteral("Radio"), ports.first());
    linkManager()->createConnectedLink(config.get());
    QCOMPARE(ports.asked, QStringList{ports.first()});
    if (stoppedMeanwhile) {
        linkManager()->disconnectLinkConfiguration(config.get());
    }

    ports.denied.insert(ports.first());
    if (!stoppedMeanwhile) {
        expectAppMessage(QRegularExpression(QStringLiteral("Cannot connect 'Radio': USB permission was denied\\.")));
    }
    SerialPortManager::instance()->rescan();
    if (!stoppedMeanwhile) {
        verifyExpectedLogMessage();
    }
    QVERIFY(!config->link());

    // The user was told that the link is not coming, so a grant by other means connects nothing by itself.
    ports.denied.clear();
    ports.granted.insert(ports.first());
    SerialPortManager::instance()->rescan();
    QVERIFY(!config->link());
#else
    QSKIP("A serial link that connects requires a Linux pseudo-terminal");
#endif
}

void LinkManagerTest::_testLinkWaitingForPortAccessNotConnected_data()
{
    QTest::addColumn<WhileWaitingForAccess>("meanwhile");

    QTest::newRow("a minute passed before the grant") << WhileWaitingForAccess::MinutePassed;
    QTest::newRow("link stopped by the user") << WhileWaitingForAccess::LinkStopped;
    QTest::newRow("connections suspended") << WhileWaitingForAccess::ConnectionsSuspended;
    // The user has the link they asked for once it is connected, so it is not connected again after it has dropped.
    QTest::newRow("link connected by other means, and dropped since") << WhileWaitingForAccess::LinkConnectedAndDropped;
}

void LinkManagerTest::_testLinkWaitingForPortAccessNotConnected()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(WhileWaitingForAccess, meanwhile);

    PseudoTerminalPorts ports(*this);
    QVERIFY(ports.isValid());
    ports.needAccess();
    SharedLinkConfigurationPtr config = ports.addLink(QStringLiteral("Radio"), ports.first());
    const auto allowConnections = qScopeGuard([this] { linkManager()->setConnectionsAllowed(); });
    linkManager()->createConnectedLink(config.get());

    switch (meanwhile) {
        case WhileWaitingForAccess::MinutePassed:
            linkManager()->_usbPermissionDeadline.setRemainingTime(0);
            break;
        case WhileWaitingForAccess::LinkStopped:
            linkManager()->disconnectLinkConfiguration(config.get());
            break;
        case WhileWaitingForAccess::ConnectionsSuspended:
            linkManager()->setConnectionsSuspended(QStringLiteral("Test"));
            break;
        case WhileWaitingForAccess::LinkConnectedAndDropped:
            // As when an automatic connection opens the port between the grant and the scan that follows it.
            ports.granted.insert(ports.first());
            QVERIFY(linkManager()->createConnectedLink(config));
            QTRY_VERIFY_WITH_TIMEOUT(config->link() && config->link()->isConnected(), TestTimeout::shortMs());
            SerialPortManager::instance()->rescan();
            config->link()->disconnect();
            QTRY_VERIFY_WITH_TIMEOUT(!config->link(), TestTimeout::shortMs());
            break;
    }

    ports.granted.insert(ports.first());
    SerialPortManager::instance()->rescan();
    QVERIFY(!config->link());
#else
    QSKIP("A serial link that connects requires a Linux pseudo-terminal");
#endif
}

void LinkManagerTest::_testLinkWaitingForPortAccessReplacedByOneThatAsks_data()
{
    QTest::addColumn<bool>("secondAsks");
    QTest::addColumn<bool>("firstConnects");

    QTest::newRow("second connection asks too") << true << false;
    // As for a device that the user has allowed before.
    QTest::newRow("second connection has nothing to ask for") << false << true;
}

void LinkManagerTest::_testLinkWaitingForPortAccessReplacedByOneThatAsks()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(bool, secondAsks);
    QFETCH(bool, firstConnects);

    PseudoTerminalPorts ports(*this);
    QVERIFY(ports.isValid());
    ports.needAccess();
    if (!secondAsks) {
        ports.granted.insert(ports.second());
    }
    const SharedLinkConfigurationPtr first = ports.addLink(QStringLiteral("First"), ports.first());
    const SharedLinkConfigurationPtr second = ports.addLink(QStringLiteral("Second"), ports.second());

    linkManager()->createConnectedLink(first.get());
    linkManager()->createConnectedLink(second.get());

    ports.granted = {ports.first(), ports.second()};
    SerialPortManager::instance()->rescan();
    QVERIFY(second->link());
    QCOMPARE(first->link() != nullptr, firstConnects);
#else
    QSKIP("A serial link that connects requires a Linux pseudo-terminal");
#endif
}

void LinkManagerTest::_testLinkWaitingForPortAccessTriesPortInUseAgain()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    PseudoTerminalPorts ports(*this);
    QVERIFY(ports.isValid());
    ports.needAccess();
    const SharedLinkConfigurationPtr config = ports.addLink(QStringLiteral("Radio"), ports.first());
    linkManager()->createConnectedLink(config.get());

    // As when an automatic connection is trying the port at the moment of the grant.
    auto reservation = SerialPortManager::instance()->reservePort(ports.first());
    QVERIFY(reservation);
    ports.granted.insert(ports.first());
    SerialPortManager::instance()->rescan();
    QVERIFY(!config->link());

    reservation.reset();
    SerialPortManager::instance()->rescan();
    QVERIFY(config->link());
#else
    QSKIP("A serial link that connects requires a Linux pseudo-terminal");
#endif
}

void LinkManagerTest::_testSerialPortFlowControlIsThatOfItsDriver()
{
    SerialPortManager* const ports = SerialPortManager::instance();
    const auto restoreInventory = qScopeGuard([ports, enumerator = ports->_enumerator] {
        ports->_enumerator = enumerator;
        ports->rescan();
    });
    // A flight controller on USB, whose driver on Android has no hardware flow control, and a radio's adapter.
    SerialPortManager::Port board{QStringLiteral("/test/usb/001/004"), QStringLiteral("001/004"),
                                  QGCSerialPortInfo::BoardTypeUnknown, QString()};
    board.stableName = QStringLiteral("usb:26ac:0011:B1");
    board.hardwareFlowControl = false;
    const SerialPortManager::Port radio{QStringLiteral("/test/usb/001/005"), QStringLiteral("001/005"),
                                        QGCSerialPortInfo::BoardTypeUnknown, QString()};
    ports->_enumerator = [&] { return QList<SerialPortManager::Port>{board, radio}; };
    ports->rescan();

    // The link editor asks by the name that it lists the port under, and a saved link may have another.
    for (const QString& name : {board.stableName, board.systemLocation, board.portName}) {
        QVERIFY2(!linkManager()->serialPortHasFlowControl(name), qPrintable(name));
    }
    QVERIFY(linkManager()->serialPortHasFlowControl(radio.systemLocation));
    // A port that is not attached cannot be asked, and is not refused what it may well have.
    QVERIFY(linkManager()->serialPortHasFlowControl(QStringLiteral("/test/absent")));
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
    GPSTest::PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    location = terminal.slavePath();
#endif
    SerialPortManager::Port port{location, location, QGCSerialPortInfo::BoardTypeUnknown, QString()};
    port.stableName = QStringLiteral("usb:0403:6015:R1");
    ports->_enumerator = [&port] { return QList<SerialPortManager::Port>{port}; };
    ports->rescan();

    // What the link editor lists is what it saves a link by.
    QCOMPARE(linkManager()->serialPorts(), QStringList{port.stableName});

    // A link saved by the system location of its port is saved by the stable name once it has been edited.
    const SharedLinkConfigurationPtr saved = _addSerialConfig(QStringLiteral("Saved by location"), location);
    const auto removeSaved = qScopeGuard([this, &saved] { linkManager()->_removeConfiguration(saved.get()); });
    const std::unique_ptr<LinkConfiguration> edited(linkManager()->startConfigurationEditing(saved.get()));
    const auto* const editedSerialConfig = qobject_cast<const SerialConfiguration*>(edited.get());
    QVERIFY(editedSerialConfig);
    QCOMPARE(editedSerialConfig->portName(), port.stableName);

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    // The link that has the port under one of its names is named to a link that wants it under another.
    _verifyOpenPortRefusesSecondLink(location, port.stableName);
#endif
}

QString LinkManagerTest::_savedSerialPort()
{
    QSettings settings;
    const QString root = LinkConfiguration::settingsRoot();
    if (settings.value(root + QStringLiteral("/count")).toInt() != 1) {
        return {};
    }
    SerialConfiguration saved(QStringLiteral("Saved"));
    saved.loadSettings(settings, root + QStringLiteral("/Link0"));
    return saved.portName();
}

void LinkManagerTest::_testSerialLinkSavedByStableNameOnceConnected_data()
{
    QTest::addColumn<QString>("stableName");
    QTest::addColumn<bool>("dynamic");
    QTest::addColumn<bool>("takesStableName");

    const QString stableName = QStringLiteral("usb:0403:6015:R1");
    QTest::newRow("port gained a stable name") << stableName << false << true;
    // As on every platform but Android.
    QTest::newRow("port without a stable name") << QString() << false << false;
    // Automatic connection makes such a configuration for the system location of a port, and it is not saved.
    QTest::newRow("dynamic configuration") << stableName << true << false;
}

void LinkManagerTest::_testSerialLinkSavedByStableNameOnceConnected()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QFETCH(QString, stableName);
    QFETCH(bool, dynamic);
    QFETCH(bool, takesStableName);

    SerialPortManager* const ports = SerialPortManager::instance();
    const auto restoreInventory = qScopeGuard([ports, enumerator = ports->_enumerator] {
        ports->_enumerator = enumerator;
        ports->rescan();
    });
    GPSTest::PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    const QString location = terminal.slavePath();
    SerialPortManager::Port port{location, location, QGCSerialPortInfo::BoardTypeUnknown, QString()};
    ports->_enumerator = [&port] { return QList<SerialPortManager::Port>{port}; };
    ports->rescan();

    // Android gives a port its stable name once the user lets QGC open the device, which may be after the link
    // was made. It scans the ports then.
    SharedLinkConfigurationPtr config = _addSerialConfig(QStringLiteral("Radio"), location);
    config->setDynamic(dynamic);
    const auto removeLink = qScopeGuard([this, &config] {
        if (config->link()) {
            linkManager()->disconnectLink(config->link());
            (void) QTest::qWaitFor([&config] { return !config->link(); }, TestTimeout::shortMs());
        }
        linkManager()->_removeConfiguration(config.get());
    });
    port.stableName = stableName;
    ports->rescan();

    QVERIFY(linkManager()->createConnectedLink(config));
    QSignalSpy connected(config->link(), &LinkInterface::connected);
    QVERIFY_SIGNAL_COUNT_WAIT(connected, 1, TestTimeout::shortMs());

    const auto* const serialConfig = qobject_cast<const SerialConfiguration*>(config.get());
    QVERIFY(serialConfig);
    QCOMPARE(serialConfig->portName(), takesStableName ? stableName : location);
    // Nothing has saved the list before, so it is there only if the connection wrote it.
    QCOMPARE(_savedSerialPort(), takesStableName ? stableName : QString());

    if (takesStableName) {
        // The link still has the port, which it opened by the system location: a link that wants the port by
        // that name is told which link has it.
        const SharedLinkConfigurationPtr second = _addSerialConfig(QStringLiteral("Second link"), location);
        const auto removeSecond = qScopeGuard([this, &second] { linkManager()->_removeConfiguration(second.get()); });
        expectAppMessage(QRegularExpression(
            QStringLiteral("Cannot connect 'Second link': its serial port is already open as the link 'Radio'")));
        linkManager()->createConnectedLink(second.get());
        verifyExpectedLogMessage();
    }
#else
    QSKIP("A serial link that connects requires a Linux pseudo-terminal");
#endif
}

void LinkManagerTest::_testSerialLinkSavedByStableNameWhenEditorCloses_data()
{
    QTest::addColumn<bool>("newLink");

    QTest::newRow("edited link") << false;
    QTest::newRow("new link") << true;
}

void LinkManagerTest::_testSerialLinkSavedByStableNameWhenEditorCloses()
{
    QFETCH(bool, newLink);

    SerialPortManager* const ports = SerialPortManager::instance();
    const auto restoreInventory = qScopeGuard([ports, enumerator = ports->_enumerator] {
        ports->_enumerator = enumerator;
        ports->rescan();
    });
    const QString location = QStringLiteral("/test/usb/001/004");
    SerialPortManager::Port port{location, location, QGCSerialPortInfo::BoardTypeUnknown, QString()};
    ports->_enumerator = [&port] { return QList<SerialPortManager::Port>{port}; };
    ports->rescan();

    const QString name = QStringLiteral("Radio");
    LinkConfiguration* saved = newLink ? nullptr : _addSerialConfig(name, location).get();
    const auto removeSaved = qScopeGuard([this, &saved] {
        if (saved) {
            linkManager()->_removeConfiguration(saved);
        }
    });
    std::unique_ptr<LinkConfiguration> edited(
        newLink ? linkManager()->createConfiguration(LinkConfiguration::TypeSerial, name)
                : linkManager()->startConfigurationEditing(saved));
    auto* const editedSerialConfig = qobject_cast<SerialConfiguration*>(edited.get());
    QVERIFY(editedSerialConfig);
    // The editor has the port by its system location, the only name the port had when it was chosen.
    editedSerialConfig->setPortName(location);
    port.stableName = QStringLiteral("usb:0403:6015:R1");
    ports->rescan();

    if (newLink) {
        saved = edited.release();
        linkManager()->endCreateConfiguration(saved);
    } else {
        linkManager()->endConfigurationEditing(saved, edited.release());
    }
    QCOMPARE(_savedSerialPort(), port.stableName);
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
