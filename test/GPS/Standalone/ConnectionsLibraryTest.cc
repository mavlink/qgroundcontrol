#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GPSReceiverAutoConnect.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"

#ifndef QGC_NO_SERIAL_LINK
class SerialInventory final : public GPSSerialDiscovery
{
public:
    QList<Port> ports;
    QMap<QString, std::weak_ptr<const Lease>> leases;
    QMap<QString, std::weak_ptr<const Lease>> exclusions;

    QList<Port> availablePorts() override { return ports; }

    ReservationPtr reservePort(const QString& name) override
    {
        if (!canReservePort(name))
            return {};
        auto lease = std::make_shared<Lease>();
        leases[name] = lease;
        return lease;
    }

    ReservationPtr excludeFromAutoConnect(const QString& name) override
    {
        auto lease = std::make_shared<Lease>();
        exclusions[name] = lease;
        return lease;
    }

    bool canReservePort(const QString& name) const override { return leases.value(name).expired(); }

    bool canAutoConnectPort(const QString& name) const override { return !isAutoConnectExcluded(name); }

    bool isAutoConnectExcluded(const QString& name) const override { return !exclusions.value(name).expired(); }
};
#endif

class ConnectionsLibraryTest : public QObject
{
    Q_OBJECT
private slots:
#ifndef QGC_NO_SERIAL_LINK
    void arrivalAdmissionAndCancellation()
    {
        ManualScheduler clock;
        SerialInventory inventory;
        GPSReceiverSession session;
        GPSReceiverAutoConnect control(&session, nullptr, nullptr, &clock);
        control.setSerialDiscovery(&inventory);
        // Cancel on admission, before any real transport or worker can be opened.
        QSignalSpy requested(&control, &GPSReceiverAutoConnect::connectRequested);
        connect(&control, &GPSReceiverAutoConnect::connectRequested, &control, [&] { control.disconnectSelected(); });
        control.setAutoConnect(true);
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 0);
        const QString device = QStringLiteral("/test/ublox");
        inventory.ports = {{ device, QStringLiteral("ublox"), QStringLiteral("U-blox"), true }};
        auto busy = inventory.reservePort(device);
        emit inventory.serialPortsChanged();
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 0);
        busy.reset();
        control.setSuspended(true);
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 0);
        control.setSuspended(false);
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 1);
        QVERIFY(!session.hasReceiver());
        QVERIFY(inventory.canReservePort(device));
        inventory.ports.clear();
        emit inventory.serialPortsChanged();
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        inventory.ports = {{ device, QStringLiteral("ublox"), QStringLiteral("U-blox"), true }};
        emit inventory.serialPortsChanged();
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 1);
        QVERIFY(control.connectSelected());
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 2);
        control.stop();
        QCOMPARE(clock.pendingCount(), 0);
    }

    void explicitSelectionAndHandover()
    {
        ManualScheduler clock;
        SerialInventory inventory;
        const QString device = QStringLiteral("/test/generic");
        inventory.ports = {{ device, QStringLiteral("generic"), QStringLiteral("Unidentified"), false }};
        GPSReceiverSession session;
        GPSReceiverAutoConnect control(&session, nullptr, nullptr, &clock);
        control.setSerialDiscovery(&inventory);
        QSignalSpy requested(&control, &GPSReceiverAutoConnect::connectRequested);
        connect(&control, &GPSReceiverAutoConnect::connectRequested, &control, [&] { control.disconnectSelected(); });
        control.setAutoConnect(true);
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 0);
        GPSReceiverProfile profile;
        profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Serial;
        profile.endpoint.device = device;
        profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure;
        profile.receiver.role = GPSReceiverConfig::Role::Position;
        control.setProfile(profile, true);
        auto nmeaLease = inventory.reservePort(device);
        auto nmeaExclusion = inventory.excludeFromAutoConnect(device);
        QVERIFY(control.connectSelected());
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 0);
        nmeaLease.reset();
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 0);
        nmeaExclusion.reset();
        QVERIFY(clock.advanceBy(std::chrono::seconds(10)));
        QCOMPARE(requested.count(), 1);
        QCOMPARE(requested.first().first().toString(), device);
        control.stop();
    }
#else
    void serialUnavailable()
    {
        GPSReceiverSession session;
        GPSReceiverAutoConnect control(&session);
        QVERIFY(!control.connectSelected());
        QVERIFY(!session.hasReceiver());
    }
#endif
};
QTEST_GUILESS_MAIN(ConnectionsLibraryTest)
#include "ConnectionsLibraryTest.moc"
