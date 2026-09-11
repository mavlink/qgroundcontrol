#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GPSProtocolFeatures.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverFamily.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "NMEAConnectionAttempt.h"

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

    void cancellationPreservesUncertainWriteEvidence()
    {
        GPSConfigurationResult configured;
        configured.status = GPSConfigurationStatus::TransportError;
        configured.transportWrite = GPSWriteResult{GPSWriteStatus::Cancelled, 4, 2, 2};
        const auto cancelled = GPSReceiverFailure::from(7, GPSConnectionError::ConfigFailed, {}, {}, configured);
        QCOMPARE(cancelled.retry, GPSRetryDisposition::Cancel);
        QCOMPARE(cancelled.cause, GPSReceiverFailure::Cause::Cancelled);
        QCOMPARE(cancelled.configuration->transportWrite->uncertainBytes, 2);
        configured.transportWrite->status = GPSWriteStatus::TimedOut;
        const auto uncertain = GPSReceiverFailure::from(7, GPSConnectionError::ConfigFailed, {}, {}, configured);
        QCOMPARE(uncertain.retry, GPSRetryDisposition::Retry);
        QCOMPARE(uncertain.cause, GPSReceiverFailure::Cause::UncertainWrite);
    }

    void configuredNmeaPreservesTerminalFailure()
    {
        if (!QGC_GPS_ENABLE_UBX)
            QSKIP("Configured NMEA output is implemented by the UBX backend");
        GPSReceiverProfile profile;
        profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Tcp;
        profile.endpoint.host = QStringLiteral("localhost");
        profile.endpoint.port = 1;
        profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure;
        profile.receiver.role = GPSReceiverConfig::Role::Position;
        profile.receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
        NMEAConnectionAttempt attempt(profile, nullptr, 27);
        QSemaphore entered;
        QSemaphore release;
        const auto cleanup = qScopeGuard([&] {
            attempt.stop();
            release.release();
            attempt.shutdown();
        });
        attempt.start([&](const std::atomic_bool&) {
            entered.release();
            release.acquire();
            return std::unique_ptr<GPSTransport>();
        });
        QVERIFY(entered.tryAcquire(1, 5000));
        auto* worker = attempt.findChild<GPSProvider*>();
        QVERIFY(worker);
        emit worker->configurationFinished({GPSConfigurationStatus::Unsupported, QStringLiteral("NMEA unavailable")});
        emit worker->connectionErrorDetail(GPSConnectionError::ConfigFailed, QStringLiteral("NMEA unavailable"));
        emit worker->connectionError(GPSConnectionError::ConfigFailed);
        QTRY_VERIFY_WITH_TIMEOUT(attempt.attempt().failure.has_value(), 5000);
        QCOMPARE(attempt.attempt().failure->retry, GPSRetryDisposition::AwaitChange);
        QCOMPARE(attempt.attempt().failure->generation, quint64(27));
        QCOMPARE(attempt.attempt().failure->configuration->status, GPSConfigurationStatus::Unsupported);
    }

    void unsupportedReceiverAttemptDoesNotRetry()
    {
        ManualScheduler clock;
        GPSReceiverSession session;
        GPSReceiverAutoConnect control(&session, nullptr, nullptr, &clock);
        QSemaphore entered;
        QSemaphore release;
        std::atomic_int factories = 0;
        const auto cleanup = qScopeGuard([&] {
            control.stop();
            release.release();
            session.shutdown();
        });
        QVERIFY(control.connectNetwork(gpsReceiverFamilies().front().type, [&](const std::atomic_bool&) {
            ++factories;
            entered.release();
            release.acquire();
            return std::unique_ptr<GPSTransport>();
        }));
        QVERIFY(entered.tryAcquire(1, 5000));
        auto* worker = session.findChild<GPSProvider*>();
        QVERIFY(worker);
        emit worker->transportOpenFinished({GPSOpenStatus::Opened});
        emit worker->configurationFinished({GPSConfigurationStatus::Unsupported, QStringLiteral("Role unsupported")});
        emit worker->connectionErrorDetail(GPSConnectionError::ConfigFailed, QStringLiteral("Role unsupported"));
        emit worker->connectionError(GPSConnectionError::ConfigFailed);
        QTRY_COMPARE_WITH_TIMEOUT(control.connectionState(), GPSConnectionState::AwaitingChange, 5000);
        QVERIFY(session.attempt().failure.has_value());
        QCOMPARE(session.attempt().failure->cause, GPSReceiverFailure::Cause::Unsupported);
        release.release();
        QTRY_VERIFY_WITH_TIMEOUT(!session.hasReceiver(), 5000);
        QVERIFY(clock.advanceBy(std::chrono::hours(24)));
        QCOMPARE(factories.load(), 1);
        QCOMPARE(control.connectionState(), GPSConnectionState::AwaitingChange);
        auto profile = session.profile();
        profile.receiver.role = GPSReceiverConfig::Role::Position;
        control.setProfile(profile, true);
        QCOMPARE(control.connectionState(), GPSConnectionState::Disconnected);
    }

    void attemptReducerRejectsStaleAndRepeatedTerminalEvents()
    {
        using Phase = GPSReceiverAttempt::Phase;
        for (const auto cause : {GPSConfigurationStatus::Unsupported, GPSConfigurationStatus::TransportError,
                                 GPSConfigurationStatus::Cancelled}) {
            GPSReceiverAttempt attempt;
            attempt.generation = 42;
            attempt.phase = Phase::Connecting;
            QVERIFY(!gpsReduceReceiverAttempt(attempt, {41, GPSOpenResult{GPSOpenStatus::Opened}}));
            QVERIFY(gpsReduceReceiverAttempt(attempt, {42, GPSOpenResult{GPSOpenStatus::Opened}}));
            QVERIFY(gpsReduceReceiverAttempt(attempt, {42, Phase::Configuring}));
            const GPSConfigurationResult result{cause, QStringLiteral("receiver result")};
            QVERIFY(gpsReduceReceiverAttempt(attempt, {42, result}));
            const auto failure = GPSReceiverFailure::from(42, GPSConnectionError::ConfigFailed, result.error,
                                                          attempt.transportOpen, attempt.configurationResult);
            QVERIFY(gpsReduceReceiverAttempt(attempt, {42, failure}));
            QCOMPARE(attempt.failure->configuration->status, cause);
            QCOMPARE(attempt.phase, cause == GPSConfigurationStatus::Cancelled ? Phase::Cancelled : Phase::Failed);
            QVERIFY(!gpsReduceReceiverAttempt(attempt, {42, failure}));
            QVERIFY(!gpsReduceReceiverAttempt(attempt, {42, Phase::Ready}));
            QVERIFY(!gpsReduceReceiverAttempt(attempt, {42, Phase::Cancelled}));
        }
    }

    void terminalFailureWaitsForChangedIntent()
    {
        ManualScheduler clock;
        GPSConnectionState connection(nullptr, &clock);
        connection.requestConnect();
        QVERIFY(connection.beginAttempt());
        connection.configuring();
        connection.failed(GPSRetryDisposition::AwaitChange);
        QCOMPARE(connection.state(), GPSConnectionState::AwaitingChange);
        QVERIFY(clock.advanceBy(std::chrono::hours(24)));
        QVERIFY(!connection.canAttempt());
        connection.failed();
        QCOMPARE(connection.state(), GPSConnectionState::AwaitingChange);
        connection.requestConnect();
        QVERIFY(connection.beginAttempt());
        connection.failed();
        QCOMPARE(connection.state(), GPSConnectionState::Retrying);
        QVERIFY(clock.advanceBy(std::chrono::seconds(1)));
        QVERIFY(connection.canAttempt());
    }

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
