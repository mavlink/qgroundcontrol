#include "GPSSatelliteStoreTest.h"

#include <QtTest/QSignalSpy>

#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"
#include "ManualScheduler.h"
#include "MonotonicClock.h"

void GPSSatelliteStoreTest::_constellationRetirement()
{
    GPSSatelliteStore store(nullptr, 500);
    store.beginSession(QStringLiteral("receiver"), 1);
    const quint64 nowUs = MonotonicClock::nowUs();
    GPSSatelliteObservation raw;
    raw.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    raw.sessionId = 1;
    raw.constellations = {
        {GPSConstellation::GPS, {nowUs - 400000, 1}, {nowUs - 400000, 1}},
        {GPSConstellation::Galileo, {nowUs - 10000, 1}, {nowUs - 10000, 0}},
    };
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    QCOMPARE(store.observation().satellitesInUseCount(), 1);
    store.setFreshnessTimeoutMs(100);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().constellations.first().constellation, GPSConstellation::Galileo);
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
    QCOMPARE(store.observation().constellations.first().view.receivedAtUs, nowUs - 10000);
    store.setFreshnessTimeoutMs(500);
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    raw.constellations[0].view.receivedAtUs = MonotonicClock::nowUs() + 1000000;
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    raw.constellations[0].view.receivedAtUs = MonotonicClock::nowUs();
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
}

void GPSSatelliteStoreTest::_viewAndUseExpireIndependently()
{
    GPSSatelliteStore store(nullptr, 500);
    GPSSourceHealth health;
    connect(&store, &GPSSatelliteStore::observationChanged, &health, &GPSSourceHealth::applySatelliteObservation);
    store.beginSession(QStringLiteral("nmeaReceiver"), 1);
    const quint64 nowUs = MonotonicClock::nowUs();
    GPSSatelliteObservation raw;
    raw.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    raw.sessionId = 1;
    raw.constellations = {{GPSConstellation::GPS, {nowUs - 10000, 1}, {nowUs - 400000, 1}}};
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
    QCOMPARE(health.satellitesInViewCount(), 1);
    QCOMPARE(health.satellitesInUseCount(), 8);
    raw.constellations[0].usage.receivedAtUs = MonotonicClock::nowUs();
    raw.constellations[0].usage.count = 0;
    store.updateObservation(raw);
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
    const quint64 nowUs = MonotonicClock::nowUs();
    GPSSatelliteObservation raw;
    raw.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    raw.sessionId = 1;
    raw.constellations = {
        {GPSConstellation::GPS, {nowUs - 900000, 1}, {nowUs - 900000, 1}},
        {GPSConstellation::Galileo, {nowUs, 1}, {nowUs, 1}},
    };
    store.updateObservation(raw);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 1, TestTimeout::shortMs());
    QCOMPARE(store.observation().constellations.first().constellation, GPSConstellation::Galileo);
    QCOMPARE(store.observation().constellations.first().view.receivedAtUs, nowUs);
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), -1, TestTimeout::mediumMs());
}

void GPSSatelliteStoreTest::_sessionsAndReentrantDelivery()
{
    GPSSatelliteStore store;
    store.beginSession(QStringLiteral("receiver"), 1);
    const auto receipt = MonotonicClock::nowUs();
    GPSSatelliteObservation raw{receipt, 1, {{GPSConstellation::Unknown, {receipt, 1}, {receipt, 1}}}};
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
    const auto firstReceipt = scheduler.nowUs();
    report.constellations = {{GPSConstellation::GPS, {firstReceipt, 1}, {firstReceipt, 1}}};
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), 1);

    report.constellations[0].usage.receivedAtUs = 0;
    report.constellations[0].usage.count.reset();
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), 1);

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    report.constellations[0].usage.receivedAtUs = scheduler.nowUs();
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
    QVERIFY(!store.observation().constellations.first().usage.count);

    report.constellations[0].usage.receivedAtUs = firstReceipt;
    report.constellations[0].usage.count = 1;
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    report.constellations[0].usage.receivedAtUs = scheduler.nowUs();
    report.constellations[0].usage.count = 0;
    store.updateObservation(report);
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
}

UT_REGISTER_TEST(GPSSatelliteStoreTest, TestLabel::Unit)

void GPSSatelliteStoreTest::_fullSnapshotsReplaceAndDeltasPreserve()
{
    GPSSatelliteStore store;
    store.beginSession(QStringLiteral("receiver"), 1);
    const auto now = MonotonicClock::nowUs();
    GPSSatelliteObservation full{now - 10000,
                                 1,
                                 {{GPSConstellation::GPS, {now - 10000, 1}, {now - 10000, 1}},
                                  {GPSConstellation::Galileo, {now - 10000, 1}, {now - 10000, 1}}}};
    store.updateObservation(full);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    GPSSatelliteObservation delta{now - 9000, 1, {{GPSConstellation::GPS, {now - 9000, 1}, {now - 9000, 1}}}};
    delta.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    store.updateObservation(delta);
    QCOMPARE(store.observation().satellitesInViewCount(), 2);
    QCOMPARE(store.observation().constellations[0].view.receivedAtUs, now - 9000);
    full.monotonicTimestampUs = now - 8000;
    full.constellations = {{GPSConstellation::GPS, {now - 8000, 1}, {}}};
    store.updateObservation(full);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().constellations[0].view.receivedAtUs, now - 8000);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
    // A retired constellation cannot be restored by an older queued delta.
    delta.constellations = {{GPSConstellation::Galileo, {now - 9000, 1}, {now - 9000, 1}}};
    store.updateObservation(delta);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    full.monotonicTimestampUs = now - 7000;
    full.constellations = {{GPSConstellation::Unknown, {now - 7000, 0}, {now - 7000, 0}}};
    store.updateObservation(full);
    QCOMPARE(store.observation().satellitesInViewCount(), 0);
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
    // Full snapshots also cannot overwrite a newer delta already accepted for that constellation.
    delta.monotonicTimestampUs = now - 5000;
    delta.constellations[0].view.receivedAtUs = now - 5000;
    delta.constellations[0].usage.receivedAtUs = now - 5000;
    store.updateObservation(delta);
    full.monotonicTimestampUs = now - 6000;
    full.constellations[0].view.receivedAtUs = now - 6000;
    full.constellations[0].usage.receivedAtUs = now - 6000;
    store.updateObservation(full);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().constellations.last().constellation, GPSConstellation::Galileo);
}

void GPSSatelliteStoreTest::_independentRetirement_data()
{
    QTest::addColumn<bool>("retireView");
    QTest::addColumn<bool>("fullSnapshot");
    QTest::newRow("expired-view") << true << false;
    QTest::newRow("expired-usage") << false << false;
    QTest::newRow("omitted-view") << true << true;
    QTest::newRow("omitted-usage") << false << true;
}

void GPSSatelliteStoreTest::_independentRetirement()
{
    QFETCH(bool, retireView);
    QFETCH(bool, fullSnapshot);
    ManualScheduler scheduler;
    GPSSatelliteStore store(nullptr, 5000, &scheduler);
    store.beginSession(QStringLiteral("receiver"), 1);
    GPSSatelliteObservation initial;
    initial.sessionId = 1;
    initial.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    initial.constellations = {{GPSConstellation::GPS, {scheduler.nowUs(), 1}, {scheduler.nowUs(), 1}}};
    store.updateObservation(initial);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));

    GPSSatelliteObservation retained;
    retained.sessionId = 1;
    retained.updateMode = fullSnapshot ? GPSSatelliteObservation::UpdateMode::FullSnapshot
                                       : GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    if (retireView) {
        retained.constellations = {{GPSConstellation::GPS, {}, {scheduler.nowUs(), 1}}};
    } else {
        retained.constellations = {{GPSConstellation::GPS, {scheduler.nowUs(), 1}, {}}};
    }
    store.updateObservation(retained);
    if (!fullSnapshot) {
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(4)));
    }
    const auto verifyRetained = [&]() {
        const auto accepted = store.observation();
        QCOMPARE(accepted.satellitesInViewCount(), retireView ? -1 : 1);
        QCOMPARE(accepted.satellitesInUseCount(), retireView ? 1 : -1);
    };
    verifyRetained();
    store.setFreshnessTimeoutMs(10000);
    store.updateObservation(initial);
    verifyRetained();
}
