#include "GPSSatelliteStoreTest.h"

#include <QtTest/QSignalSpy>

#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"
#include "ManualScheduler.h"

namespace {
GPSSatellite makeSatellite(GPSSatellite::Constellation constellation, int id, std::optional<bool> used = true)
{
    GPSSatellite satellite;
    satellite.constellation = constellation;
    satellite.id = id;
    satellite.used = used;
    return satellite;
}
}  // namespace

void GPSSatelliteStoreTest::_constellationRetirement()
{
    GPSSatelliteStore store(nullptr, 500);
    store.beginSession(QStringLiteral("receiver"), 1);
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    GPSSatelliteObservation raw;
    raw.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    raw.sessionId = 1;
    raw.satellites = {makeSatellite(GPSSatellite::Constellation::GPS, 3),
                      makeSatellite(GPSSatellite::Constellation::Galileo, 3, false)};
    raw.provenance = {{GPSSatellite::Constellation::GPS, nowUs - 400000, nowUs - 400000, 1},
                      {GPSSatellite::Constellation::Galileo, nowUs - 10000, nowUs - 10000, 0}};
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    QCOMPARE(store.observation().satellitesInUseCount(), 1);
    store.setFreshnessTimeoutMs(100);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().satellites.first().constellation, GPSSatellite::Constellation::Galileo);
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
    QCOMPARE(store.observation().provenance.first().inViewTimestampUs, nowUs - 10000);
    store.setFreshnessTimeoutMs(500);
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    raw.provenance[0].inViewTimestampUs = GPSObservation::monotonicNowUs() + 1000000;
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    raw.provenance[0].inViewTimestampUs = GPSObservation::monotonicNowUs();
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    QVERIFY(!store.observation().satellites.first().used.has_value());
}

void GPSSatelliteStoreTest::_viewAndUseExpireIndependently()
{
    GPSSatelliteStore store(nullptr, 500);
    GPSSourceHealth health;
    connect(&store, &GPSSatelliteStore::observationChanged, &health, &GPSSourceHealth::applySatelliteObservation);
    store.beginSession(QStringLiteral("nmeaReceiver"), 1);
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    GPSSatelliteObservation raw;
    raw.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    raw.sessionId = 1;
    raw.satellites = {makeSatellite(GPSSatellite::Constellation::GPS, 2)};
    raw.provenance = {{GPSSatellite::Constellation::GPS, nowUs - 10000, nowUs - 400000, 1}};
    store.updateObservation(raw);
    GPSObservation fix;
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.0, 8.0, 500.0), QDateTime::currentDateTimeUtc());
    fix.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    fix.monotonicTimestampUs = nowUs;
    fix.satellitesUsed = 8;
    health.updateObservation(fix);
    store.setFreshnessTimeoutMs(100);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
    QVERIFY(!store.observation().satellites.first().used.has_value());
    QCOMPARE(health.satellitesInViewCount(), 1);
    QCOMPARE(health.satellitesInUseCount(), 8);
    raw.provenance[0].inUseTimestampUs = GPSObservation::monotonicNowUs();
    raw.provenance[0].satellitesUsed = 0;
    raw.satellites[0].used = false;
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellites.first().used, std::optional<bool>(false));
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
    store.clear();
    QCOMPARE(health.satellitesInViewCount(), -1);
    QCOMPARE(health.satellitesInUseCount(), 8);
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), -1);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
}

void GPSSatelliteStoreTest::_timerKeepsFreshConstellation()
{
    GPSSatelliteStore store(nullptr, 1000);
    store.beginSession(QStringLiteral("receiver"), 1);
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    GPSSatelliteObservation raw;
    raw.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    raw.sessionId = 1;
    raw.satellites = {makeSatellite(GPSSatellite::Constellation::GPS, 1),
                      makeSatellite(GPSSatellite::Constellation::Galileo, 2)};
    raw.provenance = {{GPSSatellite::Constellation::GPS, nowUs - 900000, nowUs - 900000, 1},
                      {GPSSatellite::Constellation::Galileo, nowUs, nowUs, 1}};
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 1, 700);
    QCOMPARE(store.observation().satellites.first().constellation, GPSSatellite::Constellation::Galileo);
    QCOMPARE(store.observation().provenance.first().inViewTimestampUs, nowUs);
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), -1, 2000);
}

void GPSSatelliteStoreTest::_sessionsAndReentrantDelivery()
{
    GPSSatelliteStore store;
    store.beginSession(QStringLiteral("receiver"), 1);
    const auto satellite = makeSatellite(GPSSatellite::Constellation::Unknown, 4);
    GPSSatelliteObservation raw{GPSObservation::monotonicNowUs(), 1, {satellite}};
    store.updateObservation(raw);
    const auto accepted = store.observation();
    store.beginSession(QStringLiteral("replacement"), 2);
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), -1);
    raw.sessionId = 2;
    bool received = false;
    connect(&store, &GPSSatelliteStore::observationChanged, &store, [&](const GPSSatelliteObservation& observation) {
        if (observation.satellitesInViewCount() >= 0 && !received) {
            received = true;
            const auto revision = observation.revision;
            store.reset();
            QCOMPARE(observation.sourceId, QStringLiteral("replacement"));
            QCOMPARE(observation.sessionId, 2ULL);
            QCOMPARE(observation.revision, revision);
            QCOMPARE(observation.satellitesInViewCount(), 1);
        }
    });
    store.updateObservation(raw);
    QVERIFY(received);
    QVERIFY(store.observation().sourceId.isEmpty());
    QVERIFY(store.observation().revision > accepted.revision);
}

void GPSSatelliteStoreTest::_unknownUsageRetiresPreviousCount()
{
    ManualScheduler scheduler;
    GPSSatelliteStore store(nullptr, 5000, &scheduler);
    store.beginSession(QStringLiteral("receiver"), 1);
    GPSSatelliteObservation report;
    report.sessionId = 1;
    report.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    report.satellites = {makeSatellite(GPSSatellite::Constellation::GPS, 3)};
    const auto firstReceipt = scheduler.nowUs();
    report.provenance = {{GPSSatellite::Constellation::GPS, firstReceipt, firstReceipt, 1, QList<int>{3}}};
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), 1);
    QCOMPARE(store.observation().satellites.first().used, std::optional<bool>(true));

    report.provenance[0].inUseTimestampUs = 0;
    report.provenance[0].satellitesUsed.reset();
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), 1);

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    report.provenance[0].inUseTimestampUs = scheduler.nowUs();
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
    QVERIFY(!store.observation().provenance.first().usedSatelliteIds);
    QVERIFY(!store.observation().satellites.first().used);

    report.provenance[0].inUseTimestampUs = firstReceipt;
    report.provenance[0].satellitesUsed = 1;
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    report.provenance[0].inUseTimestampUs = scheduler.nowUs();
    report.provenance[0].satellitesUsed = 0;
    report.provenance[0].usedSatelliteIds = QList<int>{};
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
    QCOMPARE(store.observation().satellites.first().used, std::optional<bool>(false));
}

UT_REGISTER_TEST(GPSSatelliteStoreTest, TestLabel::Unit)

void GPSSatelliteStoreTest::_fullSnapshotsReplaceAndDeltasPreserve()
{
    GPSSatelliteStore store;
    store.beginSession(QStringLiteral("receiver"), 1);
    const auto now = GPSObservation::monotonicNowUs();
    GPSSatelliteObservation full{
        now - 10000,
        1,
        {makeSatellite(GPSSatellite::Constellation::GPS, 1), makeSatellite(GPSSatellite::Constellation::Galileo, 2)}};
    store.updateObservation(full);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    GPSSatelliteObservation delta{now - 9000, 1, {makeSatellite(GPSSatellite::Constellation::GPS, 3)}};
    delta.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    store.updateObservation(delta);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    QCOMPARE(store.observation().satellites[0].id, 3);
    full.monotonicTimestampUs = now - 8000;
    full.satellites = {makeSatellite(GPSSatellite::Constellation::GPS, 4, std::nullopt)};
    store.updateObservation(full);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().satellites[0].id, 4);
    QVERIFY(!store.observation().satellites[0].used);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
    // A retired constellation cannot be restored by an older queued delta.
    delta.satellites = {makeSatellite(GPSSatellite::Constellation::Galileo, 2)};
    store.updateObservation(delta);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    full.monotonicTimestampUs = now - 7000;
    full.satellites.clear();
    store.updateObservation(full);
    QCOMPARE(store.observation().satellitesInViewCount(), 0);
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
    // Full snapshots also cannot overwrite a newer delta already accepted for that constellation.
    delta.monotonicTimestampUs = now - 5000;
    store.updateObservation(delta);
    full.monotonicTimestampUs = now - 6000;
    store.updateObservation(full);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().satellites.first().constellation, GPSSatellite::Constellation::Galileo);
}
