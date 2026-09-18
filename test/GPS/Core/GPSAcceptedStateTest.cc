#include <algorithm>
#include <memory>
#include <type_traits>

#include <QtTest/QTest>

#include "GPSObservation.h"
#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"
#include "GPSSurveyInStatus.h"
#include "ManualScheduler.h"
#include "MonotonicClock.h"
#include "PortableTest.h"

static_assert(std::is_same_v<decltype(GPSObservation::altitudeDatum), decltype(GPSSurveyInStatus::altitudeDatum)>);

class GPSAcceptedStateTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _consumerPolicies_data();
    void _consumerPolicies();
    void _ggaDoesNotRequireAccuracy();
    void _independentSatelliteExpiry();
    void _freshnessReconfiguration_data();
    void _freshnessReconfiguration();
    void _futureReceiptRemainsRejected_data();
    void _futureReceiptRemainsRejected();
    void _maximumAge_data();
    void _maximumAge();
    void _receiptDeadlineBoundaries();
    void _satelliteNormalization_data();
    void _satelliteNormalization();
    void _surveyStatusRetainsUnitsAndProvenance();
    void _schedulerDestructionClearsAcceptedState();
};

void GPSAcceptedStateTest::_receiptDeadlineBoundaries()
{
    using namespace std::chrono_literals;
    QCOMPARE(MonotonicClock::remaining(0, 100, 10us), 0us);
    QCOMPARE(MonotonicClock::remaining(101, 100, 10us), 0us);
    QCOMPARE(MonotonicClock::remaining(100, 100, 0us), 0us);
    QCOMPARE(MonotonicClock::remaining(100, 100, -1us), 0us);
    QCOMPARE(MonotonicClock::remaining(100, 100, 10us), 10us);
    QCOMPARE(MonotonicClock::remaining(100, 109, 10us), 1us);
    QCOMPARE(MonotonicClock::remaining(100, 110, 10us), 0us);
    QCOMPARE(MonotonicClock::remaining(100, 111, 10us), 0us);
}

void GPSAcceptedStateTest::_consumerPolicies_data()
{
    QTest::addColumn<GPSObservation::PositionUse>("use");
    QTest::addColumn<double>("verticalAccuracy");
    QTest::addColumn<double>("speed");
    QTest::addColumn<double>("altitude");
    QTest::addColumn<bool>("courseAvailable");
    using Use = GPSObservation::PositionUse;
    QTest::newRow("gcs-accurate-altitude") << Use::GroundStation << 1.0 << 0.0 << 500.0 << true;
    QTest::newRow("gcs-uncertain-altitude") << Use::GroundStation << 11.0 << 0.0 << qQNaN() << true;
    QTest::newRow("motion-stationary") << Use::Motion << 1.0 << 0.0 << 500.0 << false;
    QTest::newRow("motion-moving") << Use::Motion << 1.0 << 1.0 << 500.0 << true;
    QTest::newRow("motion-uncertain-altitude") << Use::Motion << 11.0 << 1.0 << 500.0 << true;
    QTest::newRow("motion-missing-vertical-accuracy") << Use::Motion << qQNaN() << 1.0 << 500.0 << true;
    QTest::newRow("remote-id-ellipsoid") << Use::RemoteID << 11.0 << 0.0 << 550.0 << true;
    QTest::newRow("gga-raw-altitude") << Use::Gga << 11.0 << 0.0 << 500.0 << true;
}

void GPSAcceptedStateTest::_consumerPolicies()
{
    QFETCH(GPSObservation::PositionUse, use);
    QFETCH(double, verticalAccuracy);
    QFETCH(double, speed);
    QFETCH(double, altitude);
    QFETCH(bool, courseAvailable);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    if (qIsFinite(verticalAccuracy)) {
        observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, verticalAccuracy);
    }
    observation.position.setAttribute(QGeoPositionInfo::GroundSpeed, speed);
    observation.position.setAttribute(QGeoPositionInfo::Direction, 90.0);
    observation.position.setAttribute(QGeoPositionInfo::DirectionAccuracy, 1.0);
    observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    observation.altitudeEllipsoidMeters = 550.0;
    health.updateObservation(observation);
    const auto accepted = health.acceptedObservation(use);
    QVERIFY(accepted);
    if (qIsNaN(altitude)) {
        QVERIFY(qIsNaN(accepted->position.coordinate().altitude()));
        QVERIFY(!accepted->position.hasAttribute(QGeoPositionInfo::VerticalAccuracy));
    } else {
        QCOMPARE(accepted->position.coordinate().altitude(), altitude);
    }
    QCOMPARE(accepted->position.hasAttribute(QGeoPositionInfo::Direction), courseAvailable);
    QCOMPARE(accepted->position.hasAttribute(QGeoPositionInfo::DirectionAccuracy), courseAvailable);
    QCOMPARE(accepted->altitudeDatum, use == GPSObservation::PositionUse::RemoteID ? GPSAltitudeDatum::Ellipsoid
                                                                                   : GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(health.observation().position, observation.position);
    const auto projected = observation.projected(use);
    QVERIFY(projected);
    QCOMPARE(accepted->position, projected->position);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QVERIFY(!health.acceptedObservation(use));
}

void GPSAcceptedStateTest::_ggaDoesNotRequireAccuracy()
{
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8), QDateTime::currentDateTimeUtc());
    QVERIFY(!observation.projected(GPSObservation::PositionUse::GroundStation));
    health.updateObservation(observation);
    QVERIFY(!health.acceptedObservation());
    QVERIFY(health.acceptedObservation(GPSObservation::PositionUse::Gga));
    observation.receiverFixValid = false;
    health.updateObservation(observation);
    QVERIFY(!health.acceptedObservation(GPSObservation::PositionUse::Gga));
    observation.receiverFixValid = true;
    observation.fixQuality = GPSObservation::FixQuality::NoFix;
    health.updateObservation(observation);
    QVERIFY(!health.acceptedObservation(GPSObservation::PositionUse::Gga));
    observation.fixQuality = GPSObservation::FixQuality::Fix3D;
    health.updateObservation(observation);
    QVERIFY(health.acceptedObservation(GPSObservation::PositionUse::Gga));
    health.invalidatePosition();
    QVERIFY(!health.acceptedObservation());
    QVERIFY(!health.acceptedObservation(GPSObservation::PositionUse::Gga));
    QCOMPARE(health.observation().position, observation.position);
    health.reset();
    QVERIFY(!health.acceptedObservation(GPSObservation::PositionUse::Gga));
}

void GPSAcceptedStateTest::_independentSatelliteExpiry()
{
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8), QDateTime::currentDateTimeUtc());
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    observation.satellitesUsed = 12;
    health.updateObservation(observation);
    QCOMPARE(health.acceptedObservation()->satellitesUsed, std::optional<int>(12));
    health.clearSatellites();
    QVERIFY(health.acceptedObservation());
    QVERIFY(!health.acceptedObservation()->satellitesUsed);
    QCOMPARE(health.observation().satellitesUsed, std::optional<int>(12));
}

void GPSAcceptedStateTest::_freshnessReconfiguration_data()
{
    QTest::addColumn<bool>("clearCount");
    QTest::addColumn<int>("timeoutMs");
    QTest::newRow("cleared-same") << true << 5000;
    QTest::newRow("cleared-shorter") << true << 2000;
    QTest::newRow("cleared-longer") << true << 10000;
    QTest::newRow("invalid-position-shorter") << false << 2000;
    QTest::newRow("invalid-position-longer") << false << 10000;
}

void GPSAcceptedStateTest::_freshnessReconfiguration()
{
    QFETCH(bool, clearCount);
    QFETCH(int, timeoutMs);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8), QDateTime::currentDateTimeUtc());
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    observation.satellitesUsed = 12;
    health.updateObservation(observation);
    if (clearCount) {
        health.clearSatellites();
    } else {
        health.invalidatePosition();
    }
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    health.setFreshnessTimeoutMs(timeoutMs);
    QCOMPARE(health.satellitesInUseCount(), clearCount ? -1 : 12);
    QCOMPARE(health.observation().satellitesUsed, std::optional<int>(12));
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(timeoutMs - 1001)));
    QCOMPARE(health.satellitesInUseCount(), clearCount ? -1 : 12);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(health.satellitesInUseCount(), -1);
    QVERIFY(!health.acceptedObservation());
    health.setFreshnessTimeoutMs(timeoutMs * 2);
    QCOMPARE(health.satellitesInUseCount(), -1);
}

void GPSAcceptedStateTest::_futureReceiptRemainsRejected_data()
{
    QTest::addColumn<GPSObservation::PositionUse>("use");
    using Use = GPSObservation::PositionUse;
    QTest::newRow("ground-station") << Use::GroundStation;
    QTest::newRow("motion") << Use::Motion;
    QTest::newRow("remote-id") << Use::RemoteID;
    QTest::newRow("gga-without-accuracy") << Use::Gga;
}

void GPSAcceptedStateTest::_futureReceiptRemainsRejected()
{
    QFETCH(GPSObservation::PositionUse, use);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs() + 1000000;
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
    observation.satellitesUsed = 12;
    if (use != GPSObservation::PositionUse::Gga) {
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    }
    health.updateObservation(observation);
    QCOMPARE(health.state(), GPSSourceHealth::State::Invalid);
    QVERIFY(!health.acceptedObservation(use));
    health.setFreshnessTimeoutMs(10000);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QVERIFY(!health.acceptedObservation(use));
    for (const int timeoutMs : {1000, 10000, 5000}) {
        health.setFreshnessTimeoutMs(timeoutMs);
        QVERIFY(!health.acceptedObservation(use));
        QCOMPARE(health.state(), GPSSourceHealth::State::Invalid);
        QCOMPARE(health.satellitesInUseCount(), -1);
    }
    observation.monotonicTimestampUs = scheduler.nowUs();
    health.updateObservation(observation);
    QVERIFY(health.acceptedObservation(use));
    QCOMPARE(health.acceptedObservation(use)->satellitesUsed, std::optional<int>(12));
}

void GPSAcceptedStateTest::_maximumAge_data()
{
    QTest::addColumn<int>("sourceLifetimeMs");
    QTest::newRow("stricter-source") << 2000;
    QTest::newRow("same-lifetime") << 5000;
    QTest::newRow("stricter-consumer") << 10000;
}

void GPSAcceptedStateTest::_maximumAge()
{
    QFETCH(int, sourceLifetimeMs);
    using namespace std::chrono_literals;
    using Use = GPSObservation::PositionUse;
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    health.setFreshnessTimeoutMs(sourceLifetimeMs);
    GPSObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.receivedAt = QDateTime::fromMSecsSinceEpoch(1000).toUTC();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), observation.receivedAt);
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    health.updateObservation(observation);
    QVERIFY(!health.acceptedObservation(Use::RemoteID, 0ms));
    QVERIFY(!health.acceptedObservation(Use::RemoteID, -1ms));
    QVERIFY(health.acceptedObservation(Use::RemoteID, std::chrono::milliseconds::max()));
    const auto lifetime = std::min(std::chrono::milliseconds(sourceLifetimeMs), 5000ms);
    QVERIFY(scheduler.advanceBy(lifetime - 1us));
    const auto accepted = health.acceptedObservation(Use::RemoteID, 5000ms);
    QVERIFY(accepted);
    QCOMPARE(accepted->monotonicTimestampUs, observation.monotonicTimestampUs);
    QCOMPARE(accepted->receivedAt, observation.receivedAt);
    QVERIFY(scheduler.advanceBy(1us));
    QVERIFY(!health.acceptedObservation(Use::RemoteID, 5000ms));
    QCOMPARE(health.acceptedObservation(Use::RemoteID).has_value(), sourceLifetimeMs > 5000);
    observation.monotonicTimestampUs = scheduler.nowUs();
    health.updateObservation(observation);
    QVERIFY(health.acceptedObservation(Use::RemoteID, 5000ms));
}

void GPSAcceptedStateTest::_satelliteNormalization_data()
{
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("unknownUsage");
    QTest::newRow("empty") << true << false;
    QTest::newRow("known") << false << false;
    QTest::newRow("partly-unknown") << false << true;
}

void GPSAcceptedStateTest::_satelliteNormalization()
{
    QFETCH(bool, empty);
    QFETCH(bool, unknownUsage);
    ManualScheduler scheduler;
    GPSSatelliteStore native(nullptr, 1000, &scheduler);
    GPSSatelliteStore normalized(nullptr, 1000, &scheduler);
    native.beginSession(QStringLiteral("receiver"), 1);
    normalized.beginSession(QStringLiteral("receiver"), 1);
    GPSSatelliteObservation report;
    report.sessionId = 1;
    report.monotonicTimestampUs = scheduler.nowUs();
    using Constellation = GPSConstellation;
    if (!empty) {
        GPSSatellite gps;
        gps.id = 1;
        gps.constellation = Constellation::GPS;
        gps.used = false;
        GPSSatellite galileo;
        galileo.id = 2;
        galileo.constellation = Constellation::Galileo;
        galileo.used = unknownUsage ? std::nullopt : std::optional<bool>(true);
        report.satellites = {gps, galileo};
    }
    native.updateObservation(report);
    const auto receipt = report.monotonicTimestampUs;
    if (empty) {
        report.provenance = {{Constellation::Unknown, receipt, receipt, 0}};
    } else {
        report.provenance = {
            {Constellation::GPS, receipt, receipt, 0},
            {Constellation::Galileo, receipt, unknownUsage ? 0 : receipt,
             unknownUsage ? std::nullopt : std::optional<int>(1)},
        };
    }
    normalized.updateObservation(report);
    const auto actual = native.observation();
    const auto expected = normalized.observation();
    QCOMPARE(actual.satellitesInViewCount(), expected.satellitesInViewCount());
    QCOMPARE(actual.satellitesInUseCount(), expected.satellitesInUseCount());
    QCOMPARE(actual.satellites.size(), expected.satellites.size());
    QCOMPARE(actual.provenance.size(), expected.provenance.size());
    for (qsizetype index = 0; index < actual.satellites.size(); ++index) {
        QCOMPARE(actual.satellites[index].id, expected.satellites[index].id);
        QCOMPARE(actual.satellites[index].used, expected.satellites[index].used);
    }
    for (qsizetype index = 0; index < actual.provenance.size(); ++index) {
        QCOMPARE(actual.provenance[index].inViewTimestampUs, expected.provenance[index].inViewTimestampUs);
        QCOMPARE(actual.provenance[index].inUseTimestampUs, expected.provenance[index].inUseTimestampUs);
        QCOMPARE(actual.provenance[index].satellitesUsed, expected.provenance[index].satellitesUsed);
    }
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(native.observation().satellitesInViewCount(), -1);
    QCOMPARE(normalized.observation().satellitesInUseCount(), -1);
}

void GPSAcceptedStateTest::_surveyStatusRetainsUnitsAndProvenance()
{
    GPSSurveyInStatus status;
    QVERIFY(!status.coordinate.isValid());
    QVERIFY(!status.meanAccuracyMeters);
    QCOMPARE(status.altitudeDatum, GPSAltitudeDatum::Unknown);

    status.coordinate = QGeoCoordinate(47, 8);
    status.altitudeEllipsoidMeters = 500;
    status.meanAccuracyMeters = 4000000.001;
    status.duration = std::chrono::seconds(4294967295LL);
    status.altitudeDatum = GPSAltitudeDatum::Ellipsoid;
    status.sessionId = 42;
    status.monotonicTimestampUs = 100;
    const auto restored = QVariant::fromValue(status).value<GPSSurveyInStatus>();
    QCOMPARE(restored.coordinate, QGeoCoordinate(47, 8));
    QCOMPARE(restored.altitudeEllipsoidMeters, 500.0f);
    QCOMPARE(restored.meanAccuracyMeters.value(), 4000000.001);
    QCOMPARE(restored.duration.count(), 4294967295LL);
    QCOMPARE(restored.altitudeDatum, GPSAltitudeDatum::Ellipsoid);
    QCOMPARE(restored.sessionId, quint64{42});
    QCOMPARE(restored.monotonicTimestampUs, quint64{100});
}

void GPSAcceptedStateTest::_schedulerDestructionClearsAcceptedState()
{
    auto scheduler = std::make_unique<ManualScheduler>();
    GPSSatelliteStore satellites(nullptr, 5000, scheduler.get());
    GPSSourceHealth health(nullptr, scheduler.get());
    satellites.beginSession(QStringLiteral("receiver"), 1);
    GPSSatelliteObservation report;
    report.sessionId = 1;
    report.monotonicTimestampUs = scheduler->nowUs();
    report.satellites = {GPSSatellite{}};
    satellites.updateObservation(report);
    GPSObservation fix;
    fix.monotonicTimestampUs = scheduler->nowUs();
    fix.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
    fix.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    health.updateObservation(fix);
    QCOMPARE(satellites.observation().satellites.size(), 1);
    QVERIFY(health.usable());
    scheduler.reset();
    QVERIFY(satellites.observation().satellites.isEmpty());
    QVERIFY(!health.usable());
    satellites.updateObservation(report);
    satellites.clear();
    satellites.reset();
    satellites.beginSession(QStringLiteral("replacement"), 2);
    QVERIFY(satellites.observation().satellites.isEmpty());
}

QGC_REGISTER_PORTABLE_TEST(GPSAcceptedStateTest, TestLabel::Unit)

#include "GPSAcceptedStateTest.moc"
