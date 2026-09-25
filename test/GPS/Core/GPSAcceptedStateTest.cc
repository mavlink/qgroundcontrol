#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numbers>

#include <QtTest/QTest>

#include "GPSDriverReports.h"
#include "GPSObservation.h"
#include "GPSSourceHealth.h"
#include "ManualScheduler.h"
#include "MonotonicClock.h"
#include "UnitTest.h"

class GPSAcceptedStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _consumerPolicies_data();
    void _consumerPolicies();
    void _remoteIdDatum_data();
    void _remoteIdDatum();
    void _ggaDoesNotRequireAccuracy();
    void _freshnessReconfiguration_data();
    void _freshnessReconfiguration();
    void _futureReceiptRemainsRejected_data();
    void _futureReceiptRemainsRejected();
    void _maximumAge_data();
    void _maximumAge();
    void _receiptDeadlineBoundaries();
    void _surveyReportRetainsUnits();
    void _navigationObservation();
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

void GPSAcceptedStateTest::_remoteIdDatum_data()
{
    QTest::addColumn<GPSAltitudeDatum>("datum");
    QTest::addColumn<double>("altitude");
    QTest::addColumn<double>("ellipsoid");
    QTest::addColumn<double>("expected");
    QTest::newRow("unknown") << GPSAltitudeDatum::Unknown << 500.0 << qQNaN() << qQNaN();
    QTest::newRow("msl-without-geoid") << GPSAltitudeDatum::MeanSeaLevel << 500.0 << qQNaN() << qQNaN();
    QTest::newRow("msl-with-ellipsoid") << GPSAltitudeDatum::MeanSeaLevel << 500.0 << 550.0 << 550.0;
    QTest::newRow("ellipsoid-coordinate") << GPSAltitudeDatum::Ellipsoid << 550.0 << qQNaN() << 550.0;
    QTest::newRow("ellipsoid-only") << GPSAltitudeDatum::MeanSeaLevel << qQNaN() << 550.0 << 550.0;
    QTest::newRow("nonfinite-ellipsoid") << GPSAltitudeDatum::Ellipsoid << qInf() << qQNaN() << qQNaN();
}

void GPSAcceptedStateTest::_remoteIdDatum()
{
    QFETCH(GPSAltitudeDatum, datum);
    QFETCH(double, altitude);
    QFETCH(double, ellipsoid);
    QFETCH(double, expected);
    GPSObservation observation;
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, altitude), QDateTime::currentDateTimeUtc());
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
    observation.altitudeDatum = datum;
    if (qIsFinite(ellipsoid)) {
        observation.altitudeEllipsoidMeters = ellipsoid;
    }
    const auto projected = observation.projected(GPSObservation::PositionUse::RemoteID);
    QVERIFY(projected);
    QCOMPARE(projected->position.coordinate().latitude(), 47);
    QCOMPARE(projected->position.coordinate().longitude(), 8);
    if (qIsFinite(expected)) {
        QCOMPARE(projected->position.coordinate().altitude(), expected);
        QCOMPARE(projected->altitudeDatum, GPSAltitudeDatum::Ellipsoid);
    } else {
        QCOMPARE(projected->position.coordinate().type(), QGeoCoordinate::Coordinate2D);
        QVERIFY(!projected->position.hasAttribute(QGeoPositionInfo::VerticalAccuracy));
        QCOMPARE(projected->altitudeDatum, GPSAltitudeDatum::Unknown);
    }
    QCOMPARE(observation.altitudeDatum, datum);
}

void GPSAcceptedStateTest::_freshnessReconfiguration_data()
{
    QTest::addColumn<bool>("invalidate");
    QTest::addColumn<int>("timeoutMs");
    QTest::newRow("same") << false << 5000;
    QTest::newRow("shorter") << false << 2000;
    QTest::newRow("longer") << false << 10000;
    QTest::newRow("invalid-position-shorter") << true << 2000;
    QTest::newRow("invalid-position-longer") << true << 10000;
}

void GPSAcceptedStateTest::_freshnessReconfiguration()
{
    QFETCH(bool, invalidate);
    QFETCH(int, timeoutMs);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8), QDateTime::currentDateTimeUtc());
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    observation.satellitesUsed = 12;
    health.updateObservation(observation);
    if (invalidate) {
        health.invalidatePosition();
    }
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    health.setFreshnessTimeoutMs(timeoutMs);
    QCOMPARE(bool(health.acceptedObservation()), !invalidate);
    if (!invalidate) {
        QCOMPARE(health.acceptedObservation()->satellitesUsed, std::optional<int>(12));
    }
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(timeoutMs - 1001)));
    QCOMPARE(bool(health.acceptedObservation()), !invalidate);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY(!health.acceptedObservation());
    QCOMPARE(health.state(), GPSSourceHealth::State::Stale);
    health.setFreshnessTimeoutMs(timeoutMs * 2);
    QCOMPARE(health.observation().satellitesUsed, std::optional<int>(12));
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

void GPSAcceptedStateTest::_surveyReportRetainsUnits()
{
    GPSSurveyReport status;
    QVERIFY(std::isnan(status.position.latitudeDegrees));
    QVERIFY(!status.meanAccuracyMeters);

    status.position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500};
    status.meanAccuracyMeters = 4000000.001;
    status.duration = std::chrono::seconds(4294967295LL);
    const auto restored = QVariant::fromValue(status).value<GPSSurveyReport>();
    QCOMPARE(restored.position.latitudeDegrees, 47.0);
    QCOMPARE(restored.position.longitudeDegrees, 8.0);
    QCOMPARE(restored.position.altitudeMeters, 500.0f);
    QCOMPARE(restored.meanAccuracyMeters.value(), 4000000.001);
    QCOMPARE(restored.duration.count(), 4294967295LL);
}

UT_REGISTER_TEST(GPSAcceptedStateTest, TestLabel::Unit)

void GPSAcceptedStateTest::_navigationObservation()
{
    GPSNavigationValues navigation;
    navigation.fixType = GPSFixQuality::RTKFloat;
    navigation.utcTimeUs = 1'700'000'000'123'000ULL;
    navigation.latitudeDegrees = 47.5;
    navigation.longitudeDegrees = 8.25;
    navigation.altitudeMslMeters = 450;
    navigation.altitudeEllipsoidMeters = 497.5;
    navigation.horizontalAccuracyMeters = 0.3f;
    navigation.verticalAccuracyMeters = 0.6f;
    navigation.horizontalDop = 0.8f;
    navigation.speedMetersPerSecond = 2;
    navigation.courseRadians = -std::numbers::pi_v<float> / 2;
    navigation.satellitesUsed = 18;
    const auto observation = GPSObservation::fromNavigation(navigation, 1234);
    QVERIFY(observation.usable());
    QCOMPARE(observation.monotonicTimestampUs, quint64(1234));
    QCOMPARE(observation.fixQuality, GPSFixQuality::RTKFloat);
    QCOMPARE(observation.position.timestamp().toMSecsSinceEpoch(), qint64(1'700'000'000'123));
    QCOMPARE(observation.coordinate(), QGeoCoordinate(47.5, 8.25, 450));
    QCOMPARE(observation.altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(observation.altitudeEllipsoidMeters, std::optional<double>(497.5));
    QCOMPARE(observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy), qreal(0.3f));
    QCOMPARE(observation.horizontalDop, std::optional<double>(0.8f));
    QCOMPARE(observation.satellitesUsed, std::optional<int>(18));
    QVERIFY(qAbs(observation.heading() - 270) < 1e-3);
    QVERIFY(!observation.verticalDop);

    // NMEA receivers without GST report only DOP.
    navigation.fixType = GPSFixQuality::Differential;
    navigation.horizontalAccuracyMeters = std::numeric_limits<float>::quiet_NaN();
    navigation.horizontalDop = 0.5f;
    const auto dopOnly = GPSObservation::fromNavigation(navigation, 1236);
    QVERIFY(dopOnly.usable());
    QCOMPARE(dopOnly.position.attribute(QGeoPositionInfo::HorizontalAccuracy), GPSObservation::accuracyFromDop(0.5f));

    navigation.fixType = GPSFixQuality::NoFix;
    navigation.altitudeMslMeters = std::numeric_limits<double>::quiet_NaN();
    navigation.utcTimeUs = 0;
    const auto lost = GPSObservation::fromNavigation(navigation, 1235);
    QVERIFY(!lost.hasNavigationSolution());
    QCOMPARE(lost.altitudeDatum, GPSAltitudeDatum::Unknown);
    QVERIFY(lost.position.timestamp().isValid());
    QVERIFY(!GPSObservation::fromNavigation(GPSNavigationValues{}, 1).position.isValid());
}

#include "GPSAcceptedStateTest.moc"
