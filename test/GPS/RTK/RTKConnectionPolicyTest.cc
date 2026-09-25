#include "RTKConnectionPolicyTest.h"

#include <atomic>
#include <memory>
#include <thread>

#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRtk.h"
#include "GPSTransport.h"
#include "NTRIPManager.h"
#include "PositionManager.h"
#include "RTKConnectionPolicy.h"
#include "RTKSettings.h"
#include "SerialPortManager.h"
#include "SettingsManager.h"

namespace {

RTKSettings* rtkSettings()
{
    return SettingsManager::instance()->rtkSettings();
}

AutoConnectSettings* autoConnectSettings()
{
    return SettingsManager::instance()->autoConnectSettings();
}

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

void saveReceiverSettings(TestFixtures::SettingsFixture& saved, bool autoConnect = true)
{
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    auto* rtk = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), autoConnect);
    saved.setFactValue(rtk->receiverRole(), GPSRtk::ConfiguredBase);
    saved.setFactValue(rtk->connectionType(), GPSRtk::Serial);
    saved.setFactValue(rtk->baseReceiverManufacturers(), rtk->baseReceiverManufacturers()->rawValue());
    saved.setFactValue(rtk->serialDevice(), rtk->serialDevice()->rawValue());
    saved.setFactValue(rtk->serialBaudRate(), rtk->serialBaudRate()->rawValue());
}

}  // namespace

/// A receiver whose serial transport never opens real hardware.
struct RTKConnectionPolicyTest::Fixture
{
    using Owner = RTKConnectionPolicy::Owner;

    enum class Open
    {
        // The session stays up until it is retired.
        Hold,
        // The transport fails to open, so the session ends by itself.
        Fail,
    };

    explicit Fixture(SerialPortManager::Enumerator enumerator, Open open = Open::Hold)
        : ports(nullptr, std::move(enumerator))
        , receiver(std::make_unique<GPSRtk>(rtkSettings(), autoConnectSettings()))
    {
        receiver->setSerialPortManager(&ports);
        receiver->_serialTransportFactory = [open](const QString&, const std::atomic_bool& stop) {
            while (open == Open::Hold && !stop.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return std::unique_ptr<GPSTransport>{};
        };
        policy()->_connectDelayMs = 0;
    }

    RTKConnectionPolicy* policy() const { return receiver->connectionPolicy(); }

    quint64 sessions() const { return receiver->_sessionCount; }

    bool tick()
    {
        policy()->update();
        return true;
    }

    SerialPortManager ports;
    std::unique_ptr<GPSRtk> receiver;
};

void RTKConnectionPolicyTest::init()
{
    UnitTest::init();
    ignoreLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport|session ended")));
}

void RTKConnectionPolicyTest::_discoveryUnplugAndDisable()
{
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    QList<Port> inventory{rtkPort()};
    Fixture fixture([&]() { return inventory; });
    auto* policy = fixture.policy();
    policy->update();
    QCOMPARE(fixture.sessions(), 0U);
    policy->update();
    QCOMPARE(fixture.sessions(), 1U);
    QCOMPARE(fixture.receiver->activeEndpoint(), QStringLiteral("/test/rtk"));
    QCOMPARE(policy->_owner, Fixture::Owner::Auto);
    policy->update();
    QCOMPARE(fixture.sessions(), 1U);

    inventory.clear();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.tick() && !fixture.receiver->hasReceiver(), TestTimeout::mediumMs());
    QCOMPARE(policy->_owner, Fixture::Owner::None);
    QVERIFY(fixture.receiver->errorMessage().contains(QStringLiteral("unplugged")));
    inventory.append(rtkPort());
    QTRY_VERIFY_WITH_TIMEOUT(fixture.tick() && fixture.sessions() == 2, TestTimeout::mediumMs());

    SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS()->setRawValue(false);
    policy->update();
    QVERIFY(!fixture.receiver->hasReceiver());
    QCOMPARE(policy->_owner, Fixture::Owner::None);
    policy->update();
    QCOMPARE(fixture.sessions(), 2U);
}

void RTKConnectionPolicyTest::_tcpModeSkipsSerialDiscovery()
{
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    auto* connectionType = SettingsManager::instance()->rtkSettings()->connectionType();
    Fixture fixture([] { return QList<Port>{rtkPort()}; });
    auto* policy = fixture.policy();
    policy->update();
    policy->update();
    QCOMPARE(fixture.sessions(), 1U);
    connectionType->setRawValue(GPSRtk::Tcp);
    policy->update();
    QVERIFY(!fixture.receiver->hasReceiver());
    policy->update();
    policy->update();
    QCOMPARE(fixture.sessions(), 1U);
    connectionType->setRawValue(GPSRtk::Serial);
    policy->update();
    policy->update();
    QCOMPARE(fixture.sessions(), 2U);
}

void RTKConnectionPolicyTest::_excludedPorts_data()
{
    QTest::addColumn<QString>("reason");
    for (const auto* reason : {"bootloader", "busy", "passive-role", "single-port", "other-board"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
}

void RTKConnectionPolicyTest::_excludedPorts()
{
    QFETCH(QString, reason);
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    if (reason == QStringLiteral("passive-role")) {
        // Discovery configures bases; it never replaces a receiver QGroundControl must not write to.
        SettingsManager::instance()->rtkSettings()->receiverRole()->setRawValue(GPSRtk::PositionOnly);
    }
    Port port = rtkPort();
    port.bootloader = reason == QStringLiteral("bootloader");
    if (reason == QStringLiteral("other-board")) {
        port.boardType = QGCSerialPortInfo::BoardTypePixhawk;
    }
    Fixture fixture([&]() { return QList<Port>{port}; });
    (void) fixture.ports.availablePorts();
    SerialPortManager::ReservationPtr reservation;
    if (reason == QStringLiteral("busy")) {
        reservation = fixture.ports.reservePort(port.systemLocation);
    } else if (reason == QStringLiteral("single-port")) {
        fixture.ports.setSinglePortOnly(true);
        reservation = fixture.ports.reservePort(QStringLiteral("/test/mavlink"));
    }
    fixture.policy()->update();
    fixture.policy()->update();
    QCOMPARE(fixture.sessions(), 0U);
}

UT_REGISTER_TEST(RTKConnectionPolicyTest, TestLabel::Unit)

void RTKConnectionPolicyTest::_failedAttemptsBackOffAndRespectReservations()
{
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    Fixture fixture([] { return QList<Port>{rtkPort()}; }, Fixture::Open::Fail);
    auto* policy = fixture.policy();
    auto* receiver = fixture.receiver.get();
    const auto failedAttempt = [&]() {
        QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::mediumMs());
        QVERIFY(!policy->_retryDeadline.isForever());
        QCOMPARE(policy->_owner, Fixture::Owner::Auto);
    };
    policy->update();
    policy->update();
    QCOMPARE(fixture.sessions(), 1U);
    failedAttempt();
    QCOMPARE(policy->_retryDelayMs, 2000);
    policy->update();
    QCOMPARE(fixture.sessions(), 1U);
    QTRY_VERIFY_WITH_TIMEOUT(fixture.tick() && fixture.sessions() == 2, TestTimeout::mediumMs());
    failedAttempt();
    QCOMPARE(policy->_retryDelayMs, 4000);

    // A failed worker releases its port once it exits; a claim held elsewhere blocks retries.
    SerialPortManager::ReservationPtr claim;
    QTRY_VERIFY_WITH_TIMEOUT((claim = fixture.ports.reservePort(QStringLiteral("/test/rtk"))) != nullptr,
                             TestTimeout::mediumMs());
    policy->_retryDeadline.setRemainingTime(0);
    policy->update();
    QCOMPARE(fixture.sessions(), 2U);
    claim.reset();
    policy->update();
    QCOMPARE(fixture.sessions(), 3U);
    for (const int delay : {8000, 16000, 30000, 30000}) {
        failedAttempt();
        QCOMPARE(policy->_retryDelayMs, delay);
        policy->_retryDeadline.setRemainingTime(0);
        const auto attempts = fixture.sessions();
        QTRY_VERIFY_WITH_TIMEOUT(fixture.tick() && fixture.sessions() == attempts + 1, TestTimeout::mediumMs());
    }
    failedAttempt();

    // Choosing a passive role ends auto-connect ownership.
    SettingsManager::instance()->rtkSettings()->receiverRole()->setRawValue(GPSRtk::Passive);
    policy->_retryDeadline.setRemainingTime(0);
    const auto attempts = fixture.sessions();
    policy->update();
    QCOMPARE(fixture.sessions(), attempts);
    QCOMPARE(policy->_owner, Fixture::Owner::None);
    QVERIFY(policy->_autoPort.isEmpty());
    QCOMPARE(policy->_retryDelayMs, 1000);
    QVERIFY(policy->_retryDeadline.isForever());
}

void RTKConnectionPolicyTest::_compositeReceiverSelection_data()
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

void RTKConnectionPolicyTest::_compositeReceiverSelection()
{
    QFETCH(QString, scenario);
    QFETCH(bool, connectSecond);
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
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
    Fixture fixture([&] { return QList<Port>{first, second}; });
    auto claim = fixture.ports.reservePort(first.systemLocation);
    QVERIFY(claim);
    fixture.policy()->update();
    fixture.policy()->update();
    QCOMPARE(fixture.sessions(), connectSecond ? 1U : 0U);
    if (connectSecond) {
        QCOMPARE(fixture.receiver->activeEndpoint(), second.systemLocation);
    }
}

void RTKConnectionPolicyTest::_genericUsbNeedsExplicitSelection_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::newRow("unicore") << 5;
    QTest::newRow("quectel") << 6;
}

void RTKConnectionPolicyTest::_genericUsbNeedsExplicitSelection()
{
    QFETCH(int, manufacturer);
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    auto* rtkSettings = SettingsManager::instance()->rtkSettings();
    rtkSettings->baseReceiverManufacturers()->setRawValue(manufacturer);
    rtkSettings->serialDevice()->setRawValue(QStringLiteral("/test/ch340"));
    const QList<Port> inventory{genericPort(QStringLiteral("/test/ch340")), genericPort(QStringLiteral("/test/ftdi")),
                                rtkPort(QStringLiteral("/test/known"))};
    Fixture fixture([&] { return inventory; });
    fixture.policy()->update();
    fixture.policy()->update();
    QCOMPARE(fixture.sessions(), 1U);
    QCOMPARE(fixture.receiver->activeEndpoint(), QStringLiteral("/test/known"));
    QVERIFY(fixture.ports.canReservePort(QStringLiteral("/test/ch340")));
    QVERIFY(fixture.ports.canReservePort(QStringLiteral("/test/ftdi")));
}

void RTKConnectionPolicyTest::_manualConnectionRetiresAutoOwnership()
{
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    Fixture fixture([] { return QList<Port>{rtkPort(QStringLiteral("/test/known"))}; });
    auto* policy = fixture.policy();
    policy->update();
    policy->update();
    QCOMPARE(fixture.sessions(), 1U);
    fixture.receiver->disconnectConfiguredGPS();
    QVERIFY(!fixture.receiver->hasReceiver());
    QVERIFY(!settings->autoConnectRTKGPS()->rawValue().toBool());
    QCOMPARE(policy->_owner, Fixture::Owner::None);
    policy->update();
    policy->stop();
    QCOMPARE(fixture.sessions(), 1U);
    settings->autoConnectRTKGPS()->setRawValue(true);
    policy->update();
    policy->update();
    QCOMPARE(fixture.sessions(), 2U);
}

void RTKConnectionPolicyTest::_manualRetryWaitsForReturningPort()
{
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved, false);
    auto* rtk = SettingsManager::instance()->rtkSettings();
    rtk->receiverRole()->setRawValue(GPSRtk::Passive);
    rtk->serialDevice()->setRawValue(QStringLiteral("/test/manual"));
    rtk->serialBaudRate()->setRawValue(115200);
    QList<Port> inventory{genericPort(QStringLiteral("/test/manual"))};
    Fixture fixture([&] { return inventory; });
    auto* policy = fixture.policy();
    auto* receiver = fixture.receiver.get();
    QVERIFY(receiver->connectConfiguredGPS());
    QCOMPARE(policy->_owner, Fixture::Owner::Manual);
    emit receiver->_session.provider->receiverReady();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(policy->_established);

    inventory.clear();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ports.availablePorts().isEmpty() && !receiver->hasReceiver(),
                             TestTimeout::mediumMs());
    QVERIFY(receiver->reconnecting());
    QVERIFY(policy->_waitingForPort);
    QVERIFY(receiver->errorMessage().contains(QStringLiteral("plugged back in")));
    policy->_retryDeadline.setRemainingTime(0);
    policy->update();
    QCOMPARE(fixture.sessions(), 1U);

    inventory.append(genericPort(QStringLiteral("/test/manual")));
    QTRY_VERIFY_WITH_TIMEOUT(fixture.tick() && fixture.sessions() == 2, TestTimeout::mediumMs());
    QCOMPARE(receiver->activeEndpoint(), QStringLiteral("/test/manual"));
    QVERIFY(!policy->_waitingForPort);
    QCOMPARE(policy->_owner, Fixture::Owner::Manual);
    receiver->disconnectConfiguredGPS();
    QVERIFY(!receiver->reconnecting());
    QCOMPARE(policy->_owner, Fixture::Owner::None);
}

void RTKConnectionPolicyTest::_notificationSupersedesDiscovery_data()
{
    QTest::addColumn<bool>("disconnecting");
    QTest::addColumn<QString>("action");
    for (const bool disconnecting : {false, true}) {
        for (const auto* action : {"delete", "stop", "update"}) {
            const QByteArray name = QByteArray(disconnecting ? "disconnect-" : "enumeration-") + action;
            QTest::newRow(name.constData()) << disconnecting << QString::fromLatin1(action);
        }
    }
}

void RTKConnectionPolicyTest::_notificationSupersedesDiscovery()
{
    QFETCH(bool, disconnecting);
    QFETCH(QString, action);
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    auto* role = SettingsManager::instance()->rtkSettings()->receiverRole();
    Fixture fixture([] { return QList<Port>{rtkPort()}; });
    if (disconnecting) {
        fixture.policy()->update();
        fixture.policy()->update();
        QCOMPARE(fixture.sessions(), 1U);
        // A passive role disables discovery, so the next tick retires the discovered receiver.
        role->setRawValue(GPSRtk::PositionOnly);
    }
    bool notified = false;
    const auto supersede = [&] {
        if (std::exchange(notified, true)) {
            return;
        }
        if (action == QStringLiteral("delete")) {
            fixture.receiver.reset();
        } else if (action == QStringLiteral("stop")) {
            fixture.policy()->stop();
        } else {
            role->setRawValue(GPSRtk::ConfiguredBase);
            fixture.policy()->update();
        }
    };
    const auto notification = disconnecting
                                  ? connect(fixture.receiver.get(), &GPSRtk::receiverChanged, this, supersede)
                                  : connect(&fixture.ports, &SerialPortManager::portsEnumerated, this, supersede);
    fixture.policy()->update();
    disconnect(notification);
    QVERIFY(notified);
    if (action == QStringLiteral("delete")) {
        QVERIFY(!fixture.receiver);
        return;
    }
    QCOMPARE(fixture.sessions(), disconnecting ? 1U : 0U);
    if (action == QStringLiteral("stop")) {
        QVERIFY(fixture.policy()->_waitingPorts.isEmpty());
    } else {
        const quint64 expected = disconnecting ? 2 : 1;
        QTRY_VERIFY_WITH_TIMEOUT(fixture.tick() && fixture.sessions() == expected, TestTimeout::mediumMs());
    }
}

void RTKConnectionPolicyTest::_shutdownDuringConnectionTick()
{
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    auto* applicationCorrections = GPSManager::instance()->corrections();
    const auto restoreCorrections = qScopeGuard(
        [applicationCorrections] { GPSManager::instance()->ntrip()->setCorrectionManager(applicationCorrections); });
    int enumerations = 0;
    SerialPortManager ports(nullptr, [&] {
        ++enumerations;
        return QList<Port>{rtkPort()};
    });
    GPSManager manager;
    manager.gpsRtk()->setSerialPortManager(&ports);
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

void RTKConnectionPolicyTest::_connectSavedWaitsForReceiver()
{
    TestFixtures::SettingsFixture saved;
    // Auto-connect only discovers configured bases, so it does not replace the startup wait here.
    saveReceiverSettings(saved);
    auto* rtk = SettingsManager::instance()->rtkSettings();
    rtk->receiverRole()->setRawValue(GPSRtk::PositionOnly);
    rtk->serialDevice()->setRawValue(QStringLiteral("/test/startup"));
    rtk->serialBaudRate()->setRawValue(4800);
    QList<Port> inventory;
    Fixture fixture([&] { return inventory; });
    auto* policy = fixture.policy();
    auto* receiver = fixture.receiver.get();
    QSignalSpy changes(receiver, &GPSRtk::receiverChanged);
    // An absent receiver at startup is awaited like a lost one.
    policy->connectSaved();
    QCOMPARE(policy->_owner, Fixture::Owner::Manual);
    QVERIFY(policy->_established);
    QVERIFY(policy->_waitingForPort);
    QVERIFY(receiver->reconnecting());
    QVERIFY(!changes.isEmpty());
    QCOMPARE(fixture.sessions(), 0U);
    inventory.append(genericPort(QStringLiteral("/test/startup")));
    QTRY_VERIFY_WITH_TIMEOUT(fixture.tick() && fixture.sessions() == 1, TestTimeout::mediumMs());
    QCOMPARE(receiver->activeEndpoint(), QStringLiteral("/test/startup"));
    QCOMPARE(receiver->activeRole(), GPSRtk::PositionOnly);
    QVERIFY(autoConnectSettings()->autoConnectRTKGPS()->rawValue().toBool());
    receiver->disconnectConfiguredGPS();
    QVERIFY(!receiver->reconnecting());
    QCOMPARE(policy->_owner, Fixture::Owner::None);
}

void RTKConnectionPolicyTest::_connectSavedKeepsDiscovery_data()
{
    QTest::addColumn<bool>("present");
    QTest::newRow("receiver-present") << true;
    QTest::newRow("receiver-absent") << false;
}

void RTKConnectionPolicyTest::_connectSavedKeepsDiscovery()
{
    QFETCH(bool, present);
    TestFixtures::SettingsFixture saved;
    saveReceiverSettings(saved);
    saved.setFactValue(rtkSettings()->serialDevice(), QStringLiteral("/test/rtk"));
    Fact* const autoConnect = autoConnectSettings()->autoConnectRTKGPS();
    QList<Port> inventory;
    if (present) {
        inventory.append(rtkPort());
    }
    Fixture fixture([&] { return inventory; });
    auto* policy = fixture.policy();
    policy->connectSaved();
    // A startup connection is not a user choice, so auto-connect stays on.
    QVERIFY(autoConnect->rawValue().toBool());
    QVERIFY(!fixture.receiver->reconnecting());
    if (present) {
        QCOMPARE(fixture.sessions(), 1U);
        QCOMPARE(policy->_owner, Fixture::Owner::Manual);
        return;
    }
    // Waiting for the saved receiver would suspend discovery.
    QCOMPARE(fixture.sessions(), 0U);
    QCOMPARE(policy->_owner, Fixture::Owner::None);
    inventory.append(rtkPort());
    QTRY_VERIFY_WITH_TIMEOUT(fixture.tick() && fixture.sessions() == 1, TestTimeout::mediumMs());
    QCOMPARE(policy->_owner, Fixture::Owner::Auto);
    QVERIFY(autoConnect->rawValue().toBool());
}
